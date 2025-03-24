const fs = require("fs");
const path = require("path");
const sharp = require("sharp");

async function convertToTxr(inputFile) {
    return new Promise(async (resolve, reject) => {
        try {
            const outputFile = path.join(
                path.dirname(inputFile),
                path.basename(inputFile, path.extname(inputFile)) + ".txr"
            );

            const { data, info } = await sharp(inputFile)
                .raw()
                .toBuffer({ resolveWithObject: true });

            const width = info.width;
            const height = info.height;

            const buffer = Buffer.alloc(4 + width * height * 3);
            buffer.writeUInt16LE(width, 0);
            buffer.writeUInt16LE(height, 2);

            for (let i = 0; i < width * height; i++) {
                buffer[4 + i * 3] = data[i * 3];
                buffer[4 + i * 3 + 1] = data[i * 3 + 1];
                buffer[4 + i * 3 + 2] = data[i * 3 + 2];
            }

            fs.writeFileSync(outputFile, buffer);
            resolve(outputFile); // Ensure it only returns a string
        } catch (error) {
            reject(error);
        }
    });
}


module.exports = { convertToTxr };
