#include "Services/DisplayService.h"
#include "Widgets/StatusBarWidget.h" // и другие виджеты, если они есть
#include "Widgets/RxWidget.h"        // и другие виджеты, если они есть
#include <globals.h>
#include <boot.h>
#include <Fonts/FontDejaVu20.h>
uint8_t currFrame = 0;
const uint16_t *_compressedImages[] = {_boot0_compressed, _boot1_compressed, _boot2_compressed, _boot3_compressed, _boot4_compressed, _boot5_compressed, _boot6_compressed, _boot7_compressed, _boot8_compressed};
// Буфер для разжатых изображений
uint16_t decompressedImage[100 * 100];

// Конструктор класса DisplayService с инициализацией состояния отображения
DisplayService::DisplayService(TFT_eSPI *display, RSSICalibrationData *rssisCallibrationData)
    : tft(display), currentState(DisplayState::BOOT), rssisCallibrationData{rssisCallibrationData}
{
    lastChange = millis(); // Сохраняем текущее время для отсчета таймера
}

// Оптимизированная функция разжатия данных RLE
void decompressImage(const uint16_t *compressed, uint16_t *decompressed, size_t decompressedSize)
{
    size_t index = 0;
    for (size_t i = 0; i < decompressedSize * 2; i += 2)
    {
        uint16_t value = pgm_read_word_near(&compressed[i]);
        uint16_t count = pgm_read_word_near(&compressed[i + 1]);
        if (index + count <= decompressedSize)
        {
            memset(decompressed + index, value, count * sizeof(uint16_t));
            index += count;
        }
    }
}

void DisplayService::init()
{
    tft->init();
    tft->fillScreen(TFT_BLACK);

    // Инициализация виджетов и добавление их в вектор
    widgets.push_back(new StatusBarWidget(tft)); // Создаем статус-бар виджет
    uint8_t totalModules = RADIO_1_2G_ENABLED + RADIO_2_4G_ENABLED + RADIO_5_8G_ENABLED;
    Serial.println("MODULES STATE");
    Serial.println(RADIO_1_2G_ENABLED);
    Serial.println(RADIO_2_4G_ENABLED);
    Serial.println(RADIO_5_8G_ENABLED);
    Serial.println("------------");
#if RADIO_1_2G_ENABLED
    widgets.push_back(new RxWidget(tft, [](const DisplayContext &context) -> FrequencyRange
                                   {
                                       return context.range_1_2; // Возвращаем нужный массив RSSI для 1.2 GHz
                                   },
                                   "1.2G", totalModules, rssisCallibrationData->band_1_2));
#endif

#if RADIO_2_4G_ENABLED
    widgets.push_back(new RxWidget(tft, [](const DisplayContext &context) -> FrequencyRange
                                   {
                                       return context.range_2_4; // Возвращаем нужный массив RSSI для 2.4 GHz
                                   },
                                   "2.4G", totalModules, rssisCallibrationData->band_2_4));
#endif

#if RADIO_5_8G_ENABLED
    widgets.push_back(new RxWidget(tft, [](const DisplayContext &context) -> FrequencyRange
                                   {
                                       return context.range_5_8; // Возвращаем нужный массив RSSI для 5.8 GHz
                                   },
                                   "5.8G", totalModules, rssisCallibrationData->band_5_8));
#endif

    showBootScreen();
}

void DisplayService::update(DisplayContext &context)
{

    if (currentState == DisplayState::BOOT)
    {
        unsigned long currentMillis = millis();
        // Показываем экран загрузки
        if (currFrame < BOOT_IMAGES_FRAMES)
        {
            if ((currentMillis - lastChange) >= bootFrameInterval)
            {
                // Распаковываем изображение
                decompressImage(_compressedImages[currFrame], decompressedImage, 100 * 100);
                // Отображаем изображение
                tft->pushImage(70, TFT_HEIGHT - 145, 100, 100, decompressedImage);
                lastChange = currentMillis;
                currFrame++;
            }
        }
        else
        {
            if ((currentMillis - lastChange) >= bootInterval)
            {
                tft->fillScreen(TFT_BLACK);
                currentState = DisplayState::MAIN; // Переключаемся на основной экран
            }
        }
    }
    else if (currentState == DisplayState::MAIN)
    {
        // Отображение основного экрана
        for (auto &widget : widgets)
        {
            widget->update(context);
            widget->draw();
        }
    }
}

// Показываем экран загрузки
void DisplayService::showBootScreen()
{
    tft->fillScreen(TFT_BLACK);
    // Отображение экрана загрузки
    tft->fillRect(0, 0, TFT_WIDTH, 5, 0x047f);
    tft->fillRect(0, 5, TFT_WIDTH, 10, 0x10a2);
    tft->fillRect(0, 15, TFT_WIDTH, 10, 0x047f);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextSize(2);
    tft->drawString(APP_VERSION, 0, TFT_HEIGHT - 20, 1);
    tft->loadFont(FontDejaVu20);
    tft->drawString("Версия ПО:", 0, TFT_HEIGHT - 40);
    tft->drawCentreString("Создано в России", TFT_WIDTH / 2, 35, 2);
    tft->drawCentreString("Сканер частот", TFT_WIDTH / 2, 95, 2);
    tft->drawCentreString("ФИЛИН", TFT_WIDTH / 2, 125, 3);

    tft->fillRect((TFT_WIDTH - 45) / 2, 60, 45, 10, TFT_WHITE);
    tft->fillRect((TFT_WIDTH - 45) / 2, 70, 45, 10, TFT_RED);
    tft->fillRect((TFT_WIDTH - 45) / 2, 80, 45, 10, TFT_BLUE);
}
