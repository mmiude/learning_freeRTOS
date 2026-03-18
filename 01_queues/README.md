# Exercise 1

This exercise implements a code lock using FreeRTOS. Three tasks read button presses and send them via a queue to a processing task. The lock opens an LED for 5 seconds when the correct sequence 0-0-2-1-2 is entered, and resets after a 5-second timeout.
