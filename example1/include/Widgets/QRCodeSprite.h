#ifndef QRCODE_SPRITE_H
#define QRCODE_SPRITE_H

#include <TFT_eSPI.h>

class QRCodeSprite : public TFT_eSprite {
public:
    QRCodeSprite(TFT_eSPI *tft) : TFT_eSprite(tft) {}

    void drawQrCode(const uint8_t *data, int width, int height, int spriteWidth) {
        // Рассчитываем масштаб не изменяя соотношение сторон
        int scale = spriteWidth / width;
        int scaledWidth = width * scale;
        int scaledHeight = height * scale;
        createSprite(scaledWidth, scaledHeight);
        fillSprite(TFT_WHITE);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                bool module = data[y * ((width + 7) / 8) + (x / 8)] & (1 << (7 - (x % 8)));
                if (!module) {
                    fillRect(x * scale, y * scale, scale, scale, TFT_BLACK);
                }
            }
        }

        pushSprite((TFT_WIDTH - scaledWidth) / 2, TFT_HEIGHT - scaledHeight-10);
        deleteSprite();
    }
};

#endif // QRCODE_SPRITE_H