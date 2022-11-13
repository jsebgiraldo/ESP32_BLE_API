/*
 * http_server.c
 *
 *  Created on: Oct 20, 2021
 *      Author: kjagu
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "sys/param.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_sleep.h"

#include "http_server.h"
#include "wifi_app.h"


#include "driver/uart.h"

#include <stdio.h>
#include <stdlib.h>

#include "user_modulation_wave.h"
#include "user_carrier_wave.h"
#include "user_external_wave.h"
#include "user_timer.h"

static const char TAG[] = "[http_server]";

#define HTTP_DEBUG_ENABLE
#ifdef HTTP_DEBUG_ENABLE
// Tag used for ESP serial console messages
	#define HTTP_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_PURPLE) __VA_ARGS__)
#else
	#define HTTP_DEBUG(...)
#endif

// Max number clients for websocket communication.
static const size_t max_clients = 4;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;


// HTTP server task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;


/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
		.callback = &http_server_fw_update_reset_callback,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

// Embedded files: JQuery, index.html, app.css, app.js and favicon.ico files
extern const uint8_t jquery_3_3_1_min_js_start[]	asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[]		asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]				asm("_binary_index_html_start");
extern const uint8_t index_html_end[]				asm("_binary_index_html_end");
extern const uint8_t app_css_start[]				asm("_binary_app_css_start");
extern const uint8_t app_css_end[]					asm("_binary_app_css_end");
extern const uint8_t app_js_start[]					asm("_binary_app_js_start");
extern const uint8_t app_js_end[]					asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[]			asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]				asm("_binary_favicon_ico_end");

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;

	char *data;
};
	

void http_ws_server_send_messages(char * data)
{

	if (!http_server_handle) { // httpd might not have been created by now
		return;
	}

	size_t clients = max_clients;
    int    client_fds[max_clients];
	if (httpd_get_client_list(http_server_handle, &clients, client_fds) == ESP_OK) 
	{
		    for (size_t i=0; i < clients; ++i) 
			{
                int sock = client_fds[i];
                if (httpd_ws_get_fd_info(http_server_handle, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
                 
                    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
                    resp_arg->hd = http_server_handle;
                    resp_arg->fd = sock;
					resp_arg->data = data;


					httpd_handle_t hd = resp_arg->hd;
					int fd = resp_arg->fd;
					const char * data = resp_arg->data;;
					httpd_ws_frame_t ws_pkt;
					memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
					ws_pkt.payload = (uint8_t*)data;
					ws_pkt.len = strlen(data);
					ws_pkt.type = HTTPD_WS_TYPE_TEXT;

					httpd_ws_send_frame_async(hd, fd, &ws_pkt);
					free(resp_arg);

                }
            }
	} 
	else 
	{
        ESP_LOGE(TAG, "httpd_get_client_list failed!");
        return;
    }

}


/**
 * Checks the g_fw_update_status and creates the fw_update_reset timer if g_fw_update_status is true.
 */
static void http_server_fw_update_reset_timer(void)
{
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		HTTP_DEBUG("http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	}
	else
	{
		HTTP_DEBUG("http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}

/**
 * HTTP server monitor task used to track events of the HTTP server
 * @param pvParameters parameter which can be passed to the task.
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for (;;)
	{
		if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch (msg.msgID)
			{
				case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
					HTTP_DEBUG("HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();

					break;

				case HTTP_MSG_OTA_UPDATE_FAILED:
					HTTP_DEBUG("HTTP_MSG_OTA_UPDATE_FAILED");
					g_fw_update_status = OTA_UPDATE_FAILED;

					break;

				default:
					break;
			}
		}
	}
}

/**
 * Jquery get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
	HTTP_DEBUG("Jquery requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

/**
 * Sends the index.html page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
	
	HTTP_DEBUG("index.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

/**
 * app.css get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	HTTP_DEBUG("app.css requested");

	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}

/**
 * app.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	HTTP_DEBUG("app.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}

/**
 * Sends the .ico (icon) file when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
	HTTP_DEBUG("favicon.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

/**
 * Receives the .bin file fia the web page and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be started.
 */
esp_err_t http_server_OTA_update_handler(httpd_req_t *req)
{
	esp_ota_handle_t ota_handle;

	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	do
	{
		// Read the data for the request
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
		{
			// Check if timeout occurred
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
			{
				HTTP_DEBUG("http_server_OTA_update_handler: Socket Timeout");
				continue; ///> Retry receiving if timeout occurred
			}
			HTTP_DEBUG("http_server_OTA_update_handler: OTA other Error %d", recv_len);
			return ESP_FAIL;
		}
		printf("http_server_OTA_update_handler: OTA RX: %d of %d\r", content_received, content_length);

		// Is this the first data we are receiving
		// If so, it will have the information in the header that we need.
		if (!is_req_body_started)
		{
			is_req_body_started = true;

			// Get the location of the .bin file content (remove the web form data)
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
			int body_part_len = recv_len - (body_start_p - ota_buff);

			printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				printf("http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%x\r\n", update_partition->subtype, update_partition->address);
			}

			// Write this first part of the data
			esp_ota_write(ota_handle, body_start_p, body_part_len);
			content_received += body_part_len;
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			content_received += recv_len;
		}

	} while (recv_len > 0 && content_received < content_length);

	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			HTTP_DEBUG("http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
			flash_successful = true;
		}
		else
		{
			HTTP_DEBUG("http_server_OTA_update_handler: FLASHED ERROR!!!");
		}
	}
	else
	{
		HTTP_DEBUG("http_server_OTA_update_handler: esp_ota_end ERROR!!!");
	}

	// We won't update the global variables throughout the file, so send the message about the status
	if (flash_successful) { http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL); } else { http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED); }

	return ESP_OK;
}

