#ifndef START_X_TASK_H
#define START_X_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Services/BaseService.h"

// Объявление функции startXTask, которая принимает сервис и контекст, а также параметры задачи
template <typename ServiceType, typename ContextType>
void startXTask(ServiceType &service, ContextType &context, const char *taskName,
                uint16_t stackSize = 2048, UBaseType_t taskPriority = 1)
{
    xTaskCreate(
        [](void *pvParameters)
        {
            // Получаем указатели на сервис и контекст
            ServiceType *service = static_cast<ServiceType *>(static_cast<void **>(pvParameters)[0]);
            ContextType *context = static_cast<ContextType *>(static_cast<void **>(pvParameters)[1]);

            // Инициализируем сервис
            service->init();

            // Получаем интервал обновления для сервиса
            const TickType_t ticks = pdMS_TO_TICKS(service->getUpdateInterval());

            while (true)
            {
                // Обновляем сервис с указанным контекстом
                service->update(*context);

                // Ожидаем до следующего обновления
                vTaskDelay(ticks);
            }
        },
        taskName,
        stackSize,
        new void *[2]{&service, &context}, // Упакуем сервис и контекст в массив void* для передачи в задачу
        taskPriority,
        NULL // Указатель на TaskHandle_t не нужен
    );
}

#endif // START_X_TASK_H