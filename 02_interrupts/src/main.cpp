#include <iostream>
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "semphr.h"

#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

SemaphoreHandle_t blink_sem;

struct led_params{
    uint pin;
    uint delay;
};

void read_characters(void *param) {
    while (true){
        int c = getchar_timeout_us(10);
        if (c != PICO_ERROR_TIMEOUT) {
            putchar(c);
            xSemaphoreGive(blink_sem);
        }
        else vTaskDelay(3);
    }
}

void blink_task(void *param)
{
    led_params *lpr = (led_params *) param;
    const uint led_pin = lpr->pin;
    const uint delay = lpr->delay;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    while (true) {
        if (xSemaphoreTake (blink_sem, portMAX_DELAY) == pdTRUE) {
            gpio_put(led_pin, true);
            vTaskDelay(delay);
            gpio_put(led_pin, false);
            vTaskDelay(delay);
        }
        else vTaskDelay(50);
    }
}

int main()
{
    stdio_init_all();

    std::cout << "BOOT" << std::endl;

    blink_sem = xSemaphoreCreateBinary();
    static led_params lp1 = { .pin = 20, .delay = 100 };

    xTaskCreate(read_characters, "READING", 256, (void *) nullptr, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(blink_task, "BLINKING", 256, (void *) &lp1, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    while(1){};
}
