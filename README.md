# Door Lock Control

## Project Overview

This project is focused on controlling a door locking system.

The goal is to provide a clean and maintainable codebase for embedded door lock logic, keeping hardware-specific code separated from application behavior.

## Current Hardware Focus

At the moment, this project is focused on the **ESP8266** platform because of its low power consumption and good fit for this use case.

The architecture is being designed to make migration to other microcontrollers easier in the future.

## Code Structure

### `iohal`

`iohal` is the **Hardware Abstraction Layer (HAL)**.

It isolates low-level hardware details from the rest of the application, which helps migrate the code to another microcontroller with fewer changes.

### `keyboard`

`keyboard` is responsible for key reading.

It includes debounce handling and an event stream model, making it easier to listen to key events in a consistent and clean way.

## Status

This is an initial structure and the project is under active development.
