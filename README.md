# Door Lock Control

Door Lock Control is an embedded IoT project for building a connected door access system on **ESP8266** hardware. It combines lock actuation, keyboard input, persistent storage, Wi‑Fi networking, telnet-based configuration, and VSS integration in a modular architecture that can be adapted to other microcontroller-based automation projects.

## Table of Contents

- [Project Overview](#project-overview)
- [Current Hardware Focus](#current-hardware-focus)
- [Development Helper Script (`pman.sh`)](#development-helper-script-pmansh)
  - [Main command: `./pman.sh init`](#main-command-pmansh-init)
  - [Available project commands](#available-project-commands)
  - [Git hooks handled by `pman.sh`](#git-hooks-handled-by-pmansh)
- [Code Structure](#code-structure)
  - [`iohal`](#iohal)
  - [`keyboard`](#keyboard)
  - [`storage`](#storage)
  - [`tcpconnection` (`network` folder)](#tcpconnection-network-folder)
  - [`vssClient`](#vssclient)
  - [`log`](#log)
  - [`misc`](#misc)
  - [Scheduler and Promise System](#scheduler-and-promise-system)
- [Architecture Overview](#architecture-overview)
- [Status](#status)

## Project Overview

[⬆ Back to top](#door-lock-control)

This project is focused on controlling a door locking system.

Besides being focused on door lock control, this project is modular and can be easily adapted to other kinds of IoT projects.

The goal is to provide a clean and maintainable codebase for embedded door lock logic, keeping hardware-specific code separated from application behavior.

## Current Hardware Focus

[⬆ Back to top](#door-lock-control)

At the moment, this project is focused on the **ESP8266** platform because of its low power consumption and good fit for this use case.

The architecture is being designed to make migration to other microcontrollers easier in the future.

## Development Helper Script (`pman.sh`)

[⬆ Back to top](#door-lock-control)

This project includes a small management script called `pman.sh` to help with day-to-day development tasks and repository automation.

### Main command: `./pman.sh init`

[⬆ Back to top](#door-lock-control)

The most important command is:

```bash
./pman.sh init
```

Run it when setting up the project or before starting regular development. It performs the initial project setup, including:

- installing the local git hooks used by this repository,
- preparing the hidden `.pman/` workspace folder and adding it to `.gitignore`,
- opening `README.md` automatically when possible.

### Available project commands

[⬆ Back to top](#door-lock-control)

- `./pman.sh init` — initialize the project for development.
- `./pman.sh build --release` — build the firmware in release mode using PlatformIO.
- `./pman.sh build --debug` — build the firmware in debug mode.
- `./pman.sh clean` — remove generated build files.
- `./pman.sh new-version` — apply or create a new version.
- `./pman.sh finalize` — reserved for finalization tasks after development.
- `./pman.sh --help` — show the available commands and descriptions.

### Git hooks handled by `pman.sh`

[⬆ Back to top](#door-lock-control)

The `init` command installs and manages the repository git hooks in `.git/hooks`:

- `commit-msg` — validates commit message format and requires prefixes such as `feat:`, `fix:`, `docs:`, `refactor:`, `test:`, and others.
- `pre-commit` — reserved hook entry point for checks before a commit.
- `post-commit` — runs project post-commit actions.
- `post-merge` — runs project post-merge actions.

On the `main` branch, the `post-commit` and `post-merge` hooks can automatically trigger the versioning helper used by the project.

## Code Structure

[⬆ Back to top](#door-lock-control)

All logic related to door lock control is implemented in `main.cpp` and the `main.*` files. All other files and modules are project independent and can be easy used in another projects.

### code organizadion
```
  project
    ├── main.cpp -> initializes the modules and provides the dependency injection
    ├── main.lockcontrol.h -> contains the phisical look device control 
    ├── main.telnetui.h -> contains the telnet UI control, which is a simple 
    |                      telnet user interface to configure and interact with 
    |                      the system
    ├── iohal
    |     ├── iohal.h -> defines the I/O abstraction layer interface
    |     ├── impl
    |          ├── esp8266IOHal.h -> ESP8266-specific I/O implementation
    |          ├── esp8266IOHal.cpp
    ├── keyboard
    |     ├── keyboard.h -> defines the keyboard interface and event model
    |     ├── impl
    |          ├── GenericIIOHalKeyboard.h -> keyboard implementation based on 
    |                                         the I/O HAL 
    |          ├── GenericIIOHalKeyboard.cpp
    ├── storage
    |    ├── storage.h -> defines the storage interface and key-shortening strategy
    |    ├── impl
    |         ├── preferenceslibrary
    |              ├── preferenceslibrary.h -> implementation based on Preferences
    |              |                           library for esp8266
    |              ├── preferenceslibrary.cpp
    |         ├── nvsstorage
    |              ├── nvsstorage.h -> implementation based on NVS for esp32
    |              ├── nvsstorage.cpp
    ├── network
    |    ├── inetwork.h -> defines the connection service interface and network 
    |    |                 state model
    |    ├── impl
    |         ├── wfservice
    |              ├── wfservice.h -> Wi-Fi connection service implementation for ESP
    |              ├── wfservice.cpp
    ├── vssClient
    |    ├── vstpclient.h -> VSTP client interface for VSS variable operations
    |    ├── vstpclient.cpp
    ├── log
         ├── ilogger.h -> defines the logging interface and holds the 
         |                default ilogger implementation
         ├── ilogger.cpp

```

### `iohal`

[⬆ Back to top](#door-lock-control)

`iohal` is the **Hardware Abstraction Layer (HAL)**.

It isolates low-level hardware details from the rest of the application, which helps migrate the code to another microcontroller with fewer changes.

IoHal aims to abstract diferent ios types, like esp8266 digital and analogic ios, esp32 digital ans analogic ios and IO board extensions, making the code to see only a list of analogic and digital ios, don't care what is the final I/O that is being used.

### `keyboard`

[⬆ Back to top](#door-lock-control)

`keyboard` is responsible for key reading.

It includes debounce handling and an event stream model, making it easier to listen to key events in a consistent and clean way.

### `storage`

[⬆ Back to top](#door-lock-control)

`storage` defines an abstraction (`IStorage`) for internal flash persistence, with both synchronous and Promise-based asynchronous APIs (`get/set/has/remove`).

Current implementations are in `storage/impl`:

- `preferenceslibrary`: implementation based on the Preferences API, currently used in this project.
- `nvsstorage`: implementation based on NVS, intended for ESP32.

Both implementations use a key-shortening strategy to support key-length constraints and provide async operations integrated with the scheduler.

### `tcpconnection` (`network` folder)

[⬆ Back to top](#door-lock-control)

This module provides a generic connection-service interface (`IConService`) and an ESP Wi-Fi implementation (`WFService`).

`WFService` is responsible for:

- scanning and connecting to known networks,
- reconnecting when a connection is lost,
- creating an Access Point fallback when no known network is available,
- exposing connection state changes through an event stream,
- creating TCP clients through `createClient()`.

It also integrates with `storage` to persist and recover network credentials.

### `vssClient`

[⬆ Back to top](#door-lock-control)

`vssClient` provides a client implementation for VSS using the VSTP text protocol over TCP.

VSS (Variables and Streams Server) is a shared-state server and key-value database where variables are organized using dot notation and treated as data streams. Multiple clients can read/write the same variables, subscribe to changes, and react in real time to updates from other clients.

Official VSS project:
- https://github.com/rafael-tonello/Variables-and-Streams-Server-VSS

The current implementation (`VstpClient`) is responsible for:

- connecting and reconnecting to the configured VSS server,
- exchanging initial protocol headers and client ID (`setid`) during session setup,
- sending variable commands (`set`, `get`, `delete`),
- subscribing and unsubscribing to variable changes (`subscribe` / `unsubscribe`),
- dispatching asynchronous `varchanged` notifications to local observers,
- exposing connection state changes through an event stream.

This module is integrated with the internal scheduler/promise system to keep protocol operations asynchronous and non-blocking for the main application flow.

### `log`

[⬆ Back to top](#door-lock-control)

`log` contains the logging interface and default implementation.

- `ILogger` defines leveled logs (`DEBUG`, `INFO`, `WARNING`, `ERROR`, `CRITICAL`) and named log channels.
- `DefaultLogger` formats log headers (time, level, name) and writes to `Serial`.

### `misc`

[⬆ Back to top](#door-lock-control)

`misc` contains supporting libraries used by several modules, including:

- `errors`: a lightweight error/status model (`Error` as string + common constants) used in function returns and result wrappers.
- `scheduler`: the cooperative task scheduler and async utilities.
- `cancelationtoken`, `eventstream`, `stringutils`, and `configs`: general-purpose support utilities.

### Scheduler and Promise System

[⬆ Back to top](#door-lock-control)

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

[⬆ Back to top](#door-lock-control)

High-level component interaction:
<!-- 🠜 🠞 🠟 🠝 -->

```

       main/business
  ╔════════════════════╗	
  ║ main-telnet-UI ----------1------🠞 telnetserver
  ║  🠝                 ║                   |
  ║  |                 ║              +----+
  ║ main-lock-control ----+           2
  ║  |    🠝            ║  |           |
  ║  |3   |4           ║  |           🠟
  ║  +- main           ║  |          wifi
  ║       🠝            ║  |           |
  ╚═══════|════════════╝  |           5
          6               7           |
          |               |           +--> storage
          +--- keyboard   |           |
                  |       |        +--+
                  |     +-+        8
                  9     |          |
                  |     |          🠟
                  |     |    log manager
                  🠟     |        |
                iohal <-+        10
                  |              |
                  11             🠟
                  |          esp serial
                  |
                  🠟
            esp phisical i/o

  1) Telnet UI (a main extension) uses telnet server to provide a user interface
  2) Telnet server uses wifi to provide network connectivity
  3) main isolates the User Interface (a main extension) logic in main-telnet-UI
  4) main isolates the door lock control (a main extension) logic in main-lock-control
  5) wifi uses storage to persist network credentials and state
  6) Keyboard emit events, that main uses to trigger lock and unlock actions
  7) main lock control extensions uses iohal to control the physical lock device
  8) wifi manager uses log manager to log important events and errors
  9) keyboard uses iohal to read key states
  10) log manager uses esp serial to output logs
  11) iohal uses the esp physical I/O to read and write to the hardware pins  
```
Scheduler is used over all the components to keep asynchronous operations organized and non-blocking, especially for network interactions, VSS updates, and lock control timing.

## Status

[⬆ Back to top](#door-lock-control)

This is an initial structure and the project is under active development.
