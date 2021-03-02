/**
 *
 * \file
 *
 * \brief HTTP File Downloader Example.
 *
 * Copyright (c) 2016-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/** \mainpage
 * \section intro Introduction
 * This example demonstrates how to connect to an HTTP server and download
 * a file using HTTP client.<br>
 * It uses the following hardware:
 * - the SAM Xplained Pro.
 * - the WINC1500 on EXT1.
 * - the IO1 Xplained Pro on EXT2.
 *
 * \section files Main Files
 * - main.c : Initialize the SD card and WINC1500 Wi-Fi module, then download 
 * a file using HTTP client.
 *
 * \section usage Usage
 * -# Configure below code in the main.h for AP information to be connected.
 * \code
 *    #define MAIN_WLAN_SSID         "DEMO_AP"
 *    #define MAIN_WLAN_AUTH         M2M_WIFI_SEC_WPA_PSK
 *    #define MAIN_WLAN_PSK          "12345678"
 * \endcode
 *
 * -# Configure HTTP URL macro in the main.h file.
 * \code
 *    #define MAIN_HTTP_FILE_URL                   "http://www.microchip.com/Images/45093A-SmartConnectWINC1500_E_US_101014_web.pdf"
 * \endcode
 *
 * -# Build the program and download it into the board.
 * -# On the computer, open and configure a terminal application as following.
 * \code
 *    Baud Rate : 115200
 *    Data : 8bit
 *    Parity bit : none
 *    Stop bit : 1bit
 *    Flow control : none
 * \endcode
 *
 * -# Start the application.
 * -# In the terminal window, the following text should appear:<br>
 *
 * \code
 *    -- HTTP file downloader example --
 *    -- SAMXXX_XPLAINED_PRO --
 *    -- Compiled: xxx xx xxxx xx:xx:xx --
 *
 *    This example requires the AP to have internet access.
 *
 *    init_storage: please plug an SD/MMC card in slot...
 *    init_storage: mounting SD card...
 *    init_storage: SD card mount OK.
 *    main: connecting to WiFi AP DEMO_AP...
 *    wifi_cb: M2M_WIFI_CONNECTED
 *    wifi_cb: IP address is 192.168.1.107
 *    start_download: sending HTTP request...
 *    resolve_cb: www.atmel.com IP address is 72.246.56.186
 *
 *    http_client_callback: HTTP client socket connected.
 *    http_client_callback: request completed.
 *    http_client_callback: received response 200 data size 1147097
 *    store_file_packet: creating file [0:45093A-SmartConnectWINC1500_E_US_101014_web.pdf]
 *    store_file_packet: received[xxx], file size[1147097]
 *    ...
 *    store_file_packet: received[1147097], file size[1147097]
 *    store_file_packet: file downloaded successfully.
 *    main: please unplug the SD/MMC card.
 *    main: done.
 * \endcode
 *
 * \section compinfo Compilation Information
 * This software was written for the GNU GCC compiler using Atmel Studio 6.2
 * Other compilers may or may not work.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.microchip.com">Microchip</A>.\n
 */

#include <errno.h>
#include "asf.h"
#include "main.h"
#include "stdio_serial.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "iot/http/http_client.h"

#define STRING_EOL                      "\r\n"
#define STRING_HEADER                   "-- WINC1500 HTTP Client example --"STRING_EOL \
	"-- "BOARD_NAME " --"STRING_EOL	\
	"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL

/** File download processing state. */
static download_state down_state = NOT_READY;
/** SD/MMC mount. */
static FATFS fatfs;
/** File pointer for file download. */
static FIL file_object;
/** Http content length. */
static uint32_t http_file_size = 0;
/** Receiving content length. */
static uint32_t received_file_size = 0;
/** File name to download. */
static char save_file_name[MAIN_MAX_FILE_NAME_LENGTH + 1] = "0:";

/** UART module for debug. */
static struct usart_module cdc_uart_module;

/** Instance of Timer module. */
struct sw_timer_module swt_module_inst;

/** Instance of HTTP client module. */
struct http_client_module http_client_module_inst;

char http_url[100];
/**
 * \brief Initialize download state to not ready.
 */
static void init_state(void)
{
	down_state = NOT_READY;
}

