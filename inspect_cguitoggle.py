import os

# Cari file header CGUIImageButton
header_path = None
for root, dirs, files in os.walk("gframe"):
    for file in files:
        if file.lower() == "cguiimagebutton.h":
            header_path = os.path.join(root, file)
            break

if header_path:
    print(f"[INFO] Ditemukan header: {header_path}")
    with open(header_path, "r") as f:
        content = f.read()
    # Tampilkan bagian deklarasi fungsi atau member variabel
    for line in content.splitlines():
        if "Image" in line or "image" in line or "setImage" in line:
            print(line)
else:
    print("[ERROR] File CGUIImageButton.h tidak ditemukan di folder gframe.")
