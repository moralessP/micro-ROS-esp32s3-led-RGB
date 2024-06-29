#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_system.h"

#include <uros_network_interfaces.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <std_msgs/msg/u_int8_multi_array.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
#include <rmw_microros/rmw_microros.h>
#endif

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc);vTaskDelete(NULL);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

#define LED_RGB_GPIO 38
#define LEN_DATA 3

static led_strip_handle_t led_strip;

rcl_subscription_t subscriber;
std_msgs__msg__UInt8MultiArray led_rgb_msg;

static void configure_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_RGB_GPIO,
        .max_leds = 1, 
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

void subscription_callback(const void *msgin)
{
    const std_msgs__msg__UInt8MultiArray *msg = (const std_msgs__msg__UInt8MultiArray *)msgin;

    ESP_ERROR_CHECK(led_strip_clear(led_strip));
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, msg->data.data[0], msg->data.data[1], msg->data.data[2]));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void uros_task(void *arg)
{
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;

	rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
	RCCHECK(rcl_init_options_init(&init_options, allocator));

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
	rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&init_options);
	RCCHECK(rmw_uros_options_set_udp_address(CONFIG_MICRO_ROS_AGENT_IP, CONFIG_MICRO_ROS_AGENT_PORT, rmw_options));
#endif

    RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator));

    // Create Node 
    rcl_node_t node = rcl_get_zero_initialized_node();
	RCCHECK(rclc_node_init_default(&node, "led_rgb_node", "", &support));

    // Create Subscriber 
    RCCHECK(rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt8MultiArray),
        "led_rgb"));
    
    // Create executor.
	rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	unsigned int rcl_wait_timeout = 1000;
	RCCHECK(rclc_executor_set_timeout(&executor, RCL_MS_TO_NS(rcl_wait_timeout)));

    RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &led_rgb_msg, &subscription_callback, ON_NEW_DATA));

    uint8_t led_rgb_data[LEN_DATA];
    led_rgb_msg.data.data = led_rgb_data;
    led_rgb_msg.data.size = 0;
    led_rgb_msg.data.capacity = LEN_DATA;

    while (1) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        usleep(100000);
    }

    RCCHECK(rcl_subscription_fini(&subscriber, &node));
    RCCHECK(rcl_node_fini(&node));
}

void app_main(void)
{

    configure_led();
#if defined(CONFIG_MICRO_ROS_ESP_NETIF_WLAN) || defined(CONFIG_MICRO_ROS_ESP_NETIF_ENET)
    ESP_ERROR_CHECK(uros_network_interface_initialize());
#endif

    xTaskCreate(uros_task,
            "uros_task",
            CONFIG_MICRO_ROS_APP_STACK,
            NULL,
            CONFIG_MICRO_ROS_APP_TASK_PRIO,
            NULL);

}