/**
 * \brief Clear state parameter at download processing state.
 * \param[in] mask Check download_state.
 */
static void clear_state(download_state mask)
{
	down_state &= ~mask;
}

/**
 * \brief Add state parameter at download processing state.
 * \param[in] mask Check download_state.
 */
static void add_state(download_state mask)
{
	down_state |= mask;
}

/**
 * \brief File download processing state check.
 * \param[in] mask Check download_state.
 * \return true if this state is set, false otherwise.
 */

static inline bool is_state_set(download_state mask)
{
	return ((down_state & mask) != 0);
}

/**
 * \brief File existing check.
 * \param[in] fp The file pointer to check.
 * \param[in] file_path_name The file name to check.
 * \return true if this file name is exist, false otherwise.
 */
static bool is_exist_file(FIL *fp, const char *file_path_name)
{
	if (fp == NULL || file_path_name == NULL) {
		return false;
	}

	FRESULT ret = f_open(&file_object, (char const *)file_path_name, FA_OPEN_EXISTING);
	f_close(&file_object);
	return (ret == FR_OK);
}

/**
 * \brief Make to unique file name.
 * \param[in] fp The file pointer to check.
 * \param[out] file_path_name The file name change to uniquely and changed name is returned to this buffer.
 * \param[in] max_len Maximum file name length.
 * \return true if this file name is unique, false otherwise.
 */
static bool rename_to_unique(FIL *fp, char *file_path_name, uint8_t max_len)
{
	#define NUMBRING_MAX (3)
	#define ADDITION_SIZE (NUMBRING_MAX + 1) /* '-' character is added before the number. */
	uint16_t i = 1, name_len = 0, ext_len = 0, count = 0;
	char name[MAIN_MAX_FILE_NAME_LENGTH + 1] = {0};
	char ext[MAIN_MAX_FILE_EXT_LENGTH + 1] = {0};
	char numbering[NUMBRING_MAX + 1] = {0};
	char *p = NULL;
	bool valid_ext = false;

	if (file_path_name == NULL) {
		return false;
	}

	if (!is_exist_file(fp, file_path_name)) {
		return true;
	} 
	else if (strlen(file_path_name) > MAIN_MAX_FILE_NAME_LENGTH) {
		return false;
	}

	p = strrchr(file_path_name, '.');
	if (p != NULL) {
		ext_len = strlen(p);
		if (ext_len < MAIN_MAX_FILE_EXT_LENGTH) {
			valid_ext = true;
			strcpy(ext, p);
			if (strlen(file_path_name) - ext_len > MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE) {
				name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE - ext_len;
				strncpy(name, file_path_name, name_len);
			} 
			else {
				name_len = (p - file_path_name);
				strncpy(name, file_path_name, name_len);
			}
		} 
		else {
			name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE;
			strncpy(name, file_path_name, name_len);
		}
	} 
	else {
		name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE;
		strncpy(name, file_path_name, name_len);
	}

	name[name_len++] = '-';

	for (i = 0, count = 1; i < NUMBRING_MAX; i++) {
		count *= 10;
	}
	for (i = 1; i < count; i++) {
		sprintf(numbering, MAIN_ZERO_FMT(NUMBRING_MAX), i);
		strncpy(&name[name_len], numbering, NUMBRING_MAX);
		if (valid_ext) {
			strcpy(&name[name_len + NUMBRING_MAX], ext);
		}

		if (!is_exist_file(fp, name)) {
			memset(file_path_name, 0, max_len);
			strcpy(file_path_name, name);
			return true;
		}
	}
	return false;
}

/**
 * \brief Start file download via HTTP connection.
 */
static void start_download(void)
{
	if (!is_state_set(STORAGE_READY)) {
		printf("start_download: MMC storage not ready.\r\n");
		return;
	}

	if (!is_state_set(WIFI_CONNECTED)) {
		printf("start_download: Wi-Fi is not connected.\r\n");
		return;
	}

	if (is_state_set(GET_REQUESTED)) {
		printf("start_download: request is sent already.\r\n");
		return;
	}

	if (is_state_set(DOWNLOADING)) {
		printf("start_download: running download already.\r\n");
		return;
	}

	/* Send the HTTP request. */
	printf("start_download: sending HTTP request...\r\n");
	http_client_send_request(&http_client_module_inst, MAIN_HTTP_FILE_URL, HTTP_METHOD_GET, NULL, NULL);
}