/**
 * OTA status handler responds with the firmware update status after the OTA update is started
 * and responds with the compile time/date when the page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
esp_err_t http_server_OTA_status_handler(httpd_req_t *req)
{
	char otaJSON[100];

	HTTP_DEBUG("OTAstatus requested");

	sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, otaJSON, strlen(otaJSON));

	return ESP_OK;
}


static esp_err_t http_start_treatment(httpd_req_t *req)
{
	HTTP_DEBUG("start.json requested");

	
	//pwm_carrier_wave_start();

	modulation_wave_config_t wave_config;

	mcpwm_config_t pwm_config = {
		.counter_mode = MCPWM_UP_COUNTER,
		.duty_mode = MCPWM_DUTY_MODE_0,
	};

	mcpwm_config_t external_wave = {  
		.cmpr_a = 0,                                               
		.counter_mode = MCPWM_UP_COUNTER,
		.duty_mode = MCPWM_DUTY_MODE_0,
	};


	size_t len_json = 0;
	char *json_str = NULL;

	len_json = httpd_req_get_hdr_value_len(req, "frequency") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "frequency", json_str, len_json) == ESP_OK)
		{
			pwm_config.frequency = atoi(json_str);
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "ppw") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "ppw", json_str, len_json) == ESP_OK)
		{
			pwm_config.cmpr_a = atoi(json_str);
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "npw") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "npw", json_str, len_json) == ESP_OK)
		{
			pwm_config.cmpr_b = atoi(json_str);
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "T1") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "T1", json_str, len_json) == ESP_OK)
		{
			wave_config.T1 = atoi(json_str);
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "T2") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "T2", json_str, len_json) == ESP_OK)
		{
			wave_config.T2 = atoi(json_str);
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "T3") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "T3", json_str, len_json) == ESP_OK)
		{
			wave_config.T3 = atoi(json_str);
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "intensity") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "intensity", json_str, len_json) == ESP_OK)
		{
			wave_config.max_intensity = atoi(json_str)*2.55;
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "time_treatment") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "time_treatment", json_str, len_json) == ESP_OK)
		{
			timer_treatmnet_change_period(atoi(json_str));
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "ex_frequency") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "ex_frequency", json_str, len_json) == ESP_OK)
		{
			external_wave.frequency = atoi(json_str);
		}
	}

	len_json = httpd_req_get_hdr_value_len(req, "ppws") + 1;
	if (len_json > 1)
	{
		json_str = malloc(len_json);
		if (httpd_req_get_hdr_value_str(req, "ppws", json_str, len_json) == ESP_OK)
		{
			external_wave.cmpr_b = atoi(json_str);
		}
	}
	free(json_str);

	
	dac_modulation_wave_configure(&wave_config);
	dac_modulation_wave_start();

	pwm_carrier_wave_configure(&pwm_config);
	external_wave_config(&external_wave);
	pwm_carrier_wave_start();
	external_wave_start();

	timer_treatmnet_start();
	deep_sleep_timer_stop();

	char req_start[50];
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, req_start, strlen(req_start));

	return ESP_OK;
}

static esp_err_t http_stop_treatment(httpd_req_t *req)
{
	HTTP_DEBUG("stop.json requested");
	pwm_carrier_wave_stop();
	dac_modulation_wave_stop();
	external_wave_stop();

	char req_stop[50];
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, req_stop, strlen(req_stop));
	return ESP_OK;
}

static esp_err_t http_sleep(httpd_req_t *req)
{
	HTTP_DEBUG("sleep.json requested");

	char req_stop[50];
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, req_stop, strlen(req_stop));

	esp_deep_sleep_start();
	return ESP_OK;
}

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t ws_handler(httpd_req_t *req)
{
	uint8_t buf[128] = { 0 };
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }
    HTTP_DEBUG("Got packet with message: %s", ws_pkt.payload);
    HTTP_DEBUG("Packet type: %d", ws_pkt.type);


    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    return ret;
}

/**
 * Sets up the default httpd server configuration.
 * @return http server instance handle if successful, NULL otherwise.
 */
