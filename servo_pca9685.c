/**
 *  @file pca9685Test.c
 *  
 *  @author     Jonas Scharpf <jonas@brainelectronics.de>
 *  @date       Januar 2018
 *  @version    1.0
 *  
 *  @brief      set PWM of outputs of PCA9685 slave chip
 *  
 *  Description:
 *      I2C Slave device PCA9685 at adress 0x40 can be controlled
 *      to set individual PWM to the outputs
 *      The PWM frequency can only be set for all outputs to same value
 *      
 *  Circuit:
 *      PCA9685 attached to pins 4, 5
 *      I2C Connection:
 *          Board   SDA       SCL
 *          ESP32   any (5)   any (4)
 *          Mega    20        21
 *          Tiny    D0/pin 5  D2/pin 7  (Digispark, Digispark Pro)
 *          Uno     A4        A5
 *  
 */
#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#include "pca9685.h"
#include "servo_pca9685.h"

#define TAG "PCA9685"
//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH 500 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2500 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 90 //Maximum angle in degree upto which servo can rotate
#define PCA9685_CLOCK_FREQUENCY_HZ 60 // 1000 Hz for LED's, 50Hz for Servos.
#define MAX_CHANNELS 16

/***************************
 * globals
 ***************************/
typedef struct channel_config {
    uint32_t min_pulse_us;
    uint32_t max_pulse_us;
    uint32_t max_degree;
} channel_config_t;

channel_config_t channels[MAX_CHANNELS];


#undef ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x)   do { esp_err_t rc = (x); if (rc != ESP_OK) { ESP_LOGE("err", "esp_err_t = %d", rc); assert(0 && #x);} } while(0);


void i2c_scan() {
    ESP_LOGI(TAG, "i2c scan:");
    for (uint8_t i = 1; i < 127; i++)
    {
        int ret;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 100 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
    
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Found device at: 0x%2x", i);
            // RCLCPP_DEBUG(node->get_logger(), "My log message %d", 4);
        }
    }
    ESP_LOGI(TAG, "I2C scan complete:");
}

void servo_pca9685_initialise(uint8_t addr) {
    ESP_LOGI(TAG, "initialising pca9685, executing on core %d, with address: %x", xPortGetCoreID(), addr);
    esp_err_t ret;
    set_pca9685_adress(addr);
    ret = resetPCA9685();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "resetting pca9685 failed: error code: %d", ret);
        return;
    }
    ret = setFrequencyPCA9685(PCA9685_CLOCK_FREQUENCY_HZ);  // 1000 Hz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "setting frequence pca9685 failed: error code: %d", ret);
        return;
    }

    turnAllOff();

    ESP_LOGI(TAG,"Finished pca9685 setup");
}

void set_channel_min_max_pulse_us(uint8_t channel, uint32_t min_pulse_us, uint32_t max_pulse_us, uint32_t max_degree) {

    channels[channel].min_pulse_us = min_pulse_us;
    channels[channel].max_pulse_us = max_pulse_us;
    channels[channel].max_degree = max_degree;
} 


/**
 * @brief Use this function to calcute pulse width for per degree rotation
 *
 * @param  degree_of_rotation the angle in degree to which servo has to rotate
 *
 * @return
 *     - calculated pulse width
 */
uint32_t servo_rot_to_pulsewidth(uint8_t channel, uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    float max_pulse_width = (channels[channel].max_pulse_us - channels[channel].min_pulse_us);
    float min_width = channels[channel].min_pulse_us;
    float max_degrees = channels[channel].max_degree;
    float req_rotation = degree_of_rotation;

    
    cal_pulsewidth = min_width +  (max_pulse_width * req_rotation / max_degrees);

    ESP_LOGI(TAG,"min:%d, max:%d, max degrees:%d, pulse width: %d",
        channels[channel].min_pulse_us,
        channels[channel].max_pulse_us,
        channels[channel].max_degree,
         cal_pulsewidth);

    return cal_pulsewidth;
}

void get_steps_on_off_pulse_width(uint8_t channel, uint16_t *step_on, uint16_t *step_off, uint32_t pulse_width) {
    *step_on = 0;

    // work out the maximum ticks - 4096 steps per period of the clock frequency
    // we set. So 1/freq * 1,000,000 to convert to microseconds. 
    float max_pulse_width = (float) 1.0 / PCA9685_CLOCK_FREQUENCY_HZ * 1000000;

    // convert % pulse of max width to a number of steps (% of 4096)
    *step_off = ((float)(pulse_width / max_pulse_width) * PCA9685_MAX_STEPS);
}


void set_pca9685_servo_angle(uint8_t num, uint32_t degree_angle) {
    esp_err_t ret;

    ESP_LOGI(TAG,"Servo: %d -> Angle of rotation: %d", num, degree_angle);
    if (degree_angle > SERVO_MAX_DEGREE) {
        degree_angle = SERVO_MAX_DEGREE; 
    } else if (degree_angle < 0) {
        degree_angle = 0;
    }

    uint32_t pulse_width = servo_rot_to_pulsewidth(num,degree_angle);
    
    uint16_t step_on, step_off;
    get_steps_on_off_pulse_width(num, &step_on,&step_off,pulse_width);
    ESP_LOGI(TAG,"step on: %d, step off: %d", step_on, step_off);

    ret = setPWM(num, step_on, step_off);

    if(ret == ESP_ERR_TIMEOUT)
    {
        ESP_LOGI(TAG, "I2C timeout");
    }
    else if(ret == ESP_OK)
    {
        ESP_LOGI(TAG, "PWM set successfully");
    }
    else
    {
        ESP_LOGE(TAG, "No ack, sensor not connected...skip...\n");
    }
}