struct http_entity g_http_entity = {0,};
struct http_entity *_example_http_set_default_entity(void);
const char*			_example_http_get_contents_type(void *priv_data);
int					_example_http_get_contents_length(void *priv_data);
int					_example_http_read(void *priv_data, char *buffer, uint32_t size, uint32_t written);
int					_example_http_read_file(void *priv_data, FIL* file, char *buffer, uint32_t size, uint32_t written);
void				_example_http_close(void *priv_data);

struct http_entity * _example_http_set_default_entity()
{
	memset(&g_http_entity, 0x00, sizeof(struct http_entity));
	g_http_entity.close = _example_http_close;
	g_http_entity.file_format = HTTP_FILE_FORMAT_NONE;
	g_http_entity.is_chunked = 0;
	g_http_entity.priv_data = NULL;
	g_http_entity.read = _example_http_read;
	g_http_entity.read_file = _example_http_read_file;
	g_http_entity.get_contents_length = _example_http_get_contents_length;
	g_http_entity.get_contents_type = _example_http_get_contents_type;
	
	return &g_http_entity;
}

const char* _example_http_get_contents_type(void *priv_data)
{
	if (strstr(priv_data, EXAMPLE_HTTP_CONTENT_BOUNDARY) != NULL)
		return (const char*)EXAMPLE_HTTP_FORM_DATA_CONTENT_TYPE;
	else
		return (const char*)EXAMPLE_HTTP_FORM_URLENCODED_CONTENT_TYPE;
}

int _example_http_get_contents_length(void *priv_data)
{
	return strlen( (char*)priv_data);
}

int _example_http_read(void *priv_data, char *buffer, uint32_t size, uint32_t written)
{
	int32_t length = 0;
	
	if(priv_data)
	{
		length = strlen( (char*)priv_data);
		memcpy(buffer,(char*)priv_data, length);
	}
	
	return length;
}

int _example_http_read_file(void *priv_data, FIL* file, char *buffer, uint32_t size, uint32_t written)
{
	int32_t length = 0;
	unsigned byte_read = 0;
	FRESULT res;
	static read_length = 0;

	memset(buffer, 0, size);
	
	if (file->fsize == read_length)
	{
		sprintf(buffer,"%s%s%s", "\r\n", EXAMPLE_HTTP_CONTENT_BOUNDARY, "--\r\n");
		return strlen(buffer);
	}
	// send private data before the file data
	if (read_length == 0)
	{
		length = strlen( (char*)priv_data);
		memcpy(buffer,(char*)priv_data, length);
		buffer += length;
		size -= length;
	}
	
	if ((file->fsize - read_length) == 0)
		return 0;
	
	
	
	if ((file->fsize - read_length) >= size)
		res = f_read(file, buffer, size, &byte_read);
	else
		res = f_read(file, buffer, file->fsize - read_length, &byte_read);
		
	if (res != FR_OK) {
		printf("-E- f_read pb: 0x%X\n\r", res);
		return 0;
	}
	
	read_length += byte_read;
	
	if (length > 0)
		byte_read += length;
	
	
		
	printf("file.fsize=%d\r\n", file->fsize);
	printf("read_length=%d\r\n", read_length);
	printf("byte_read=%d\r\n", byte_read);

	return byte_read;
}

void _example_http_close(void *priv_data)
{
}
/**
 * \brief Start file download via HTTP connection.
 */
