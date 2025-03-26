const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("electronAPI", {
    selectImage: () => ipcRenderer.invoke("select-image"),
    convertImage: (filePath) => ipcRenderer.invoke("convert-image", filePath),
    generatePalette: (filePath, colorCount) => ipcRenderer.invoke("generate-palette", filePath, colorCount),
    onPaletteProgress: (callback) => ipcRenderer.on("palette-progress", (event, progress) => callback(progress)),
    onPaletteApplying: (callback) => ipcRenderer.on("palette-applying", (event, applying) => callback(applying))
});
