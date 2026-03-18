# Learning FreeRTOS

This repository contains my school exercises using **FreeRTOS**, the real-time operating system for embedded devices.  
Each exercise is a small, self-contained project demonstrating concepts like task scheduling, queues, event groups and timers.

## Exercises Overview

| Exercise        | Description                                                |
|-----------------|------------------------------------------------------------|                                               
| 01_queues       | Code lock using button-reading tasks and a queue, unlocks LED on correct 0‑0‑2‑1‑2 sequence|
| 02.2_interrupts | Rotary encoder and push button control LED state and blink frequency using GPIO interrupts and a queue|
| 02_interrupts   | Serial character reader signals LED blinker via binary semaphore to indicate activity |
| 03_timers       | Interrupt-driven UART command reader with inactivity timer and LED toggle, supporting simple command interpreter|
| 04_eventGroups  | Four-task program using event group: button triggers tasks 1–3, debug task logs activity at lower priority|
| 04_eventGroups2 | Five-task program with button monitoring, event group, and watchdog: tasks report activity, debug task logs status|

