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
#define TAG "UROS"
#include "servo_driver.h"


#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status in %s on line %d: %d. Aborting.\n",__FILE__, __LINE__,(int)temp_rc);vTaskDelete(NULL);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status %s on line %d: %d. Continuing.\n",__FILE__, __LINE__,(int)temp_rc);}}

rcl_subscription_t subscriber;
std_msgs__msg__Int32 msg;
QueueHandle_t xDataQueue;

void subscription_callback(const void * msgin)
{
	const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
	ESP_LOGI(TAG, "received data: %d\n", msg->data);

	BufferDataType txData;
    BaseType_t queueResult;
        
    sprintf(txData.msg,"Data\n%d",msg->data); // TODO: risky copy, as we may overflow allowed msg size. 
    /* send the message to the gui to display */
    queueResult = xQueueSend( xDataQueue,
                              ( void * ) &txData,
                                xBlockTime );

    if( pdPASS != queueResult)
    {
        ESP_LOGI(TAG, "couldn't send data to gui.");
    }

    ESP_LOGI(TAG, "setting servo angle: %d\n", msg->data);
    set_servo_angle(msg->data);
}

void uros_start(QueueHandle_t inQueueHandle)
{
	xDataQueue = inQueueHandle;
	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;


	// set up servo on pin 18
	servo_driver_initialize(18);
	ESP_LOGI(TAG, "servo driver initialised");

	// create init_options
	RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

	
	// create node
	rcl_node_t node;
	RCCHECK(rclc_node_init_default(&node, "lv_demo_rclc", "", &support));

	ESP_LOGI(TAG, "Node created: lv_demo_rclc");
	// create subscriber
	RCCHECK(rclc_subscription_init_default(
		&subscriber,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"/microROS/int32_subscriber"));

	ESP_LOGI(TAG, "subscription created to: /microROS/int32_subscriber");
	// create executor
	rclc_executor_t executor;
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));

	ESP_LOGI(TAG, "executor spinning");
	while(1){
			rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
			usleep(100000);
	}

	ESP_LOGI(TAG, "executor exited? cleaning up and dying (shouldnt happen)");
	// free resources
	RCCHECK(rcl_subscription_fini(&subscriber, &node));
	RCCHECK(rcl_node_fini(&node));
	
	vTaskDelete(NULL);
}
