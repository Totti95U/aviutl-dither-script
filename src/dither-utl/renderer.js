document.getElementById("selectBtn").addEventListener("click", async () => {
    const filePath = await window.electronAPI.selectImage();
    if (filePath) {
        document.getElementById("filePath").innerText = `Selected: ${filePath}`;
    }
});

document.getElementById("convertBtn").addEventListener("click", async () => {
    const filePath = document.getElementById("filePath").innerText.split("Selected: ")[1];
    if (!filePath) return alert("Please select an image first.");

    const result = await window.electronAPI.convertImage(filePath);
    if (result.success) {
        alert(`Conversion successful! File saved at: ${result.outputFile}`);
    } else {
        alert(`Error: ${result.error}`);
    }
});
