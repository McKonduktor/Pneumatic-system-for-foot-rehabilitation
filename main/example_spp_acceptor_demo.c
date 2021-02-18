#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_types.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "driver/gpio.h"
#include <driver/dac.h>
#include "driver/timer.h"
#include "time.h"
#include "driver/periph_ctrl.h"
#include "sys/time.h"
#include <driver/adc.h>
#include "esp_adc_cal.h"

#define VALVE_1_GPIO 18					//IO18
#define VALVE_2_GPIO 17					//IO17
#define DUAL_VALVE_GPIO 27				//IO27
#define COMPRESSOR_GPIO DAC_CHANNEL_2 	//IO26
#define LED_GPIO 16						//IO16
#define SENSOR_1 ADC_CHANNEL_5   		//IO12
#define SENSOR_2 ADC_CHANNEL_6   		//IO14
#define SPP_TAG "ESP_SSP_LOG"
#define SPP_SERVER_NAME "ESP_SPP_SERVER"
#define DEVICE_NAME "ESP32 Rehab"
#define SPP_SHOW_DATA 0
#define SPP_SHOW_SPEED 1
#define SPP_SHOW_MODE SPP_SHOW_DATA


static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static struct timeval time_old;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

int STATE;
int VALVE_1_STATE;
int VALVE_2_STATE;
int DUAL_VALVE_STATE;
int PRESSURE;
int COMPRESSOR_SPEED;
int TIME;
int LOCK;

void set_speed(int speed){
	dac_output_voltage(COMPRESSOR_GPIO, speed);
}

void set_valve_1_state(int state){
	if(0 == state){
		VALVE_1_STATE = 1;
	} else {
		VALVE_1_STATE = 0;
	}
	gpio_set_level(VALVE_1_GPIO, VALVE_1_STATE);
}

void set_valve_2_state(int state){
	if(0 == state){
		VALVE_2_STATE = 1;
	} else {
		VALVE_2_STATE = 0;
	}
	gpio_set_level(VALVE_2_GPIO, VALVE_2_STATE);
}

void set_dual_valve_state(int state){
	if(0 == state){
		DUAL_VALVE_STATE = 0;
	} else {
		DUAL_VALVE_STATE = 1;
	}
	gpio_set_level(DUAL_VALVE_GPIO, DUAL_VALVE_STATE);
}

int get_pressure_1(){
	int read_raw;
	adc2_get_raw(SENSOR_1, ADC_WIDTH_12Bit, &read_raw);
	return read_raw*3.3/4096*50/2.976;
}

int get_pressure_2(){
	int read_raw;
	adc2_get_raw(SENSOR_2, ADC_WIDTH_12Bit, &read_raw);
	return read_raw*3.3/4096*50/2.976;
}

void led(int state){
	if(0 == state){
		gpio_set_level(LED_GPIO, 0);
	} else {
		gpio_set_level(LED_GPIO, 1);
	}
}


static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:
#if (SPP_SHOW_MODE == SPP_SHOW_DATA)
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d",
                 param->data_ind.len, param->data_ind.handle);
        esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
        STATE = param->data_ind.data[0];
        PRESSURE = param->data_ind.data[1];
        COMPRESSOR_SPEED = param->data_ind.data[2];
        TIME = param->data_ind.data[3];
        printf("BT_data_received");
#else
        gettimeofday(&time_new, NULL);
        data_num += param->data_ind.len;
        if (time_new.tv_sec - time_old.tv_sec >= 3) {
            print_speed();
        }
#endif
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
        gettimeofday(&time_old, NULL);
        break;
    default:
        break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(SPP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(SPP_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    default: {
        ESP_LOGI(SPP_TAG, "event: %d", event);
        break;
    }
    }
    return;
}



void app_main(void)
{

    gpio_pad_select_gpio(VALVE_1_GPIO);
    gpio_set_direction(VALVE_1_GPIO, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(VALVE_2_GPIO);
    gpio_set_direction(VALVE_2_GPIO, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(DUAL_VALVE_GPIO);
    gpio_set_direction(DUAL_VALVE_GPIO, GPIO_MODE_OUTPUT);

    dac_output_enable(COMPRESSOR_GPIO);

    adc2_config_channel_atten( SENSOR_1, ADC_ATTEN_11db );
    vTaskDelay(2 * portTICK_PERIOD_MS);
    adc2_config_channel_atten( SENSOR_2, ADC_ATTEN_11db );
    vTaskDelay(2 * portTICK_PERIOD_MS);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    while(1){
    	if(STATE){
    		led(1);
    		int current_pressure = 0;
    		switch(DUAL_VALVE_STATE){
				case 0:
					set_dual_valve_state(0);
					current_pressure = get_pressure_1();
					set_valve_1_state(0);
					while(current_pressure <= PRESSURE && STATE){
						current_pressure = get_pressure_1();
						vTaskDelay( 2 * portTICK_PERIOD_MS );
						set_speed(COMPRESSOR_SPEED);
					}
					set_speed(0);
					vTaskDelay(TIME * 1000 / portTICK_PERIOD_MS);
					set_valve_1_state(1);
					DUAL_VALVE_STATE = 1;
					break;
				case 1:
					set_dual_valve_state(1);
					current_pressure = get_pressure_2();
					set_valve_2_state(0);
					while(current_pressure <= PRESSURE && STATE){
						current_pressure = get_pressure_2();
						vTaskDelay( 2 * portTICK_PERIOD_MS );
						set_speed(COMPRESSOR_SPEED);
					}
					set_speed(0);
					vTaskDelay(TIME * 1000 / portTICK_PERIOD_MS);
					set_valve_2_state(1);
					DUAL_VALVE_STATE = 0;
					break;
				default: break;
    		}
		} else {
			led(0);
			set_speed(0);
			set_valve_1_state(1);
			set_valve_2_state(1);
			set_dual_valve_state(0);
		}
    }
}

