# Door Lock Control

## Project Overview

This project is focused on controlling a door locking system.

Besides being focused on door lock control, this project is modular and can be easily adapted to other kinds of IoT projects.

The goal is to provide a clean and maintainable codebase for embedded door lock logic, keeping hardware-specific code separated from application behavior.

## Current Hardware Focus

At the moment, this project is focused on the **ESP8266** platform because of its low power consumption and good fit for this use case.

The architecture is being designed to make migration to other microcontrollers easier in the future.

## Code Structure

All logic related to door lock control is implemented in `main.cpp` and the `main.*` files. All other files and modules are project independent and can be easy used in another projects.

### `iohal`

`iohal` is the **Hardware Abstraction Layer (HAL)**.

It isolates low-level hardware details from the rest of the application, which helps migrate the code to another microcontroller with fewer changes.

IoHal aims to abstract diferent ios types, like esp8266 digital and analogic ios, esp32 digital ans analogic ios and IO board extensions, making the code to see only a list of analogic and digital ios, don't care what is the final I/O that is being used.

### `keyboard`

`keyboard` is responsible for key reading.

It includes debounce handling and an event stream model, making it easier to listen to key events in a consistent and clean way.

### `storage`

`storage` defines an abstraction (`IStorage`) for internal flash persistence, with both synchronous and Promise-based asynchronous APIs (`get/set/has/remove`).

Current implementations are in `storage/impl`:

- `preferenceslibrary`: implementation based on the Preferences API, currently used in this project.
- `nvsstorage`: implementation based on NVS, intended for ESP32.

Both implementations use a key-shortening strategy to support key-length constraints and provide async operations integrated with the scheduler.

### `tcpconnection` (`network` folder)

This module provides a generic connection-service interface (`IConService`) and an ESP Wi-Fi implementation (`WFService`).

`WFService` is responsible for:

- scanning and connecting to known networks,
- reconnecting when a connection is lost,
- creating an Access Point fallback when no known network is available,
- exposing connection state changes through an event stream,
- creating TCP clients through `createClient()`.

It also integrates with `storage` to persist and recover network credentials.

### `log`

`log` contains the logging interface and default implementation.

- `ILogger` defines leveled logs (`DEBUG`, `INFO`, `WARNING`, `ERROR`, `CRITICAL`) and named log channels.
- `DefaultLogger` formats log headers (time, level, name) and writes to `Serial`.

### `misc`

`misc` contains supporting libraries used by several modules, including:

- `errors`: a lightweight error/status model (`Error` as string + common constants) used in function returns and result wrappers.
- `scheduler`: the cooperative task scheduler and async utilities.
- `cancelationtoken`, `eventstream`, `stringutils`, and `configs`: general-purpose support utilities.

### Scheduler and Promise System

The scheduler is a core piece of this project architecture.

It is a cooperative scheduler with a weighted-priority model. Tasks are queued by priority and executed following an internal run model generated from priority weights.

Main capabilities:

- immediate task scheduling (`run`),
- delayed one-shot tasks (`delayedTask`),
- periodic tasks (`periodicTask`),
- runtime task control (`abort`, `pause`, `resume`) through `TimedTask`.

Inside `misc/scheduler/misc`, the Promise system (`Promise`, `AsyncProcessChain`) provides lightweight asynchronous chaining (`then`, `post/resolve`) used throughout the project.

Utilities such as `runFsInOrder` and `WaitPromises` help compose async flows cleanly, reducing nested callback code and improving readability.

## Architecture Overview

High-level component interaction:

	main / door-lock logic
			|
			+--> scheduler + promise chain (execution model)
			|
			+--> keyboard (input events)
			|
			+--> iohal (hardware abstraction)
			|
			+--> storage (persistent configuration/state)
			|
			+--> tcpconnection/network (Wi-Fi and client connections)
			|
			+--> log (observability)

Typical runtime flow:

1. `main` initializes infrastructure modules (`scheduler`, `log`, `storage`, `network`, input, and HAL).
2. Inputs and connection events are converted into asynchronous operations.
3. Operations are scheduled with priorities and optional delays/periodicity.
4. State is persisted through `storage` and relevant transitions are logged.
5. The application loop calls the scheduler to cooperatively execute pending work.

## Status

This is an initial structure and the project is under active development.
