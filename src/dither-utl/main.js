const { app, BrowserWindow, ipcMain, dialog } = require("electron");
const path = require("path");
const fs = require("fs");
const { convertToTxr } = require("./txrConverter");

let mainWindow;

app.whenReady().then(() => {
    mainWindow = new BrowserWindow({
        width: 800,
        height: 600,
        webPreferences: {
            preload: path.join(__dirname, "preload.js"),
            nodeIntegration: false,
            contextIsolation: true,
        },
    });

    mainWindow.loadFile("index.html");
});

ipcMain.handle("select-image", async () => {
    const { filePaths } = await dialog.showOpenDialog({
        properties: ["openFile"],
        filters: [{ name: "Images", extensions: ["png", "jpg", "jpeg"] }],
    });
    return filePaths[0] || null;
});

ipcMain.handle("convert-image", async (event, filePath) => {
    if (!filePath) return { success: false, error: "No file selected." };

    try {
        const outputFile = await convertToTxr(filePath); // Ensure async handling
        return { success: true, outputFile: String(outputFile) }; // Ensure it's a plain string
    } catch (error) {
        return { success: false, error: String(error.message) }; // Ensure error is a string
    }
});
