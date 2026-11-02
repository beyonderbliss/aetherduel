game_cpp_path = "gframe/game.cpp"
with open(game_cpp_path, "r") as f:
    content = f.read()

new_resize_func = '''void Game::ResizeCardinfoWindow(bool keep_ratio) {
    // [AetherDuel] Fullscreen Left-Side Card View with Toggle & Clean Panel Coverage
    bool hide_ui = is_fullscreen;

    if (stName) stName->setVisible(!hide_ui);
    if (stInfo) stInfo->setVisible(!hide_ui);
    if (btnClearLog) btnClearLog->setVisible(!hide_ui);
    if (btnExpandLog) btnExpandLog->setVisible(!hide_ui);
    if (btnClearChat) btnClearChat->setVisible(!hide_ui);
    if (btnExpandChat) btnExpandChat->setVisible(!hide_ui);

    if (is_fullscreen) {
        // Dapatkan tinggi layar dari video driver
        irr::video::IVideoDriver* driver = env->getVideoDriver();
        int screen_height = driver ? driver->getScreenSize().Height : 720;
        int card_height = screen_height;
        int card_width = card_height * 59 / 86; // Aspek rasio kartu Yu-Gi-Oh! (59:86)

        irr::core::recti fs_rect(0, 0, card_width, card_height);
        
        if (wCardImg) {
            wCardImg->setVisible(true);
            wCardImg->setRelativePosition(fs_rect);
            wCardImg->bringToFront(); // Pindahkan kontainer ke layer paling depan
        }
        if (imgCard) {
            imgCard->setVisible(true);
            imgCard->setRelativePosition(irr::core::recti(0, 0, card_width, card_height));
            
            // Hitung manual imageScale dengan casting eksplisit ke CGUIImageButton dan namespace irr:: penuh
            irr::video::ITexture* tex = ((irr::gui::CGUIImageButton*)imgCard)->getImage();
            if (tex) {
                irr::core::dimension2du origSize = tex->getOriginalSize();
                if (origSize.Width > 0 && origSize.Height > 0) {
                    irr::f32 scaleX = (irr::f32)card_width / (irr::f32)origSize.Width;
                    irr::f32 scaleY = (irr::f32)card_height / (irr::f32)origSize.Height;
                    ((irr::gui::CGUIImageButton*)imgCard)->setImageScale(irr::core::vector2df(scaleX, scaleY));
                }
            }
            imgCard->bringToFront(); // Pindahkan tombol gambar ke layer paling depan di atas UI
        }
    } else {
        // Mode Default EDOPro
        const auto rect = keep_ratio ?
            ResizeWithCappedWidth(1, 1, 1 + CARD_IMG_WRAPPER_WIDTH, 1 + CARD_IMG_WRAPPER_HEIGHT, CARD_IMG_WRAPPER_ASPECT_RATIO) :
            Resize(1, 1, 1 + CARD_IMG_WIDTH + 20, 1 + CARD_IMG_HEIGHT + 18);
        if (wCardImg) {
            wCardImg->setVisible(true);
            wCardImg->setRelativePosition(rect);
        }
        if (imgCard) {
            imgCard->setVisible(true);
            imgCard->setRelativePosition(irr::core::recti(10, 9, 10 + CARD_IMG_WIDTH, 9 + CARD_IMG_HEIGHT));
            ((irr::gui::CGUIImageButton*)imgCard)->setImageScale(irr::core::vector2df(1.0f, 1.0f)); // Kembalikan skala normal
        }
    }
}'''

import re
pattern = r'void\s+Game::ResizeCardinfoWindow\s*\([^)]*\)\s*\{.*?\}(?=\n\w|\Z)'

if re.search(pattern, content, re.DOTALL):
    content = re.sub(pattern, new_resize_func, content, count=1, flags=re.DOTALL)
    with open(game_cpp_path, "w") as f:
        f.write(content)
    print("[SUKSES] ResizeCardinfoWindow berhasil diperbarui!")
else:
    print("[ERROR] Gagal menemukan fungsi ResizeCardinfoWindow di game.cpp.")
