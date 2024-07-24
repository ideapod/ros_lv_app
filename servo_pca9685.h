#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief sets up the servo on given pin and initialises the mcpwm module on ESP32 for the pin
 *
 * @param  addr - the I2C address for the pca9685
 *
 * @return
 *     - calculated pulse width
 */
void servo_pca9685_initialise(uint8_t addr);

/**
 * @brief Use this function to change the servo
 *
 * @param  num - the channel for the servo
 * @param degree_angle - the angle for the servo
 *
 * @return
 *     - void
 */
void set_pca9685_servo_angle(uint8_t num, uint32_t degree_angle);

/*
 * @brief set the servo characteristics for the channel
 * @param channel - the channel for the servo
 * @param min_pulse_us - the minimum pulse width in microseconds(us)
 * @param max_pulse_us - the maximum pulse width in microseconds(us)
 * @param max_degrees - the maximum degrees for the servo rotation.
 */
void set_channel_min_max_pulse_us(uint8_t channel, uint32_t min_pulse_us, uint32_t max_pulse_us, uint32_t max_degree);

/* 
 * dump out to log all the i2C devices.
 */
void i2c_scan();

#define PCA9685_MAX_STEPS 4096

#ifdef __cplusplus
}
#endif
