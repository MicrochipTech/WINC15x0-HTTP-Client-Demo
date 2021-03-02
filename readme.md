---
title: WINC1500 HTTP POST/Get Example Project
---

# WINC1500 HTTP POST/Get Example Project

This is an example project code of HTTP Post/ Get feature on WINC1500 Wi-Fi module

## Description

This reprosoitry is WINC1500 HTTP Post/Get example project code. This demonstrate the use of HTTP Get/ Post Method to download or upload data to the HTTP server through WINC1500 Wi-Fi module.   The HTTP POST include the data upload using HTTP content-type "application/x-www-form-urlencoded" and the file upload using HTTP content-type "multipart/form-data"  
The project code is running on SAMD21 XPRO board and WINC1500 XPOR board.  



## Configuration
### HTTP Configuration
The demo can be configured to download a file using HTTP GET, upload test data or upload file using HTTP Post.  
You can change the macro defination in **main.h** to select the HTTP METHOD 
```
//#define TEST_HTTP_GET
//#define TEST_HTTP_POST_FILE
#define TEST_HTTP_POST_VALUE
```

POST Method URL can be configured in below defination in **main.h**
```
#define MAIN_HTTP_POST_URL                   "http://xxxxxxxxxxxxxxx"
```

### Wi-Fi Configuration

You can set the SSID and password in **main.h** to provision the WINC1500 device to the target network
```
/** Wi-Fi AP Settings. */
#define MAIN_WLAN_SSID                       "Demo_AP" /**< Destination SSID */
#define MAIN_WLAN_AUTH                       M2M_WIFI_SEC_WPA_PSK /**< Security manner */
#define MAIN_WLAN_PSK                        "12345678" /**< Password for Destination SSID */
```

## Hardware Setup

SAMD21 XPRO board and WINC1500 XPOR board are needed to run the demo.
Beside, IO1 XPRO Extension Kit with micro SD card is also needed if you are testing HTTP Get (TEST_HTTP_GET) or HTTP Post File (TEST_HTTP_POST_FILE)

You can order [ATWINC1500-XSTK Xplained Pro starter kit](https://www.microchip.com/DevelopmentTools/ProductDetails/ATWINC1500-XSTK) from microchip website. This kit include all the hardware needed for this demo.  

### Connection
1. Plug the WINC1500-XPRO into the SAMD21-XPRO board in the EXT1 location.
2. Plug the IO1-XPRO into the SAMD21-XPRO board in the EXT2 location.
3. Connect the DEBUG USB port of the SAMD21-XPRO board to the host computer using a Type A to Micro B USB Cable



## Build and Test

1. Lauch [Atmel Studio](https://www.microchip.com/mplab/avr-support/avr-and-sam-downloads-archive) 
2. Click **File** > **Open** > **Project**, select **WINC1500_HTTP_EXAMPLE.cproj**
3. Click **Debug** > **Start Without Debugging** to build the project and download the program to the EVB
4. Start up a UART terminal emulator like TeraTerm and open the EDBG COM Port connection with the following settings:
  - Baud rate: 115200
  - Data: 8 bit
  - Parity: None
  - Stop bits: 1
  - Flow control: None 
5. After program firmware successfully, the below log is shown in the SAMD21 console:
```
-- WINC1500 HTTP Client example --
-- SAMD21_XPLAINED_PRO --
-- Compiled: Mar  1 2021 18:17:20 --

This example requires the AP to have internet access.

(APP)(INFO)Chip ID 1503a0
(APP)(INFO)DriverVerInfo: 0x13301361
(APP)(INFO)Firmware ver   : 19.7.3 Svnrev 19093
(APP)(INFO)Firmware Build Jan 18 2021 Time 10:58:45
(APP)(INFO)Firmware Min driver ver : 19.3.0
(APP)(INFO)Driver ver: 19.6.1
(APP)(INFO)Driver built at Mar  1 2021  17:57:15
main: connecting to WiFi AP Demo_AP...
wifi_cb: M2M_WIFI_CONNECTED
wifi_cb: IP address is 172.20.10.10
http_url = http://xxxxxxxxxxxxxxx
start_post_data: sending HTTP request...
(APP)(INFO)Socket 0 session ID = 1
resolve_cb: webhook.site IP address is 46.4.105.116

http_client_callback: HTTP client socket connected.
user agent=atmel/1.0.2, len=11
http_client_callback: received response 200 data size 0
main: done.

```
    