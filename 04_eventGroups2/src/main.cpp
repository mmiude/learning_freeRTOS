#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "queue.h"
#include <iostream>
#include <ctime>

#include "event_groups.h"
#include "hardware/timer.h"

#define SW_0 9
#define SW_1 8
#define SW_2 7

#define EVENT_BIT_0 (1UL << 0UL)
#define EVENT_BIT_1 (1UL << 1UL)
#define EVENT_BIT_2 (1UL << 2UL)
#define NUM_EVENT_BITS 3

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

EventGroupHandle_t eventG;
QueueHandle_t syslog_q;

struct debugEvent {
    const char *format;
    uint32_t data[3];
    TickType_t timeStamp;
};


void debug(const char *format, uint32_t d1, uint32_t d2, uint32_t d3) {

    debugEvent event = {
        .format = format,
        .data = {d1, d2, d3},
        .timeStamp = xTaskGetTickCount()
    };

    if (xQueueSendToBack(syslog_q, &event, 10) == pdPASS);
}

void debugTask(void *param) {

    char buffer[64];
    debugEvent e;

    while (true) {
        if (xQueueReceive(syslog_q, &e, portMAX_DELAY)) {
            snprintf(buffer, 64, e.format, e.data[0], e.data[1], e.data[2]);
            std::cout << "[time stamp: " << e.timeStamp << " ticks] ";
            std::cout << buffer;
        }
    }
}

void button_SW_0_task(void *param) { //sets 0 bit
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);

    while (true) {
        if (!gpio_get(SW_0)) {
            while (!gpio_get(SW_0)) {
                vTaskDelay(10);
            }
            xEventGroupSetBits(eventG, EVENT_BIT_0);
            debug("Button %d pressed\n", 0, 0,0);
        }
        else vTaskDelay(50);
    }
}

void button_SW_1_task(void *param) { // sets 1 bit

    gpio_set_dir(SW_1, GPIO_IN);
    gpio_pull_up(SW_1);

    while (true) {
        if (!gpio_get(SW_1)) {
            while (!gpio_get(SW_1)) {
                vTaskDelay(10);
            }
            xEventGroupSetBits(eventG, EVENT_BIT_1);
            debug("Button %d pressed\n", 1, 0,0);
        }
        else vTaskDelay(50);
    }
}

void button_SW_2_task(void *param) { // sets 2 bit

    gpio_set_dir(SW_2, GPIO_IN);
    gpio_pull_up(SW_2);

    while (true) {
        if (!gpio_get(SW_2)) {
            while (!gpio_get(SW_2)) {
                vTaskDelay(10);
            }
            xEventGroupSetBits(eventG, EVENT_BIT_2);
            debug("Button %d pressed\n", 2, 0,0);
        }
        else vTaskDelay(50);
    }
}

void watchdog_task(void *param) {

    EventBits_t bits_set;
    TickType_t last_ok = 0;

    while (true) {

        bits_set = xEventGroupWaitBits(eventG, (EVENT_BIT_0 | EVENT_BIT_1 | EVENT_BIT_2), pdTRUE, pdTRUE, 10000);

        if (bits_set == (EVENT_BIT_0 | EVENT_BIT_1 | EVENT_BIT_2)) {
            debug("[OK] Ticks since last ok: %d\n", xTaskGetTickCount() - last_ok, 0, 0);
            last_ok = xTaskGetTickCount();
        }
        else {
            for (int i = 0; i < NUM_EVENT_BITS; i++) {
                if (!(bits_set & (1 << i))) {
                    debug("[FAIL] Task: %d\n", i, 0, 0);
                }
            }
            debug("[SUSPENDED]\n", 0, 0, 0);
            vTaskSuspend(xTaskGetCurrentTaskHandle());
        }
    }
}


int main()
{

    stdio_init_all();

    std::cout << "BOOT" << std::endl;

    syslog_q = xQueueCreate(10, sizeof(debugEvent));
    eventG = xEventGroupCreate();
    vQueueAddToRegistry(syslog_q, "SYSLOG");

    xTaskCreate(debugTask, "DEBUG", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(button_SW_0_task, "BUTTON", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(button_SW_1_task, "PRINT2", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(button_SW_2_task, "PRINT3", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(watchdog_task, "WATCHDOG", 256, NULL, tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();

    while(1){};
}