char send_buf[400];
static void start_upload_file(TCHAR* file_name, fileFormat file_format, char* key, char* value)
{
	//TCHAR file_name[30] = "2014_09_15_01_33_29.fit";
	FRESULT res;
		
	if (!is_state_set(STORAGE_READY)) {
		printf("start_upload_file: MMC storage not ready.\r\n");
		return;
	}

	if (!is_state_set(WIFI_CONNECTED)) {
		printf("start_upload_file: Wi-Fi is not connected.\r\n");
		return;
	}

	if (is_state_set(GET_REQUESTED)) {
		printf("start_upload_file: request is sent already.\r\n");
		return;
	}

	if (is_state_set(DOWNLOADING)) {
		printf("start_upload_file: running download already.\r\n");
		return;
	}

	/* Send the HTTP request. */
	printf("start_upload_file: sending HTTP request...\r\n");
	
	
	
	//sprintf(send_buf,"%s%s%s%s%s%s", EXAMPLE_HTTP_CONTENT_BOUNDARY,"\r\nContent-Disposition: form-data; name=\"memberid\"\r\n\r\n", "RXtYOSoAZk36u9kOxIN+sQ==\r\n", EXAMPLE_HTTP_CONTENT_BOUNDARY, "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n", "Content-Type: application/octet-stream\r\n\r\n");
	
	if (file_format == HTTP_FILE_FORMAT_NONE)
		sprintf(send_buf,"%s%s%s%s%s%s%s", EXAMPLE_HTTP_CONTENT_BOUNDARY, "\r\nContent-Disposition: form-data; name=\"", key, "\"\r\n\r\n", value, "\r\n", EXAMPLE_HTTP_CONTENT_BOUNDARY);
	else if (file_format == HTTP_FILE_FORMAT_FIT)
		sprintf(send_buf,"%s%s%s%s%s%s%s", EXAMPLE_HTTP_CONTENT_BOUNDARY, "\r\nContent-Disposition: form-data; name=\"", key, "\"; filename=\"", file_name, "\"\r\n", "Content-Type: application/octet-stream\r\n\r\n");
	else
		sprintf(send_buf,"%s%s%s%s%s%s%s", EXAMPLE_HTTP_CONTENT_BOUNDARY, "\r\nContent-Disposition: form-data; name=\"", key, "\"; filename=\"", file_name, "\"\r\n", "Content-Type: text/plain\r\n\r\n");
	
	struct http_entity * entity = _example_http_set_default_entity();	
	entity->file_format = file_format;
	entity->priv_data = (void*)send_buf;
	//printf("send_buf=%s, len=%d\r\n", entity->priv_data, strlen(entity->priv_data));
	
	if (file_format != HTTP_FILE_FORMAT_NONE)
	{
		res = f_open(&entity->file_object, (char const *)file_name,
		FA_OPEN_EXISTING | FA_READ);
		if (res != FR_OK) {
			printf("-E- f_open read pb: 0x%X\n\r", res);
			return 0;
		}
	}
	
	http_client_send_request(&http_client_module_inst, MAIN_HTTP_POST_URL, HTTP_METHOD_POST, entity, NULL);
}


static void prepare_url_parameter(char para_name[KEY_NUM_MAX][VALUE_LEN_MAX], char para_val[KEY_NUM_MAX][VALUE_LEN_MAX], int para_num)
{
	char para_str[60];
	memset(para_str, 0 , sizeof(para_str));
	int i= 0;

	for (i=0; i< para_num; i++)
	{
		if (i>0)
			sprintf(para_str,"%s&",para_str);
		sprintf(para_str,"%s%s=%s", para_str, &para_name[i], &para_val[i]);
	}
	sprintf(http_url,"%s?%s", MAIN_HTTP_POST_URL, para_str);
	printf("http_url = %s\r\n", http_url);
}

char body_str[KEY_NUM_MAX* (KEY_LEN_MAX + VALUE_LEN_MAX)];

static void start_post_data(char key[KEY_NUM_MAX][KEY_LEN_MAX], char value[KEY_NUM_MAX][VALUE_LEN_MAX], int key_num)
{
	FRESULT res;
	int i = 0;
	

	if (!is_state_set(WIFI_CONNECTED)) {
		printf("start_post_data: Wi-Fi is not connected.\r\n");
		return;
	}

	if (is_state_set(GET_REQUESTED)) {
		printf("start_post_data: request is sent already.\r\n");
		return;
	}

	if (is_state_set(DOWNLOADING)) {
		printf("start_post_data: running download already.\r\n");
		return;
	}
	
	/* Send the HTTP request. */
	printf("start_post_data: sending HTTP request...\r\n");
	
	if (key_num > 0)
	{
		for (i=0; i< key_num; i++)
		{
			if (i>0)
			sprintf(body_str,"%s&",body_str);
			sprintf(body_str,"%s%s=%s", body_str, &key[i], &value[i]);
		}

		struct http_entity * entity = _example_http_set_default_entity();
		entity->priv_data = (void*)body_str;
		//printf("send_buf=%s, len=%d\r\n", entity->priv_data, strlen(entity->priv_data));
		
		http_client_send_request(&http_client_module_inst, http_url, HTTP_METHOD_POST, entity, NULL);
	}
	else
	{
		http_client_send_request(&http_client_module_inst, http_url, HTTP_METHOD_POST, NULL, NULL);
	}
	
}

