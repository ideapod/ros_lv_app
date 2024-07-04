
 /* 
 * lv_app - this will manage gui as a task + uros task
 * this module's responsibility is creating and monitoring the tasks.

 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"

/* 
 * micro ROS includes 
 */
#include "uros_task.h"
#include "gui_task.h"

#define TAG "lv_app"

/**********************
 * IPC definitions
 **********************/
#include "app.h"
QueueHandle_t xDataQueue; // transmits data from uros task to gui to display (IPC)
TaskHandle_t guiTaskHandle;

/**********************
 *   APPLICATION MAIN
 **********************/
// void app_main() {
void appMain(){ 

    /* 
     * creating message buffer for inter task comms
     */ 
    xDataQueue = xQueueCreate( kQueueMaxCount, kQueueDataSize );
    
    if( xDataQueue == NULL )
    {
        ESP_LOGI(TAG, "Couldn't allocate queue. aborting");    
        vTaskDelete(NULL);
    }


    ESP_LOGI(TAG, "starting GUI Task.");
    BaseType_t taskCreateResult;
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 
    xTaskCreatePinnedToCore( TaskFunction_t pvTaskCode,
                                        const char * const pcName,
                                        const uint32_t usStackDepth,
                                        void * const pvParameters,
                                        UBaseType_t uxPriority,
                                        TaskHandle_t * const pvCreatedTask,
                                        const BaseType_t xCoreID);
    */ 

    taskCreateResult = xTaskCreatePinnedToCore(
            guiTask, 
            "gui", 
            kGuiStackSize, 
            (void *)&xDataQueue, 
            0, &guiTaskHandle, 1);

    if (pdPASS != taskCreateResult) {
        ESP_LOGI(TAG, "GUI Task create failed");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "starting ROS Task.");
    uros_start(xDataQueue);
}


