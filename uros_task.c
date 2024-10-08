/*
 * micro ros tasks to adapt robot to ros.
 */

#include <stdio.h>
#include <unistd.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <std_msgs/msg/int32.h>

#include <rclc/rclc.h>
#include <rclc/executor.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "uros_task.h"
#include "app.h"
#include "http_calls.h"
#define TAG "UROS"

//	#include "servo_driver.h"
//	#define SERVO_PIN 18	
#include "servo_pca9685.h"
#define I2C_ADDRESS 0x40

// uncomment if we need to do http calls for heartbeats.
// #define HTTP_HEARTBEAT 1

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status in %s on line %d: %d. Aborting.\n",__FILE__, __LINE__,(int)temp_rc);vTaskDelete(NULL);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status %s on line %d: %d. Continuing.\n",__FILE__, __LINE__,(int)temp_rc);}}

/*************************
 * globals
 *************************/
rcl_subscription_t subscriber, subscriber0, subscriber1;
std_msgs__msg__Int32 servo0_msg, servo1_msg;
QueueHandle_t xDataQueue;
rcl_node_t node;
rcl_publisher_t publisher;
std_msgs__msg__Int32 publish_msg;
int count_seconds;
rcl_timer_t timer;
bool do_http_heartbeat = false;
bool do_report = false;

/*****************************
Prototypes
******************************/
void send_queue_servo_angle(int servo_num, int32_t data);
void process_servo_msg(int servo_num, const std_msgs__msg__Int32 *msg);



void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
	RCLC_UNUSED(last_call_time);
	if (timer != NULL) {
		RCSOFTCHECK(rcl_publish(&publisher, &publish_msg, NULL));
		publish_msg.data++;

		count_seconds++;

		if (count_seconds % 50 == 0) {
			do_report = true;
		}

		if (count_seconds % 10 == 0) {
			do_http_heartbeat = true;
		}


	}
}


/*
 * Initialise the servo control system. 
 */
void servo_control_initialise() {

	// assuming that the i2c master is set up on the UI task
	// we need to wait until it's ready.
	ESP_LOGI(TAG, "waiting for i2c master to be setup by GUI");
	vTaskDelay(100);
	ESP_LOGI(TAG, "assume it's done by now.");

	// set up servo on pin 18
	// servo_driver_initialize(SERVO_PIN);
	servo_pca9685_initialise(I2C_ADDRESS);
	set_channel_min_max_pulse_us(0, 500, 2500, 180); // Channel 0 - CSPower DS-S006M 500us -> 2500us), 180 degrees
	set_channel_min_max_pulse_us(1, 1000, 2000, 180); // Channel 1 - Tower Pro SG 90 - 1000us -> 2000 us, 180 degrees

	ESP_LOGI(TAG, "servo driver initialised");
}


/*
 * Send a message to the UI to display the angle requested and the servo requested
 */
void send_queue_servo_angle(int servo_num, int32_t data) {
	BufferDataType txData;
  BaseType_t queueResult;
        
  sprintf(txData.msg,"Servo:%d\n%d",servo_num, data); // TODO: risky copy, as we may overflow allowed msg size. 
  /* send the message to the gui to display */
  queueResult = xQueueSend( xDataQueue,
                            ( void * ) &txData,
                              xBlockTime );

  if( pdPASS != queueResult)
  {
      ESP_LOGI(TAG, "couldn't send data to gui.");
  }
}

/*
 * process the ros message for the given servo
 */
void process_servo_msg(int servo_num, const std_msgs__msg__Int32 *msg) {
	
	ESP_LOGI(TAG, "setting servo angle: %d", msg->data);

	send_queue_servo_angle(servo_num,msg->data);

	// set_servo_angle(msg->data);
	set_pca9685_servo_angle (servo_num, msg->data);
}


void servo0_callback(const void * msgin)
{
	const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
	
	ESP_LOGI(TAG, "servo 0 received msg: %d", msg->data);

	process_servo_msg(0,msg); // in this configuration there is only 1 servo 
}

