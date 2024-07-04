#pragma once
/* 
 * this is the app coordination header.
 * all tasks should include this header and use to communicate 
 */ 
#include <unistd.h>
#include <std_msgs/msg/int32.h>
#include "freertos/queue.h"

#define MESSAGE_SIZE 20
typedef struct BufferDataType {
	char msg[MESSAGE_SIZE];
	int32_t data;
} BufferDataType;
#define kQueueDataSize (sizeof(BufferDataType))
#define kQueueMaxCount 5
#define xBlockTime pdMS_TO_TICKS(20)