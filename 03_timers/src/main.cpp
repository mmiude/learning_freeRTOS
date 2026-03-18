#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "timers.h"
#include "projdefs.h"

#include "hardware/timer.h"
#include "PicoOsUart.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <iomanip>

#define LED 20
#define MAX_BUFFER_SIZE 64
#define DEFAULT_LED_DELAY 5

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

TimerHandle_t toggleTimer, inactivityTimer;
QueueHandle_t command_q;

struct command_struct{
    char line[MAX_BUFFER_SIZE];
};

struct toggle_id {
    TickType_t time;
};


void inactivity_callback(TimerHandle_t xTimer) {
    printf("[Inactive]\n");
}

void toggle_callback(TimerHandle_t xTimer) {
    gpio_put(LED, !gpio_get(LED));

}


void serial_task(void *param)
{
    xTimerStart(inactivityTimer, 30);

    PicoOsUart u(0, 0, 1, 115200);
    uint8_t buffer[MAX_BUFFER_SIZE];
    int char_count = 0;
    command_struct command;

    while (true) {
        if(int count = u.read(buffer, MAX_BUFFER_SIZE, 30); count > 0) {
            xTimerReset(inactivityTimer, 0);
            u.write(buffer, count);
            for (int i = 0; i < count; i++) {
                if (buffer[i] == '\n' || buffer[i] == '\r') {
                    command.line[char_count] = '\0';
                    xQueueSendToBack(command_q, &command, 0);
                    char_count = 0;
                    break;
                }
                if (char_count < MAX_BUFFER_SIZE) command.line[char_count++] = buffer[i];
            }
        } else if (xTimerIsTimerActive(inactivityTimer) == pdFALSE) {
            printf("flushing...\n");
            u.flush();
            char_count = 0;
        }
    }
}

void command_interpreter(void *param) {

    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    xTimerStart(toggleTimer, 30);
    command_struct command;
    std::stringstream ss;
    float delay = DEFAULT_LED_DELAY;
    std::string cmd;

    while (true) {
        if (xQueueReceive(command_q, &command, portMAX_DELAY)) {
            std::string cmd(command.line);
            std::stringstream ss(command.line);

            if (cmd == "help") {
                std::cout << "***** HELP *****" << std::endl;
                std::cout << "type \"interval <number>\" to set new toggle interval\n" << std::endl;
                std::cout << "type \"time\" to receive last toggle time\n" << std::endl;
            }
            else if (cmd == "time") {
                float last_toggle_time_s = (xTimerGetPeriod(toggleTimer) - (xTimerGetExpiryTime(toggleTimer) - xTaskGetTickCount())) / 1000.0;
                std::cout << "last toggle: " << std::fixed << std::setprecision(1) << last_toggle_time_s << std::endl;
            }
            else if (ss >> cmd >> delay && cmd == "interval") {
                xTimerChangePeriod(toggleTimer, delay * 1000, 0);
                //std::cout << "interval changed: " << delay << std::endl;
            }
            else printf("UNKNOWN COMMAND\n");
        }
    }
}


int main()
{
    stdio_init_all();

    std::cout << "BOOT" << std::endl;

    command_q = xQueueCreate(5, sizeof(command_struct));

    inactivityTimer = xTimerCreate("INACTIVITY_TIMER", 15000, pdFALSE, NULL, inactivity_callback);
    toggleTimer = xTimerCreate("TOGGLE_TIMER", 5000, pdTRUE, NULL, toggle_callback);

    xTaskCreate(serial_task, "SERIAL", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(command_interpreter, "COMMAND", 256, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    while(1){};
}
