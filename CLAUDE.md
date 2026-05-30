# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32 firmware project that bridges micro-ROS and an LVGL-based OLED display (SSD1306), with servo control via a PCA9685 16-channel PWM board over I2C. It is built using the ESP-IDF framework with the micro-ROS component.

## Build System

This project uses **ESP-IDF** (not a standard CMakeLists.txt at the root). The entry point is `appMain()` in `app.c` — note it is **not** named `app_main()`, which is intentional for integration with the micro-ROS component build system.

The `app-colcon.meta` file configures the micro-ROS middleware (rmw_microxrcedds) build constraints:
- Max 1 node, 1 publisher, 2 subscriptions, no services/clients, history=5

Common ESP-IDF commands (run from repo root with IDF environment sourced):
```bash
idf.py build
idf.py flash
idf.py monitor
idf.py flash monitor     # flash and immediately open serial monitor
idf.py menuconfig        # configure project options
```

## Architecture

The application has three main FreeRTOS tasks communicating via a single FreeRTOS queue (`xDataQueue`):

```
uros_task  ──(xDataQueue)──▶  gui_task
    │
    └──▶  servo_pca9685 (I2C → PCA9685 board → servos)
```

**`app.c`** — creates `xDataQueue` and spawns `guiTask` (pinned to core 1) then calls `uros_start()`. The queue carries `BufferDataType` structs (`app.h`) with a short display string and an int32 value.

**`uros_task.c`** — the micro-ROS node. Subscribes to `/servo0/int32_subscriber` and `/servo1/int32_subscriber` (std_msgs/Int32). On receipt, calls `set_pca9685_servo_angle()` and sends angle to the queue for GUI display. Also publishes a heartbeat counter at ~1 msg/sec to keep the transport layer alive (this was found necessary to prevent the travel router from dropping forwarded messages).

**`gui_task.c`** — LVGL task driving the SSD1306 OLED. Reads from `xDataQueue` and updates the display with servo status.

**`servo_pca9685.c`** — Wraps the PCA9685 component to set servo angles. Each channel has configurable min/max pulse widths and degrees. Current configuration:
- Channel 0: CSPower DS-S006M, 500–2500 µs, 180°
- Channel 1: Tower Pro SG90, 1000–2000 µs, 180°

**`components/pca9685/`** — Forked from [brainelectronics/esp32-pca9685](https://github.com/brainelectronics/esp32-pca9685) with a modified PWM frequency calculation (see `pca9685.c` line 218).

**`servo_driver.c`** — An older direct-GPIO PWM servo driver (currently commented out in `uros_task.c`, superseded by the PCA9685 approach).

**`http_calls.c`** — HTTP GET helper using ESP-IDF's HTTP client. Used for optional heartbeat calls to keep the TCP connection through a travel router active. Controlled by `#define HTTP_HEARTBEAT` in `uros_task.c` (currently disabled).

## Key Design Notes

- I2C master is initialized by `gui_task` before `uros_task` starts servo control. `uros_task` does a fixed `vTaskDelay(100)` to wait — if I2C init hangs or is slow, this is the fragile point.
- If the PCA9685 disappears from the I2C bus (e.g., due to power brownout), `i2c_scan()` in `uros_start()` will log this; re-seating the cable/power has been the fix.
- The micro-ROS node name is `lv_demo_rclc`.
- ROS topic angles are plain `Int32` degree values (0–180 clamped in `set_pca9685_servo_angle`).

## Hardware / Physical Files

- `TestRigOLEDServo.scad` — OpenSCAD model for the test rig housing OLED and servo
- `ssd1603-128x64-oled.scad` — OpenSCAD model for the SSD1306 screen
- `Hobby-Servo-Brackets/` — 3D-printable servo bracket designs (mini and standard)
