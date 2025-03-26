const { app, BrowserWindow, ipcMain, dialog } = require("electron");
const fs = require("fs");
const path = require("path");
const sharp = require("sharp");
// const { OctreeQuantizer } = require("./octreeQuantizer");
const { AduQuantizer } = require("./aduQuantizer");

let mainWindow;

app.whenReady().then(() => {
    mainWindow = new BrowserWindow({
        width: 800,
        height: 600,
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            enableRemoteModule: false,
            preload: path.join(__dirname, "preload.js"),
        },
    });

    mainWindow.setMenuBarVisibility(false);
    mainWindow.loadFile("index.html");
    // mainWindow.webContents.openDevTools();
});

ipcMain.handle("select-image", async () => {
    const { filePaths } = await dialog.showOpenDialog({ 
        properties: ["openFile"], 
        filters: [{ name: "Images", extensions: ["png", "jpg", "jpeg"] }] 
    });

    return filePaths[0] || null;
});

ipcMain.handle("convert-image", async (event, filePath) => {
    try {
        const outputFile = filePath.replace(/\.\w+$/, ".txr");

        // Read the image
        const { data, info } = await sharp(filePath)
            .removeAlpha() // Ensure no transparency
            .raw()
            .toBuffer({ resolveWithObject: true });

        const { width, height } = info;

        // Open file and write TXR data
        const fileStream = fs.createWriteStream(outputFile);
        fileStream.write(Buffer.from([width & 0xff, width >> 8, height & 0xff, height >> 8])); // Store width and height

        for (let i = 0; i < data.length; i += 3) {
            fileStream.write(Buffer.from([data[i], data[i + 1], data[i + 2]]));
        }
        
        fileStream.end();

        return { success: true, outputFile };
    } catch (error) {
        console.error("Error in TXR conversion:", error);
        return { success: false, error: error.message };
    }
});

ipcMain.handle("generate-palette", async (event, filePath, colorCount) => {
    try {
        const decodedPath = decodeURIComponent(filePath);

        const { data, info } = await sharp(decodedPath)
            .removeAlpha()
            .raw()
            .toBuffer({ resolveWithObject: true });

        const progressCallback = (progress) => {
            event.sender.send("palette-progress", progress);
        };

        const quantizer = new AduQuantizer();
        await quantizer.addPixels(data);
        await quantizer.generatePalette(colorCount, progressCallback);
        progressCallback(100); // Ensure it reaches 100%
        event.sender.send("palette-applying", true)

        const quantizedData = await quantizer.applyPalette(data);
        const quantizedBuffer = await sharp(Buffer.from(quantizedData), {
            raw: { width: info.width, height: info.height, channels: 3 },
        })
        .toFormat("png")
        .toBuffer();
        event.sender.send("palette-applying", false);

        return {
            success: true,
            quantizedImage: `data:image/png;base64,${quantizedBuffer.toString("base64")}`,
            palette: quantizer.getPalette(),
        };
    } catch (error) {
        console.error("Error generating quantized image:", error);
        return { success: false, error: error.message };
    }
});