void servo1_callback(const void * msgin)
{
	const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
	
	ESP_LOGI(TAG, "servo 1 received msg: %d", msg->data);

	process_servo_msg(1,msg); // in this configuration there is only 1 servo 
}

void do_http_call()
{
	char http_buffer[500] = {0};
	int http_buf_len = 500;
	esp_err_t err = http_rest_get("http://httpbin.org/get", (char *)http_buffer, http_buf_len); // simple http method.
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Could not retrieve data %s", esp_err_to_name(err));
	} else {
		ESP_LOGI(TAG, "retreieved: length: %d, data: %s", strlen((char *)http_buffer), (char *)http_buffer);	
	}
}

void uros_start(QueueHandle_t inQueueHandle)
{
	xDataQueue = inQueueHandle;
	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;


	servo_control_initialise();

	i2c_scan();

	http_calls_init();

	// create init_options
	RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

	
	// create node

	RCCHECK(rclc_node_init_default(&node, "lv_demo_rclc", "", &support));

	ESP_LOGI(TAG, "Node created: lv_demo_rclc");

	// create subscriber 
/*	RCCHECK(rclc_subscription_init_default(
		&subscriber,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"/microROS/int32_subscriber"));
*/
	RCCHECK(rclc_subscription_init_default(
		&subscriber0,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"/servo0/int32_subscriber"));

	ESP_LOGI(TAG, "subscription created to: /servo0/int32_subscriber");

	RCCHECK(rclc_subscription_init_default(
		&subscriber1,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"/servo1/int32_subscriber"));
	ESP_LOGI(TAG, "subscription created to: /servo1/int32_subscriber");

	// create publisher
	RCCHECK(rclc_publisher_init_default(
		&publisher,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"freertos_int32_publisher"));

	// create timer for publishing a message. 
	
	const unsigned int timer_timeout = 1000; // publish every 1 second. 
	count_seconds = 0;
	publish_msg.data = 0;
	RCCHECK(rclc_timer_init_default(
		&timer,
		&support,
		RCL_MS_TO_NS(timer_timeout),
		timer_callback));


	// create executor
	rclc_executor_t executor;
	int num_handles = 3; // subscription, publisher, timer.
	RCCHECK(rclc_executor_init(&executor, &support.context, num_handles, &allocator));
	// RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));
	
	RCCHECK(rclc_executor_add_subscription(&executor, &subscriber0, &servo0_msg, &servo0_callback, ON_NEW_DATA));
	RCCHECK(rclc_executor_add_subscription(&executor, &subscriber1, &servo1_msg, &servo1_callback, ON_NEW_DATA));
	RCCHECK(rclc_executor_add_timer(&executor, &timer));

	ESP_LOGI(TAG, "executor spinning");
	
	int no_data = 0;
	int error_count = 0;
	rcl_ret_t ret;

	while(1){
			ret = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
			if (ret == RCL_RET_TIMEOUT) {
				no_data = no_data + 1;
			}
			if (ret == RCL_RET_ERROR) {
				error_count = error_count + 1;
			}

			// every 50 seconds summarise rclc returns		
			if (do_report  == true) {
				ESP_LOGI(TAG, "%d seconds passed, no data returned %d times, errors %d times.",count_seconds, no_data, error_count);
				no_data = 0;
				error_count = 0;
				do_report = false;
			}

#ifdef HTTP_HEARTBEAT 
			// every 10 seconds do an http_call to keep it awake.
			if (do_http_heartbeat == true) {
					do_http_call();
					do_http_heartbeat = false;
			}
#endif

			usleep(10000);
	}

	ESP_LOGI(TAG, "executor exited? cleaning up and dying (shouldnt happen)");
	// free resources
	RCCHECK(rcl_subscription_fini(&subscriber, &node));
	RCCHECK(rcl_node_fini(&node));
	
	vTaskDelete(NULL);
}
