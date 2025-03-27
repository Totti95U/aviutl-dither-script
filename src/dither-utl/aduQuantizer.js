"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
/* === Color Space Section === */
class sRGB {
    constructor(r, g, b) {
        // each channel is in [0, 255] with integer value
        this.r = r;
        this.g = g;
        this.b = b;
    }
    toLinearRGB() {
        function toLiner(c) {
            const gamma = 2.4;
            const thr = 0.04045;
            return c <= thr ? c / 12.92 : Math.pow((c + 0.055) / 1.055, gamma);
        }
        let r = toLiner(this.r / 255);
        let g = toLiner(this.g / 255);
        let b = toLiner(this.b / 255);
        return new LinerRGB(r, g, b);
    }
    toOklab() {
        return this.toLinearRGB().toOklab();
    }
    vectorize() {
        return [this.r, this.g, this.b];
    }
}
class LinerRGB {
    constructor(r, g, b) {
        // each channel is in [0, 1] with float value
        this.r = r;
        this.g = g;
        this.b = b;
    }
    toSRGB() {
        function toStandard(c) {
            const gamma = 2.4;
            const thr = 0.0031308;
            return c <= thr ? 12.92 * c : 1.055 * Math.pow(c, 1 / gamma) - 0.055;
        }
        let r = toStandard(this.r);
        let g = toStandard(this.g);
        let b = toStandard(this.b);
        return new sRGB(Math.round(r * 255), Math.round(g * 255), Math.round(b * 255));
    }
    toOklab() {
        const l = 0.4122214708 * this.r + 0.5363325363 * this.g + 0.0514459929 * this.b;
        const m = 0.2119034982 * this.r + 0.6806995451 * this.g + 0.1073969566 * this.b;
        const s = 0.0883024619 * this.r + 0.2817188376 * this.g + 0.6299787005 * this.b;
        const l_ = Math.cbrt(l);
        const m_ = Math.cbrt(m);
        const s_ = Math.cbrt(s);
        const L = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_;
        const a = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_;
        const b = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_;
        return new Oklab(L, a, b);
    }
    vectorize() {
        return [this.r, this.g, this.b];
    }
}
class Oklab {
    constructor(l, a, b) {
        this.l = l;
        this.a = a;
        this.b = b;
    }
    toSRGB() {
        return this.toLinerRGB().toSRGB();
    }
    toLinerRGB() {
        const l_ = this.l + 0.3963377774 * this.a + 0.2158037573 * this.b;
        const m_ = this.l - 0.1055613458 * this.a - 0.0638541728 * this.b;
        const s_ = this.l - 0.0894841775 * this.a - 1.2914855480 * this.b;
        const l = l_ * l_ * l_;
        const m = m_ * m_ * m_;
        const s = s_ * s_ * s_;
        const r = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
        const g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
        const b = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;
        return new LinerRGB(r, g, b);
    }
    vectorize() {
        return [this.l, this.a, this.b];
    }
}
/* === Quantizer Section === */
/**
 * Cluster class for Adaptive Dynamic Uniform Quantization
 */
