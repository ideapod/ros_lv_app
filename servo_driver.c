/* servo motor control example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_log.h"
#define TAG "SERVO"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"


#include "servo_driver.h"

//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH 500 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2500 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 90 //Maximum angle in degree upto which servo can rotate
#define SERVO_PIN 18



/**
 * @brief sets up the servo on given pin and initialises the mcpwm module on ESP32 for the pin
 *
 * @param  servo_pin the ESP32 pin for the servo
 *
 * @return
 *     - calculated pulse width
 */
void servo_driver_initialize(uint32_t servo_pin)
{
    ESP_LOGI(TAG,"initializing mcpwm servo control gpio on pin %d",servo_pin);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, servo_pin);    //Set GPIO 18 as PWM0A, to which servo is connected
    ESP_LOGI(TAG,"Configuring Initial Parameters of mcpwm......");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
}

/**
 * @brief Use this function to calcute pulse width for per degree rotation
 *
 * @param  degree_of_rotation the angle in degree to which servo has to rotate
 *
 * @return
 *     - calculated pulse width
 */
uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}

/**
 * @brief Use this function to change the servo
 *
 * @param  the angle in degrees to which servo has to rotate
 *
 * @return
 *     - void
 */
void set_servo_angle(uint32_t degree_angle)
{
    ESP_LOGI(TAG,"Angle of rotation: %d", degree_angle);
    if (degree_angle > SERVO_MAX_DEGREE) {
        degree_angle = SERVO_MAX_DEGREE; 
    } else if (degree_angle < 0) {
        degree_angle = 0;
    }

    uint32_t pulse_width = servo_per_degree_init(degree_angle);
    ESP_LOGI(TAG,"pulse width: %dus", pulse_width);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, pulse_width);
    // vTaskDelay(10);     //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation at 5V
}