/**
 * \brief Store received packet to file.
 * \param[in] data Packet data.
 * \param[in] length Packet data length.
 */
static void store_file_packet(char *data, uint32_t length)
{
	FRESULT ret;
	if ((data == NULL) || (length < 1)) {
		printf("store_file_packet: empty data.\r\n");
		return;
	}

	if (!is_state_set(DOWNLOADING)) {
		char *cp = NULL;
		save_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		save_file_name[1] = ':';
		cp = (char *)(MAIN_HTTP_FILE_URL + strlen(MAIN_HTTP_FILE_URL));
		while (*cp != '/') {
			cp--;
		}
		if (strlen(cp) > 1) {
			cp++;
			strcpy(&save_file_name[2], cp);
		} else {
			printf("store_file_packet: file name is invalid. Download canceled.\r\n");
			add_state(CANCELED);
			return;
		}

		rename_to_unique(&file_object, save_file_name, MAIN_MAX_FILE_NAME_LENGTH);
		printf("store_file_packet: creating file [%s]\r\n", save_file_name);
		ret = f_open(&file_object, (char const *)save_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (ret != FR_OK) {
			printf("store_file_packet: file creation error! ret:%d\r\n", ret);
			return;
		}

		received_file_size = 0;
		add_state(DOWNLOADING);
	}

	if (data != NULL) {
		UINT wsize = 0;
		ret = f_write(&file_object, (const void *)data, length, &wsize);
		if (ret != FR_OK) {
			f_close(&file_object);
			add_state(CANCELED);
			printf("store_file_packet: file write error, download canceled.\r\n");
			return;
		}

		received_file_size += wsize;
		printf("store_file_packet: received[%lu], file size[%lu]\r\n", (unsigned long)received_file_size, (unsigned long)http_file_size);
		if (received_file_size >= http_file_size) {
			f_close(&file_object);
			printf("store_file_packet: file downloaded successfully.\r\n");
			port_pin_set_output_level(LED_0_PIN, false);
			add_state(COMPLETED);
			return;
		}
	}
}

/**
 * \brief Callback of the HTTP client.
 *
 * \param[in]  module_inst     Module instance of HTTP client module.
 * \param[in]  type            Type of event.
 * \param[in]  data            Data structure of the event. \refer http_client_data
 */
static void http_client_callback(struct http_client_module *module_inst, int type, union http_client_data *data)
{
	switch (type) {
	case HTTP_CLIENT_CALLBACK_SOCK_CONNECTED:
		printf("http_client_callback: HTTP client socket connected.\r\n");
		break;

	case HTTP_CLIENT_CALLBACK_REQUESTED:
		printf("http_client_callback: request completed.\r\n");
		add_state(GET_REQUESTED);
		break;

	case HTTP_CLIENT_CALLBACK_RECV_RESPONSE:
		printf("http_client_callback: received response %u data size %u\r\n",
				(unsigned int)data->recv_response.response_code,
				(unsigned int)data->recv_response.content_length);
		if ((unsigned int)data->recv_response.response_code == 200) {
			http_file_size = data->recv_response.content_length;
			received_file_size = 0;
		} 
		else {
			add_state(CANCELED);
			return;
		}
/*		
		if (data->recv_response.content_length > 0)
		{
			printf("response data = \r\n");
			for (int i= 0; i< data->recv_response.content_length; i++)
			printf("%c", data->recv_response.content[i]);
			printf("\r\n\r\n");
		}
*/		
		
		if (data->recv_response.content_length <= MAIN_BUFFER_MAX_SIZE) {
#ifdef STORE_TO_NVM
			store_file_packet(data->recv_response.content, data->recv_response.content_length);
#endif
			add_state(COMPLETED);
		}
		break;

	case HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA:
#ifdef STORE_TO_NVM
		store_file_packet(data->recv_chunked_data.data, data->recv_chunked_data.length);
#endif
		if (data->recv_chunked_data.is_complete) {
			add_state(COMPLETED);
		}

		break;

	case HTTP_CLIENT_CALLBACK_DISCONNECTED:
		printf("http_client_callback: disconnection reason:%d\r\n", data->disconnected.reason);

		/* If disconnect reason is equal to -ECONNRESET(-104),
		 * It means the server has closed the connection (timeout).
		 * This is normal operation.
		 */
		if (data->disconnected.reason == -EAGAIN) {
			/* Server has not responded. Retry immediately. */
			if (is_state_set(DOWNLOADING)) {
				f_close(&file_object);
				clear_state(DOWNLOADING);
			}

			if (is_state_set(GET_REQUESTED)) {
				clear_state(GET_REQUESTED);
			}

			start_download();
		}

		break;
	}
}

/**
 * \brief Callback to get the data from socket.
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	http_client_socket_event_handler(sock, u8Msg, pvMsg);
}

/**
 * \brief Callback for the gethostbyname function (DNS Resolution callback).
 * \param[in] pu8DomainName Domain name of the host.
 * \param[in] u32ServerIP Server IPv4 address encoded in NW byte order format. If it is Zero, then the DNS resolution failed.
 */
static void resolve_cb(uint8_t *pu8DomainName, uint32_t u32ServerIP)
{
	printf("resolve_cb: %s IP address is %d.%d.%d.%d\r\n\r\n", pu8DomainName,
			(int)IPV4_BYTE(u32ServerIP, 0), (int)IPV4_BYTE(u32ServerIP, 1),
			(int)IPV4_BYTE(u32ServerIP, 2), (int)IPV4_BYTE(u32ServerIP, 3));
	http_client_socket_resolve_handler(pu8DomainName, u32ServerIP);
}

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
 *  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
 *  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
 *  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
 *  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
 *  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type. Existing types are:
 *  - tstrM2mWifiStateChanged
 *  - tstrM2MWPSInfo
 *  - tstrM2MP2pResp
 *  - tstrM2MAPResp
 *  - tstrM2mScanDone
 *  - tstrM2mWifiscanResult
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType) {
	case M2M_WIFI_RESP_CON_STATE_CHANGED:
	{
		tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
		if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
			printf("wifi_cb: M2M_WIFI_CONNECTED\r\n");
			m2m_wifi_request_dhcp_client();
		} else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
			printf("wifi_cb: M2M_WIFI_DISCONNECTED\r\n");
			clear_state(WIFI_CONNECTED);
			if (is_state_set(DOWNLOADING)) {
				f_close(&file_object);
				clear_state(DOWNLOADING);
			}

			if (is_state_set(GET_REQUESTED)) {
				clear_state(GET_REQUESTED);
			}

			m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID),
					MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
		}

		break;
	}

	case M2M_WIFI_REQ_DHCP_CONF:
	{
		uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
		printf("wifi_cb: IP address is %u.%u.%u.%u\r\n",
				pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
		add_state(WIFI_CONNECTED);
		
#if defined(TEST_HTTP_GET)
		start_download();
#elif defined(TEST_HTTP_POST_FILE)

		//start_upload_file(NULL, HTTP_FILE_FORMAT_NONE, "key1", "value1");
		//start_upload_file("test.fit", HTTP_FILE_FORMAT_FIT, "key1", "value1");
		start_upload_file("test.txt", HTTP_FILE_FORMAT_TXT, "key1", "value1");
		
#else		// TEST_HTTP_POST_VALUE
		
		char name[2][VALUE_LEN_MAX];
		char value[2][VALUE_LEN_MAX];
		
		strcpy(&name[0], "key1");
		strcpy(&value[0], "value1");
		strcpy(&name[1], "key2");
		strcpy(&value[1], "value2");
		
		prepare_url_parameter(&name, &value, 2);
		start_post_data(&name, &value, 2);
#endif

		break;
	}

	default:
		break;
	}
}