static httpd_handle_t http_server_configure(void)
{
	// Generate the default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Create HTTP server monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor,HTTP_SERVER_MONITOR_CORE_ID);

	// Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	// The core that the HTTP server will run on
	config.core_id = HTTP_SERVER_TASK_CORE_ID;

	// Adjust the default priority to 1 less than the wifi application task
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	// Bump up the stack size (default is 4096)
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	// Increase uri handlers
	config.max_uri_handlers = 25;

	// Increase the timeout limits
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	config.max_open_sockets = max_clients;

	HTTP_DEBUG("http_server_configure: Starting server on port: '%d' with task priority: '%d'",
			config.server_port,
			config.task_priority);

	// Start the httpd server
	if (httpd_start(&http_server_handle, &config) == ESP_OK)
	{
		HTTP_DEBUG("http_server_configure: Registering URI handlers");

		// register query handler
		httpd_uri_t jquery_js = {
				.uri = "/jquery-3.3.1.min.js",
				.method = HTTP_GET,
				.handler = http_server_jquery_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		// register index.html handler
		httpd_uri_t index_html = {
				.uri = "/",
				.method = HTTP_GET,
				.handler = http_server_index_html_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &index_html);

		// register app.css handler
		httpd_uri_t app_css = {
				.uri = "/app.css",
				.method = HTTP_GET,
				.handler = http_server_app_css_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_css);

		// register app.js handler
		httpd_uri_t app_js = {
				.uri = "/app.js",
				.method = HTTP_GET,
				.handler = http_server_app_js_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_js);

		// register favicon.ico handler
		httpd_uri_t favicon_ico = {
				.uri = "/favicon.ico",
				.method = HTTP_GET,
				.handler = http_server_favicon_ico_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);

		// register OTAupdate handler
		httpd_uri_t OTA_update = {
				.uri = "/OTAupdate",
				.method = HTTP_POST,
				.handler = http_server_OTA_update_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		// register OTAstatus handler
		httpd_uri_t OTA_status = {
				.uri = "/OTAstatus",
				.method = HTTP_POST,
				.handler = http_server_OTA_status_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_status);

		// register OpenDoor.json handler
		httpd_uri_t start_treatment_json = {
				.uri = "/start.json",
				.method = HTTP_POST,
				.handler = http_start_treatment,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &start_treatment_json);

		// register OpenDoor.json handler
		httpd_uri_t stop_treatment_json = {
				.uri = "/stop.json",
				.method = HTTP_GET,
				.handler = http_stop_treatment,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &stop_treatment_json);

			// register OpenDoor.json handler
		httpd_uri_t sleep_json = {
				.uri = "/sleep.json",
				.method = HTTP_GET,
				.handler = http_sleep,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &sleep_json);

		httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true,
		.handle_ws_control_frames = true
		};
		httpd_register_uri_handler(http_server_handle, &ws);
	
		return http_server_handle;
	}

	return NULL;
}

void http_server_start(void)
{
	if (http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
	}
}


void http_server_stop(void)
{

	wifi_sta_list_t wifi_sta_list; 
	esp_wifi_ap_get_sta_list(&wifi_sta_list);

	if(wifi_sta_list.num == false) // Only stop server if it's connected one client.
	{
		if (http_server_handle)
		{
			httpd_stop(http_server_handle);
			HTTP_DEBUG("http_server_stop: stopping HTTP server");
			http_server_handle = NULL;
		}
		if (task_http_server_monitor)
		{
			vTaskDelete(task_http_server_monitor);
			HTTP_DEBUG("http_server_stop: stopping HTTP server monitor");
			task_http_server_monitor = NULL;
		}
	}
	
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
	
}

void http_server_fw_update_reset_callback(void *arg)
{
	HTTP_DEBUG("http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}


// ------------------------------------------ * Websocket functions * --------------------------------

static QueueHandle_t ws_queue = NULL;

struct AMessage
{
 char ucMessageID;
 char ucData[255];
} xMessage;


struct AMessage *pxMessage;


void ws_print(void *pvParameters)
{
	struct AMessage *pxRxedMessage;
	
	for(;;)
	{
		if(xQueueReceive(ws_queue, &( pxRxedMessage ), (portTickType)portMAX_DELAY))
		{
			http_ws_server_send_messages(pxRxedMessage->ucData);
		}
	}
	
}


int http_websocket_vprintf(const char *format, va_list args)
{
	static char vprintf_buff[255];
	int written = vsnprintf(vprintf_buff, sizeof(vprintf_buff), format, args);
	if (written > 0)
	{
		
		if(ws_queue != NULL)
		{
			pxMessage = &xMessage;
			memcpy(pxMessage->ucData,vprintf_buff,sizeof(vprintf_buff));
			xQueueSend( ws_queue, ( void * ) &pxMessage, ( TickType_t ) 0 );
		}
		
		printf(vprintf_buff);
	}

	return written;
}

void log_for_websocket_setup(void)
{
	ws_queue = xQueueCreate(255, sizeof(struct AMessage *)); 

	xTaskCreatePinnedToCore(ws_print, "websocket", 2048, NULL, 12, NULL,0);

	esp_log_set_vprintf(http_websocket_vprintf); 
}
