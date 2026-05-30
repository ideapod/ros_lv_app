# ros_lv_app

ESP32 firmware that connects a micro-ROS node to an SSD1306 OLED display and hobby servos via a PCA9685 PWM board. Receive ROS `Int32` angle commands over Wi-Fi, drive servos, and show status on screen.

## Hardware

- ESP32 dev board
- SSD1306 128×64 OLED (I2C)
- Adafruit PCA9685 16-channel PWM/servo board (I2C, address `0x40`)
- Servos on PCA9685 channels 0 and 1:
  - Ch 0: CSPower DS-S006M (500–2500 µs, 180°)
  - Ch 1: Tower Pro SG90 (1000–2000 µs, 180°)
- Travel router providing Wi-Fi bridge to the ROS host

## ROS Interface

| Topic | Type | Direction | Description |
|---|---|---|---|
| `/servo0/int32_subscriber` | `std_msgs/Int32` | subscribe | Angle (degrees) for servo 0 |
| `/servo1/int32_subscriber` | `std_msgs/Int32` | subscribe | Angle (degrees) for servo 1 |
| *(heartbeat)* | `std_msgs/Int32` | publish | Counter published at ~1 Hz to keep the transport alive |

Node name: `lv_demo_rclc`

Send a servo command from the host:
```bash
ros2 topic pub /servo0/int32_subscriber std_msgs/msg/Int32 "{data: 90}"
```

## Building & Flashing

Requires ESP-IDF with the micro-ROS component configured. With the IDF environment sourced:

```bash
idf.py build
idf.py flash monitor
```

## Architecture

Three FreeRTOS tasks communicate via a single queue:

```
uros_task  ──(xDataQueue)──▶  gui_task   (LVGL/SSD1306, pinned core 1)
    │
    └──▶  servo_pca9685  (I2C → PCA9685 → servos)
```

`uros_task` receives ROS messages, moves the servos, and forwards display strings to `gui_task` through the queue. The 1 Hz publish keeps the travel router forwarding inbound messages.

## 3D Files

- `TestRigOLEDServo.scad` — test rig for the OLED and servo
- `ssd1603-128x64-oled.scad` — SSD1306 screen mount
- `Hobby-Servo-Brackets/` — mini and standard servo mounting brackets (OpenSCAD → STL for FDM printing)