/**
 * \brief Initialize SD/MMC storage.
 */
static void init_storage(void)
{
	FRESULT res;
	Ctrl_status status;

	/* Initialize SD/MMC stack. */
	sd_mmc_init();
	while (true) {
		printf("init_storage: please plug an SD/MMC card in slot...\r\n");

		/* Wait card present and ready. */
		do {
			status = sd_mmc_test_unit_ready(0);
			if (CTRL_FAIL == status) {
				printf("init_storage: SD Card install failed.\r\n");
				printf("init_storage: try unplug and re-plug the card.\r\n");
				while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
				}
			}
		} while (CTRL_GOOD != status);

		printf("init_storage: mounting SD card...\r\n");
		memset(&fatfs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fatfs);
		if (FR_INVALID_DRIVE == res) {
			printf("init_storage: SD card mount failed! (res %d)\r\n", res);
			return;
		}

		printf("init_storage: SD card mount OK.\r\n");
		add_state(STORAGE_READY);
		return;
	}
}

/**
 * \brief Configure UART console.
 */
static void configure_console(void)
{
	struct usart_config usart_conf;

	usart_get_config_defaults(&usart_conf);
	usart_conf.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	usart_conf.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	usart_conf.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	usart_conf.baudrate    = 115200;

	stdio_serial_init(&cdc_uart_module, EDBG_CDC_MODULE, &usart_conf);
	usart_enable(&cdc_uart_module);
}