class AduCluster {
    constructor() {
        this.color = new Oklab(0, 0, 0);
        this.winCount = 0;
    }
}
class AduQuantizer {
    constructor() {
        this.colorSet = [];
        this.clusters = [];
        this.maxWinPer = 1 << 18; // 5 ~ 50
        this.learningRate = 0.1; // 0.01 ~ 0.5
    }
    addColor(color) {
        this.colorSet.push(color);
    }
    addPixels(colorData) {
        return __awaiter(this, void 0, void 0, function* () {
            // c1 will be the average color of the colorData
            let c1 = new AduCluster();
            c1.color = new sRGB(colorData[0], colorData[1], colorData[2]).toOklab();
            this.addColor(c1.color);
            const chunkSize = 3000;
            for (let i = 3; i < colorData.length; i += chunkSize) {
                const end = Math.min(i + chunkSize, colorData.length);
                for (let j = i; j < end; j += 3) {
                    const c = new sRGB(colorData[j], colorData[j + 1], colorData[j + 2]).toOklab();
                    this.addColor(c);
                    c1.color.l += c.l;
                    c1.color.a += c.a;
                    c1.color.b += c.b;
                }
                yield new Promise((resolve) => setImmediate(resolve));
            }
            c1.color.l /= colorData.length;
            c1.color.a /= colorData.length;
            c1.color.b /= colorData.length;
            this.clusters.push(c1);
        });
    }
    reducePalette(targetColors, progressCallback) {
        return __awaiter(this, void 0, void 0, function* () {
            const self = this;
            const maxEpochs = 1e8;
            const eps = 0.000001;
            function sendProgress(epoch) {
                const colorProgress = self.clusters.length / targetColors * 100;
                const epochProgress = epoch / maxEpochs * 100;
                const progress = Math.floor(Math.max(colorProgress, epochProgress));
                progressCallback(Math.min(99, progress));
            }
            for (let epoch = 0; epoch < maxEpochs; epoch++) {
                // shuffle colorSet
                this.shuffle(this.colorSet);
                for (const color of this.colorSet) {
                    const closestClusterIdx = this.findClosestClusterIdx(color);
                    const closestCluster = this.clusters[closestClusterIdx];
                    // update cluster center
                    closestCluster.color.l += this.learningRate * (color.l - closestCluster.color.l);
                    closestCluster.color.a += this.learningRate * (color.a - closestCluster.color.a);
                    closestCluster.color.b += this.learningRate * (color.b - closestCluster.color.b);
                    closestCluster.winCount++;
                    if (closestCluster.winCount > this.maxWinPer) {
                        sendProgress(epoch);
                        yield new Promise((resolve) => setImmediate(resolve));
                        if (this.clusters.length >= targetColors) {
                            return;
                        }
                        const newCluster = new AduCluster();
                        newCluster.color.l = closestCluster.color.l + eps;
                        newCluster.color.a = closestCluster.color.a + eps;
                        newCluster.color.b = closestCluster.color.b + eps;
                        newCluster.winCount = 0;
                        this.clusters.push(newCluster);
                        closestCluster.winCount = 0;
                    }
                }
                sendProgress(epoch);
                yield new Promise((resolve) => setImmediate(resolve));
            }
        });
    }
    shuffle(array) {
        for (let i = array.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [array[i], array[j]] = [array[j], array[i]];
        }
    }
    /**
     *
     * @param {*} color `Oklab` object
     * @returns index of the closest cluster
     */
    findClosestClusterIdx(color) {
        let minDist = Infinity;
        let closestClusterIdx = -1;
        this.clusters.forEach((cluster, idx) => {
            const dl = cluster.color.l - color.l;
            const da = cluster.color.a - color.a;
            const db = cluster.color.b - color.b;
            const dist = dl * dl + da * da + db * db;
            if (dist < minDist) {
                minDist = dist;
                closestClusterIdx = idx;
            }
        });
        return closestClusterIdx;
    }
    /**
     *
     * @param {*} color `:Oklab` object
     * @returns color of the closest cluster
     */
    findClosestColor(color) {
        const closestClusterIdx = this.findClosestClusterIdx(color);
        return this.clusters[closestClusterIdx].color;
    }
    getPalette() {
        return this.clusters.map((cluster) => cluster.color.toSRGB().vectorize());
    }
    generatePalette(colorCount, progressCallback) {
        return __awaiter(this, void 0, void 0, function* () {
            yield this.reducePalette(colorCount, progressCallback);
            return this.getPalette();
        });
    }
    applyPalette(pixelData) {
        return __awaiter(this, void 0, void 0, function* () {
            const quantizedData = Buffer.alloc(pixelData.length);
            const chunkSize = 3000;
            for (let i = 0; i < pixelData.length; i += chunkSize) {
                const end = Math.min(i + chunkSize, pixelData.length);
                for (let j = i; j < end; j += 3) {
                    const pixel = new sRGB(pixelData[j], pixelData[j + 1], pixelData[j + 2]).toOklab();
                    const drawer = this.findClosestColor(pixel).toSRGB();
                    quantizedData[j + 0] = drawer.r;
                    quantizedData[j + 1] = drawer.g;
                    quantizedData[j + 2] = drawer.b;
                }
                yield new Promise((resolve) => setImmediate(resolve));
            }
            return quantizedData;
        });
    }
}
module.exports = { AduQuantizer };
