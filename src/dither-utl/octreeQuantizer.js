class OctreeNode {
    constructor(level, parent) {
        this.level = level;
        this.parent = parent;
        this.pixelCount = 0;
        this.red = 0;
        this.green = 0;
        this.blue = 0;
        this.children = [];
        this.index = null;
    }

    isLeaf() {
        return this.children.length === 0;
    }

    addColor(color, level, quantizer) {
        this.pixelCount++;
        this.red += color[0];
        this.green += color[1];
        this.blue += color[2];

        if (level < 8) {
            const index = ((color[0] >> (7 - level)) & 1) << 2 |
                          ((color[1] >> (7 - level)) & 1) << 1 |
                          ((color[2] >> (7 - level)) & 1);

            if (!this.children[index]) {
                this.children[index] = new OctreeNode(level + 1, this);
                quantizer.addToLevel(level + 1, this.children[index]);
            }
            this.children[index].addColor(color, level + 1, quantizer);
        }
    }

    merge() {
        if (!this.isLeaf()) {
            this.pixelCount = 0;
            this.red = 0;
            this.green = 0;
            this.blue = 0;

            for (const child of this.children) {
                if (child) {
                    this.pixelCount += child.pixelCount;
                    this.red += child.red;
                    this.green += child.green;
                    this.blue += child.blue;
                }
            }
            this.children = [];
        }
    }

    getColor() {
        return [
            Math.round(this.red / this.pixelCount),
            Math.round(this.green / this.pixelCount),
            Math.round(this.blue / this.pixelCount)
        ];
    }
}

class OctreeQuantizer {
    constructor() {
        this.root = new OctreeNode(0, null);
        this.levels = Array.from({ length: 9 }, () => []);
        this.addToLevel(0, this.root);
    }

    addToLevel(level, node) {
        this.levels[level].push(node);
    }

    async addPixels(pixelData) {
        // TODO: delete bellow line
        const startTime = performance.now();

        const chunkSize = 3000; // Process 3000 bytes (~1000 pixels) per chunk.
        for (let i = 0; i < pixelData.length; i += chunkSize) {
            const end = Math.min(i + chunkSize, pixelData.length);
            for (let j = i; j < end; j += 3) {
                const color = [pixelData[j], pixelData[j + 1], pixelData[j + 2]];
                this.root.addColor(color, 0, this);
            }
            await new Promise((resolve) => setImmediate(resolve)); // Yield after processing each chunk.
        }
        // TODO: delete bellow line
        console.log("Time to add pixels:", (performance.now() - startTime) / 1000, "seconds");
    }

    async reducePalette(targetColors, progressCallback) {
        // TODOL: delete bellow line
        const startTime = performance.now();

        const initialPaletteSize = this.getPalette().length;
        let mergeableNodes = this.collectMergeableNodes();
        
        while (mergeableNodes.length > 0 && this.getPalette().length > targetColors) {
            // Sort mergeable nodes by pixel count, merge the smallest
            mergeableNodes.sort((a, b) => a.pixelCount - b.pixelCount);
            const nodeToMerge = mergeableNodes.shift();
            nodeToMerge.parent.merge();
            
            // Recalculate mergeable nodes
            mergeableNodes = this.collectMergeableNodes();
            
            // Progress callback
            const progress = Math.round(((initialPaletteSize - this.getPalette().length) / (initialPaletteSize - targetColors)) * 100);
            progressCallback(Math.min(99, progress));
        }
        progressCallback(100);
        // TODO: delete bellow line
        console.log("Time to reduce palette:", (performance.now() - startTime) / 1000, "seconds");
    }
    
    collectMergeableNodes() {
        const mergeableNodes = [];
        for (const level of this.levels) {
            for (const node of level) {
                if (!node.isLeaf() && node.parent && node.parent.isLeaf()) {
                    mergeableNodes.push(node);
                }
            }
        }
        return mergeableNodes;
    }

    getPalette() {
        const palette = [];
        for (const level of this.levels) {
            for (const node of level) {
                if (node.isLeaf()) {
                    palette.push(node.getColor());
                }
            }
        }
        return palette;
    }

    async generatePalette(colorCount, progressCallback) {
        await this.reducePalette(colorCount, progressCallback);
        return this.getPalette();
    }

    async applyPalette(pixelData) {
        const palette = this.getPalette();

        // Find the closest color in the palette using inline squared differences.
        function findClosestColor(r, g, b) {
            let minDist = Infinity;
            let closestColor = null;
            for (const [pr, pg, pb] of palette) {
                const dr = pr - r;
                const dg = pg - g;
                const db = pb - b;
                const dist = dr * dr + dg * dg + db * db;
                if (dist < minDist) {
                    minDist = dist;
                    closestColor = [pr, pg, pb];
                }
            }
            return closestColor;
        }

        const quantizedData = Buffer.alloc(pixelData.length);
        const chunkSize = 3000; // Process chunk per 3000 bytes.
        for (let i = 0; i < pixelData.length; i += chunkSize) {
            const end = Math.min(i + chunkSize, pixelData.length);
            for (let j = i; j < end; j += 3) {
                const [r, g, b] = findClosestColor(pixelData[j], pixelData[j + 1], pixelData[j + 2]);
                quantizedData[j] = r;
                quantizedData[j + 1] = g;
                quantizedData[j + 2] = b;
            }
            await new Promise((resolve) => setImmediate(resolve));
        }

        return quantizedData;
    }
}

module.exports = { OctreeQuantizer };