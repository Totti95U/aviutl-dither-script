const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("electronAPI", {
    selectImage: () => ipcRenderer.invoke("select-image"),
    convertImage: (filePath) => ipcRenderer.invoke("convert-image", filePath),
});