/**
 * \brief Configure Timer module.
 */
static void configure_timer(void)
{
	struct sw_timer_config swt_conf;
	sw_timer_get_config_defaults(&swt_conf);

	sw_timer_init(&swt_module_inst, &swt_conf);
	sw_timer_enable(&swt_module_inst);
}

/**
 * \brief Configure HTTP client module.
 */
static void configure_http_client(void)
{
	struct http_client_config httpc_conf;
	int ret;

	http_client_get_config_defaults(&httpc_conf);

	httpc_conf.recv_buffer_size = MAIN_BUFFER_MAX_SIZE;
	httpc_conf.timer_inst = &swt_module_inst;

	ret = http_client_init(&http_client_module_inst, &httpc_conf);
	if (ret < 0) {
		printf("configure_http_client: HTTP client initialization failed! (res %d)\r\n", ret);
		while (1) {
		} /* Loop forever. */
	}

	http_client_register_callback(&http_client_module_inst, http_client_callback);
}

/**
 * \brief Main application function.
 *
 * Application entry point.
 *
 * \return program return value.
 */
int main(void)
{
	tstrWifiInitParam param;
	int8_t ret;
	init_state();

	/* Initialize the board. */
	system_init();

	/* Initialize the UART console. */
	configure_console();
	printf(STRING_HEADER);
	printf("\r\nThis example requires the AP to have internet access.\r\n\r\n");

	/* Initialize the Timer. */
	configure_timer();

	/* Initialize the HTTP client service. */
	configure_http_client();

	/* Initialize the BSP. */
	nm_bsp_init();

#ifdef STORE_TO_NVM
	/* Initialize SD/MMC storage. */
	init_storage();
#endif
	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

	/* Initialize Wi-Fi driver with data and status callbacks. */
	param.pfAppWifiCb = wifi_cb;
	ret = m2m_wifi_init(&param);
	if (M2M_SUCCESS != ret) {
		printf("main: m2m_wifi_init call error! (res %d)\r\n", ret);
		while (1) {
		}
	}

	/* Initialize socket module. */
	socketInit();
	/* Register socket callback function. */
	registerSocketCallback(socket_cb, resolve_cb);

	/* Connect to router. */
	printf("main: connecting to WiFi AP %s...\r\n", (char *)MAIN_WLAN_SSID);
	m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);

	while (!(is_state_set(COMPLETED) || is_state_set(CANCELED))) {
		/* Handle pending events from network controller. */
		m2m_wifi_handle_events(NULL);
		/* Checks the timer timeout. */
		sw_timer_task(&swt_module_inst);
	}
#ifdef STORE_TO_NVM
	printf("main: please unplug the SD/MMC card.\r\n");
#endif
	printf("main: done.\r\n");

	while (1) {
	} /* Loop forever. */

	return 0;
}
