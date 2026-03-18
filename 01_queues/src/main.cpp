#include <iostream>
#include <vector>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/stdlib.h"

#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

struct b_task_params {
    QueueHandle_t comm_q;
    uint pin_nr;
    uint8_t id;
};

struct lock_task_params {
    QueueHandle_t comm_q;
    uint pin_nr;
    uint delay;
};

void checkButtonPress(void *param){
    b_task_params *tpr = (b_task_params *) param;
    const uint button_pin = tpr->pin_nr;
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_pull_up(button_pin);
    uint8_t id = tpr->id;

    vTaskDelay(500);

    while (true) {
        if (!gpio_get(button_pin)) {
            if (xQueueSendToBack(tpr->comm_q, &id, 0) == pdPASS) {
                //printf("button pressed. value %d added to queue...\n", id);
                while (!gpio_get(button_pin)) {
                    vTaskDelay(20);
                }
            }
        }
        vTaskDelay(50);
    }

};

void lockProcessing(void *param) {
    lock_task_params *tpr = static_cast<lock_task_params *>(param);
    const uint led_pin = tpr->pin_nr;
    const uint delay = tpr->delay;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    static const uint8_t lock_sequence[] = {0, 0, 2, 1, 2};
    int index = 0;
    uint8_t value = 0;

    while (true) {
        if (xQueueReceive(tpr->comm_q, &value, delay) == pdTRUE) {
            //printf("here we are. value: %d, index: %d\n", value, index);
            if (value == lock_sequence[index]) {
                //printf("correct!\n");
                if (index == sizeof(lock_sequence) - 1) {
                    //printf("locked opened...\n");
                    gpio_put(led_pin, true);
                    xQueueReceive(tpr->comm_q, &value, delay);
                    gpio_put(led_pin, false);
                    index = 0;
                }
                else ++index;
            }
        } else index = 0;
    }
}



int main()
{
    stdio_init_all();

    std::cout << "BOOT" << std::endl;

    static b_task_params b1 = {.pin_nr = 7, .id = 2};
    static b_task_params b2 = {.pin_nr = 8, .id = 1};
    static b_task_params b3 = {.pin_nr = 9, .id = 0};
    static lock_task_params lock_params = {.pin_nr = 20, .delay = 5000};

    b1.comm_q = xQueueCreate(5, sizeof(uint8_t));
    b2.comm_q = b1.comm_q;
    b3.comm_q = b1.comm_q;
    lock_params.comm_q = b1.comm_q;

    vQueueAddToRegistry(b1.comm_q, "BTN_PRESS");

    xTaskCreate(checkButtonPress, "CHECK1", 512, (void*) &b1, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(checkButtonPress, "CHECK2", 512, (void*) &b2, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(checkButtonPress, "CHECK3", 512, (void*) &b3, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(lockProcessing, "PROCESS", 512, (void*) &lock_params, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    while(1){};
}
