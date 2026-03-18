#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include <iostream>
#include <algorithm>

#define ENCODER_PIN1 10
#define ENCODER_PIN2 11
#define BUTTON 12
#define LED 20

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

typedef struct {
    QueueHandle_t eventQueue;
    QueueHandle_t commandQueue;
} event_task_params;

typedef struct {
    bool set_on;
    int delay;
} event_t;

QueueHandle_t xQueueEvents, xQueueCommands;
static uint32_t last_button_press = 0;

void gpio_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int8_t event = 1;

    if (gpio == ENCODER_PIN1) {
        if (gpio_get(ENCODER_PIN2)) event = - 1;
    }
    else if (gpio == BUTTON) {
        if ((to_ms_since_boot((get_absolute_time())) - last_button_press) >= 250) {
            event = 2;
            last_button_press = to_ms_since_boot((get_absolute_time()));
        }
    }
    xQueueOverwriteFromISR(xQueueEvents, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void check_events(void *param) {
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN);
    gpio_set_pulls(BUTTON, true, false);

    gpio_init(ENCODER_PIN1);
    gpio_set_dir(ENCODER_PIN1, GPIO_IN);
    gpio_init(ENCODER_PIN2);
    gpio_set_dir(ENCODER_PIN2, GPIO_IN);

    gpio_set_irq_enabled_with_callback(ENCODER_PIN1, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BUTTON, GPIO_IRQ_EDGE_FALL, true);

    const auto *tpr = static_cast<event_task_params *>(param);

    int8_t event;
    int freq = 200;
    int previous_freq = 0;
    event_t blink_event = {false, 5};

    while (true) {
        if (xQueueReceive(tpr->eventQueue, &event, portMAX_DELAY) == pdPASS) {
            if (event == 2) {
                blink_event.set_on = !blink_event.set_on;
                xQueueSendToBack(tpr->commandQueue, &blink_event, 0);
            }
            else {
                if (blink_event.set_on) {
                    if (event == 1) freq -= 6;
                    else freq +=6;
                    freq = std::clamp(freq, 2, 200);

                    if (previous_freq != freq) {
                        printf("FREQ: %d\n", freq);
                        blink_event.delay = (1000 / freq) / 2;
                        xQueueSendToBack(tpr->commandQueue, &blink_event, 0);
                    }
                    previous_freq = freq;
                }
            }
        }
    }
}

void blink_task(void *param) {
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    const auto *que = static_cast<QueueHandle_t *>(param);

    event_t led_event = {false, 5};

    while (true) {
        gpio_put(LED, false);
        if (xQueueReceive(*que, &led_event, portMAX_DELAY) == pdPASS) {
            while (led_event.set_on) {
                gpio_put(LED, !gpio_get(LED));
                xQueueReceive(*que, &led_event, led_event.delay);
            }
        }
    }
}

int main()
{
    stdio_init_all();

    std::cout << "BOOT" << std::endl;

    event_task_params event_params;

    xQueueEvents = xQueueCreate(1, sizeof(int8_t));
    xQueueCommands = xQueueCreate(3, sizeof(event_t));

    event_params.eventQueue = xQueueEvents;
    event_params.commandQueue = xQueueCommands;

    vQueueAddToRegistry(xQueueEvents, "EVENT_QUEUE");
    vQueueAddToRegistry(xQueueCommands, "BLINK_QUEUE");

    xTaskCreate(check_events, "EVENT_HANDLING", 256, &event_params, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(blink_task, "BLINKING", 256, &xQueueCommands, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    while(1){};
}
