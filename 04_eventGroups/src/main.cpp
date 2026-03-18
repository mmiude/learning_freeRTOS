#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "queue.h"
#include <iostream>
#include <ctime>

#include "event_groups.h"
#include "hardware/timer.h"

#define BUTTON 9

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

void main_print(int task_nr, uint seed) {


    srand(time(NULL));
    TickType_t last_print = 0;

    while (true) {
        last_print  = xTaskGetTickCount() - last_print;
        debug("Task number: %d. Time %d\n", task_nr, last_print, 0);
        last_print = xTaskGetTickCount();
        vTaskDelay(pdMS_TO_TICKS((rand_r(&seed) % 1000) + 1000));
    }
}

void debugTask(void *param) {

    char buffer[64];
    debugEvent e;

    while (true) {
        if (xQueueReceive(syslog_q, &e, portMAX_DELAY)) {
            snprintf(buffer, 64, e.format, e.data[0], e.data[1], e.data[2]);
            std::cout << "[time stamp: " << e.timeStamp <<  " ticks] ";
            std::cout << buffer;
        }
    }
}

void button_task(void *param) {
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN);
    gpio_pull_up(BUTTON);
    bool button_pressed = false;
    uint seed = 1;

    while (true) {
        if (!gpio_get(BUTTON)) {
            xEventGroupSetBits(eventG, (1UL << 0UL));
            button_pressed = true;
            while (!gpio_get(BUTTON)) {
                vTaskDelay(10);
            }
        }
        else {
            vTaskDelay(50);
            if (button_pressed) main_print(1, seed);
        }
    }
}

void task_2(void *param) {

    //TickType_t last_print = 0;
    uint seed = 2;

    xEventGroupWaitBits(eventG, (1UL << 0UL), pdTRUE, pdTRUE, portMAX_DELAY);
    main_print(2, seed);
    /*while (true) {

        last_print  = xTaskGetTickCount() - last_print;
        debug("Task number: %d. Time %d\n", 2, last_print, 0);
        last_print = xTaskGetTickCount();
        vTaskDelay(pdMS_TO_TICKS((rand_r(&seed) % 1000) + 1000));
    }*/
}

void task_3(void *param) {

    //TickType_t last_print = 0;
    uint seed = 3;

    xEventGroupWaitBits(eventG, (1UL << 0UL), pdTRUE, pdTRUE, portMAX_DELAY);
    main_print(3, seed);

    /*while (true) {

        debug("Task number: %d. Time: %d\n", 3, xTaskGetTickCount() - last_print, 0);
        last_print = xTaskGetTickCount();
        vTaskDelay(pdMS_TO_TICKS((rand_r(&seed) % 1000) + 1000));
    }*/
}


int main()
{

    stdio_init_all();

    std::cout << "BOOT" << std::endl;

    syslog_q = xQueueCreate(10, sizeof(debugEvent));

    eventG = xEventGroupCreate();

    vQueueAddToRegistry(syslog_q, "SYSLOG");

    xTaskCreate(debugTask, "DEBUG", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(button_task, "BUTTON", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(task_2, "PRINT2", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(task_3, "PRINT3", 256, NULL, tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();

    while(1){};
}
