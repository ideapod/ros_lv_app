#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief sets up the servo on given pin and initialises the mcpwm module on ESP32 for the pin
 *
 * @param  void - none
 *
 * @return
 *     - none
 */
void http_calls_init(void);


/**
 * @brief invokes get request on endpointURL and puts the resulting pack in results_buf
 *
 * @param  endpointURL - the URL for the endpoint
 * 
 * @param  result_buf - pre-allocated buffer
 * 
 * @param max_buf_size - the size of the pre-allocated buffer
 *
 * @return
 *     - esp error value. ok or not. 
 */
esp_err_t http_rest_get(char *endpointURL, char *result_buf, uint32_t max_buf_size); 


#ifdef __cplusplus
}
#endif
