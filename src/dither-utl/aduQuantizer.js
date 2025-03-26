class AduCluster {
    constructor() {
        this.color = Array(3).fill(0);
        this.winCount = 0;
    }
}

class AduQuantizer {
    constructor() {
        this.colorSet = [];
        this.clusters = [];
        this.maxWinPer = 1 << 20; // 5 ~ 50
        this.learningRate = 0.1; // 0.01 ~ 0.5
    }

    addColor(color) {
        this.colorSet.push(color);
    }

    async addPixels(colorData) {
        let c1 = new AduCluster();
        c1.color[0] = colorData[0];
        c1.color[1] = colorData[1];
        c1.color[2] = colorData[2];
        this.addColor(c1.color);

        const chunkSize = 3000;
        for (let i = 3; i < colorData.length; i += chunkSize) {
            const end = Math.min(i + chunkSize, colorData.length);
            for (let j = i; j < end; j += 3) {
                this.addColor([colorData[j], colorData[j + 1], colorData[j + 2]]);

                c1.color[0] += colorData[j];
                c1.color[1] += colorData[j + 1];
                c1.color[2] += colorData[j + 2];
            }
            await new Promise((resolve) => setImmediate(resolve));
        }
        c1.color[0] /= colorData.length;
        c1.color[1] /= colorData.length;
        c1.color[2] /= colorData.length;
        this.clusters.push(c1);
    }

    async reducePalette(targetColors, progressCallback) {
        const self = this;
        const maxEpochs = 1e8;
        const eps = 1.0;

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
                closestCluster.color[0] += this.learningRate * (color[0] - closestCluster.color[0]);
                closestCluster.color[1] += this.learningRate * (color[1] - closestCluster.color[1]);
                closestCluster.color[2] += this.learningRate * (color[2] - closestCluster.color[2]);

                closestCluster.winCount++;
                if (closestCluster.winCount > this.maxWinPer) {
                    sendProgress(epoch);
                    await new Promise((resolve) => setImmediate(resolve))
                    if (this.clusters.length >= targetColors) {
                        return;
                    }
                    const newCluster = new AduCluster();
                    newCluster.color = [...closestCluster.color];
                    newCluster.color[0] += eps;
                    newCluster.color[1] += eps;
                    newCluster.color[2] += eps;
                    newCluster.winCount = 0;
                    this.clusters.push(newCluster);

                    closestCluster.winCount = 0;
                }
            }

            sendProgress(epoch);
            await new Promise((resolve) => setImmediate(resolve))
        }
    }

    shuffle(array) { 
        for (let i = array.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [array[i], array[j]] = [array[j], array[i]];
        }
    }

    findClosestClusterIdx(color) {
        let minDist = Infinity;
        let closestClusterIdx = null;
        this.clusters.forEach((cluster, idx) => {
            const dr = cluster.color[0] - color[0];
            const dg = cluster.color[1] - color[1];
            const db = cluster.color[2] - color[2];
            const dist = dr * dr + dg * dg + db * db;
            if (dist < minDist) {
                minDist = dist;
                closestClusterIdx = idx;
            }
        });
        return closestClusterIdx;
    }

    findClosestColor(color) {
        const closestClusterIdx = this.findClosestClusterIdx(color);
        return this.clusters[closestClusterIdx].color;
    }

    getPalette() {
        return this.clusters.map((cluster) => cluster.color);
    }

    async generatePalette(colorCount, progressCallback) {
        await this.reducePalette(colorCount, progressCallback);
        return this.getPalette();
    }

    async applyPalette(pixelData) {
        const quantizedData = Buffer.alloc(pixelData.length);
        const chunkSize = 3000;
        for (let i = 0; i < pixelData.length; i += chunkSize) {
            const end = Math.min(i + chunkSize, pixelData.length);
            for (let j = i; j < end; j += 3) {
                const [r, g, b] = this.findClosestColor([pixelData[j], pixelData[j + 1], pixelData[j + 2]]);
                quantizedData[j] = r;
                quantizedData[j + 1] = g;
                quantizedData[j + 2] = b;
            }
            await new Promise((resolve) => setImmediate(resolve));
        }

        return quantizedData;
    }
}

module.exports = { AduQuantizer };