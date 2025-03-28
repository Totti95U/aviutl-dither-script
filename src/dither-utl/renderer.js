const dropArea = document.getElementById("drop-area");
const dropText = document.getElementById("drop-text");
const imagePreview = document.getElementById("image-preview");
const filePathText = document.getElementById("filePath");
const colorCountInput = document.getElementById("colorCount");
const convertBtn = document.getElementById("convertBtn");
const generatePaletteBtn = document.getElementById("generatePaletteBtn");
const copyPaletteBtn = document.getElementById("copyPaletteBtn");
const copyNotification = document.getElementById("copyNotification");
const paletteProgress = document.getElementById("paletteProgress");

const preventDefault = (e) => {
    e.preventDefault();
    e.stopPropagation();
};

dropArea.addEventListener("dragover", (event) => {
    preventDefault(event);
    dropArea.classList.add("dragover");
});

dropArea.addEventListener("dragleave", () => {
    dropArea.classList.remove("dragover");
});

dropArea.addEventListener("drop", async (event) => {
    preventDefault(event);
    dropArea.classList.remove("dragover");

    const file = window.electronAPI.showFilePath(event.dataTransfer.files[0]);
    if (file.path && file.type.startsWith("image/")) {
        loadImage(file);
    } else {
        alert("Please drop a valid image file.");
    }
});

dropArea.addEventListener("click", async () => {
    const filePath = await window.electronAPI.selectImage();
    if (filePath) {
        loadImage(filePath);
    }
});

function loadImage(file) {
    const filePath = file.path;
    filePathText.innerText = `Selected: ${filePath}`;
    imagePreview.src = `file://${filePath}`;
    imagePreview.style.display = "block";
    dropText.style.display = "none";
    convertBtn.disabled = false;
    generatePaletteBtn.disabled = false;
    copyPaletteBtn.disabled = true;
}

convertBtn.addEventListener("click", async () => {
    const filePath = filePathText.innerText.split("Selected: ")[1];
    if (!filePath) return alert("Please select an image first.");

    const result = await window.electronAPI.convertImage(filePath);
    if (result.success) {
        alert(`Conversion successful! File saved at: ${result.outputFile}`);
    } else {
        alert(`Error: ${result.error}`);
    }
});

let animationInterval = null;

function startButtonAnimation(button, baseText) {
    let dots = 0;
    animationInterval = setInterval(() => {
        dots = (dots + 1) % 4; // Cycle between 0, 1, 2, 3
        button.innerHTML = baseText + ".".repeat(dots) + "&nbsp;".repeat(3 - dots);
    }, 500); // Update every 500ms
}

function stopButtonAnimation(button, defaultText) {
    clearInterval(animationInterval);
    button.innerText = defaultText;
}

window.electronAPI.onPaletteApplying((applying) => {
    if (applying) {
        stopButtonAnimation(generatePaletteBtn, "Generate Palette");
        startButtonAnimation(generatePaletteBtn, "Applying Palette");
    } else {
        stopButtonAnimation(generatePaletteBtn, "Generate Palette");
        generatePaletteBtn.disabled = false;
    }
});

let generatePalette = []; // Store the generated palette

generatePaletteBtn.addEventListener("click", async () => {
    const colorCount = parseInt(document.getElementById("colorCount").value, 10);
    const filePathRaw = filePathText.innerText;
    const filePath = filePathRaw.startsWith("Selected: ") ? filePathRaw.substring("Selected: ".length) : filePathRaw;
    
    if (!filePath || filePath === "No image selected") {
        alert("Please select an image first.");
        return;
    }

    generatePaletteBtn.disabled = true;
    paletteProgress.style.display = "block";
    paletteProgress.value = 0;
    startButtonAnimation(generatePaletteBtn, "Generating");
    try {
        const response = await window.electronAPI.generatePalette(filePath, colorCount);
        if (response.success) {
            // Store the palette in the global variable
            generatedPalette = response.palette;
            copyPaletteBtn.disabled = false; // Enable the copy button
            imagePreview.src = response.quantizedImage;
        } else {
            alert("Failed to generate palette: " + response.error);
        }
    } catch (error) {
        console.error("Error generating palette:", error);
    }
});

window.electronAPI.onPaletteProgress((progress) => {
    const paletteProgress = document.getElementById("paletteProgress");
    paletteProgress.value = progress;
    if (progress >= 100) {
        setTimeout(() => { paletteProgress.value = 0; }, 500);
    }
});

copyPaletteBtn.addEventListener("click", () => {
    if (generatedPalette.length === 0) {
        alert("No palette to copy. Please generate a palette first.");
        return;
    }

    // Format the palette as {0xaabbcc, 0x112233, ...}
    const paletteText = `{${generatedPalette.map(([r, g, b]) => 
        `0x${((r << 16) | (g << 8) | b).toString(16).padStart(6, "0")}`
    ).join(", ")}}`;

    navigator.clipboard.writeText(paletteText)
        .then(() => {
            // show the notification
            copyNotification.style.visibility = "visible";
            copyNotification.style.opacity = 1;

            // Hide the notification after 2 seconds
            setTimeout(() => {
                copyNotification.style.opacity = 0;
                setTimeout(() => { copyNotification.style.visibility = "hidden";}, 300);
            }, 2000);
        })
        .catch(err => console.error("Failed to copy palette: ", err));
});
