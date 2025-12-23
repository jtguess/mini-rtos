# Mini-RTOS Scheduler (C++)

A small, self-contained fixed-rate task scheduler written in modern C++ that demonstrates real-time embedded concepts—periodic scheduling, jitter measurement, deadline monitoring, overloadbehavior, and deterministic timing—running on a desktop OS (macOS/Linux).

This project is intentionally minimal and designed as a learning and interview artifact for real-time and embedded software roles.

---

## Features

- Fixed-rate periodic tasks using a monotonic clock
- Absolute release scheduling to prevent drift
- Per-task telemetry
    - start jitter (avg / max)
    - execution time (avg / max)
    - release count
    - deadline overruns
- Overload injection to demonstrate missed deadlines and recovery
- Deterministic behavior with a fixed task set and no dynamic allocation in the steady-state path
- Cooperative, single-threaded scheduler

---

## Building
## CMake
    cmake -S . -B build
    cmake --build build -j
    ./build/mini_rtos
## Overload mode
    ./build/mini_rtos --overload
