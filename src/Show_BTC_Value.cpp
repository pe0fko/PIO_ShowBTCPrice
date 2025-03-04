//=====================================================================
// Show BTC Price from the BitVavo provider.
// 2018/10/14 - Fred Krom, pe0fko.
//
// V0.1	- Initial version
// V1.0	- LOLIN32 OLED version, 128x64 OLED display
//
// Arduino Tools menu:
// Board:		ESP-WROOM-32, Wemos LOLIN32 OLED, and other WSP32
// Processor:	ESP32 Dev Module
//
// Ports used:
//
//=====================================================================


#ifdef DEBUG
  #define	USAGE_DEEP_SLEEP_SEC	0		// No Deep sleep
  #define	GRAPH_SIZE			(30)		// Number of graphDataPionts
#else
  #define	USAGE_DEEP_SLEEP_SEC	60		// Deep sleep time in seconds
  #define	GRAPH_SIZE			(4*60)		// Number of graphDataPionts, 4h
#endif
  //#define USAGE_DEBUG_JSON						// Debug JSON data
  #define BOOT_TIME_FOR_DEEP_SLEEP	6500	// Ms Boot time when deep sleep is enabled

#include <Arduino.h>				// Arduino
#include <WiFi.h>					// WiFi
#include <time.h>					// time
#include <esp_sntp.h>				// sNTP
#include <HTTPClient.h>				// HTTPClient
#include <WiFiClientSecure.h>		// WiFiClientSecure
#include <ArduinoJson.h>			// ArduinoJson
#include <SPI.h>					// SPI
#include <Wire.h>					// I2C
#include <Adafruit_SSD1306.h>		// Adafruit SSD1306 Wemos Mini OLED
#include "certificate.h"			// Root certificate for https definded
#include <WiFiWPS.h>				// WiFi WPS, OTA & sNTP auto config
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>		// Sans, Mono, 9, 12 ,18, 24
#define	BtcFont1		FreeSansBold12pt7b
#define	BtcFont2		FreeSansBold9pt7b

#define SCREEN_WIDTH	128			// OLED display width, in pixels
#define SCREEN_HEIGHT	64			// OLED display height, in pixels
#define OLED_RESET		-1			// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS	0x3C		// Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#ifdef DEBUG
const		uint32_t	GetHttps_Value	= 30 * 1000;	// 1min in ms
#else
const		uint32_t	GetHttps_Value	= 60 * 1000;	// 1min in ms
#endif
static		uint32_t	GetHttps_Timer	= 0UL;

static		uint32_t	Message_Value	= 0U;
static		uint32_t	Message_Timer	= 0UL;

class		WiFiWPS		wifiWPS;		// Define the WiFi WPS class
static		String		OTAHostName		= "ShowBtc";
static		String		OTAPassword		= "pe0fko";

// RTC_DATA_ATTR, 8Kb of RTC memory
RTC_DATA_ATTR	int		graphDataPionts[GRAPH_SIZE];
RTC_DATA_ATTR	int		graphUsedLength;
RTC_DATA_ATTR	int		bootCount;

// Forward references
bool		get_https(const char *https_host, String &payload);
void		JsonDecode(const char* json);
void		displayInit();
void		displayTime();
void		displayBTC(int nr, int y0);
void		displayGraph(int btc, int x0, int x1, int y0, int y1);
void		displayMessage(int ms, const char line[]);
void		print_wakeup_reason();

void setup() 
{
	Wire.begin(5, 4);		// SDA, SCL, Wemos LOLIN32 oLED

	bootCount += 1;			// Keep track of the number of boots

	Serial.begin(115200);
	Serial.setDebugOutput(true);
	while(!Serial) ;
	log_d("Setup start\n");

	Serial.printf("=== PE0FKO: Show BTC Price\n");
	Serial.printf("=== Build: %s %s\n", __DATE__, __TIME__);
	if (bootCount == 1) 
	{
		Serial.printf("=== First boot, clear RTC memory\n");
		memset(graphDataPionts, 0, sizeof(graphDataPionts));
		graphUsedLength = 0;
		displayInit();		
	} else
		Serial.printf("=== Boot %d times\n", bootCount);

	// Enable the WiFi, WPS, sNTP and OTA
	wifiWPS.setOTA( OTAHostName, OTAPassword );
	wifiWPS.begin();
//	wifiWPS.getWifiSsidPsk(ssid, psk);	// Retrieve the WPS SSID/PSK

	GetHttps_Timer = millis() - GetHttps_Value;

	Serial.printf("=== Exit the setup()\n");
}

void loop() 
{
	wifiWPS.handle();

	bool enter_deep_sleep = false;

	if (Message_Value != 0 && ((millis() - Message_Timer) >= Message_Value))	// Clear message after time
	{
		Message_Value = 0;
	}
	else
	if ((WiFi.status() != WL_CONNECTED)) 
	{
		Serial.printf("Error: WiFi status not connect!\n");
		delay(1000);
	}
	else
	if (!wifiWPS.timeNtpIsSet())				// Wait for NTP time sync
	{
#if 1
		/* Wait untill the sNTP is completed and the correct time
		 * is received. De message is only shown after 4 seconds
		 * with a 1 second interval.
		 */
		static int ntp_wait = 0;			// Dont show every time

		if (Message_Value == 0) 
		{
			if (ntp_wait++ == 5)
			{
				Serial.printf("Waiting for the sNTP date and time\n");
				displayMessage(1000, "Wait sNTP");
				ntp_wait = 0;
			}
			else
			{
				Message_Timer = millis();
				Message_Value = 1000;
			}
		}
#endif
	}
	else
	if (millis() - GetHttps_Timer > GetHttps_Value)
	{
		GetHttps_Timer = millis();

//		Process_Timer = millis();
//		Process_Timer -= millis();
//		Serial.printf("Process time: %d ms\n", -Process_Timer);

		String payload;
		if (get_https(https_host, payload))				// Get the API for the BTC price
		{
#if USAGE_DEEP_SLEEP_SEC > 0
			esp_sntp_stop();							// Stop sNTP
			delay(10);									// Wait for the sNTP to stop
			WiFi.disconnect(true);						// Disconnect WiFi for clean restart
			delay(10);									// Wait for the WiFi to disconnect

			JsonDecode(payload.c_str());				// Decode the JSON data, the BTC price, and display it

			Serial.println("Deep sleep enter.......");
			Serial.flush(); 

			esp_sleep_enable_timer_wakeup				// 1 minute in microseconds
			(	(USAGE_DEEP_SLEEP_SEC) * 1000000		// in Sec
			-	(BOOT_TIME_FOR_DEEP_SLEEP) * 1000		// in Ms
			);
			esp_deep_sleep_start();
#else
			Serial.println("Deep sleep not enabled");
			JsonDecode(payload.c_str());				// Decode the JSON data, the BTC price, and display it
#endif
		}
	}
}


bool get_https(const char *https_host, String &payload)
{
	WiFiClientSecure*	client;
	HTTPClient			https;
	bool				ret = false;

	https.setUserAgent("PE0FKO ESP32 ShowBTC");

	if ((client = new WiFiClientSecure) != NULL)
	{
		SET_ROOT_CERTIFICATE_CACERT(client);			// Set the root certificate or setInsecure!    
		if (https.begin(*client, https_host))			// HTTPS
		{
			int httpCode = https.GET();		// start connection and send HTTP header
			if (httpCode > 0) 
			{
				// HTTP header has been send and Server response header has been handled	    
				// file found at server
				if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
				{
					payload = https.getString();
// #ifdef DEBUG
// 					Serial.printf("[HTTPS] GET: %s\n", payload.c_str());
// 					Serial.flush();
// #endif
					ret = true;
				}
			} else {
				Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
				displayMessage(1000, "E: HTTPS GET");
			}
			https.end();
		}
		else 
		{
			Serial.printf("[HTTPS] Unable to connect\n");
			displayMessage(1000, "E: HTTPS connect");
			https.end();
        }
		delete client;
	} 
	else 
	{
		Serial.printf("[HTTPS] Unable to create https client\n");
		displayMessage(1000, "E: HTTPS client");
	}

	return ret;
}

void JsonDecode(const char* json)
{
	DeserializationError error;
	JsonDocument doc;						// Allocate the JSON document

	error = deserializeJson(doc, json);		// Deserialize the JSON document
	if (error) {
		Serial.printf("deserializeJson() failed: %s\n", error.f_str());
		displayMessage(1000, "E: JSON");
		return;
	}

/*  {"market":"BTC-EUR","startTimestamp":1707345544376,"timestamp":1707431944376,
    "open":"41029",
    "openTimestamp":1707345556907,
    "high":"42249",
    "low":"4.089E+4",
    "last":"41945",
    "closeTimestamp":1707431938361,
    "bid":"41945",
    "bidSize":"0.04847245",
    "ask":"41952",
    "askSize":"0.31887",
    "volume":"940.65235665",
    "volumeQuote":"39236728.00683897"}
*/

#ifdef USAGE_DEBUG_JSON
	int   bk_open     = doc["open"];
	int   bk_high     = doc["high"];
	int   bk_low      = doc["low"];
	int   bk_last     = doc["last"];
	int   bk_bid      = doc["bid"];
	float bk_bidSize  = doc["bidSize"];
	int   bk_ask      = doc["ask"];
	float bk_askSize  = doc["askSize"];
	float bk_volume   = doc["volume"];

	static  int   bk_last_prev = 0;
	if (bk_last_prev == 0) bk_last_prev = bk_last;

	Serial.printf(
		"Open:%5d (%5d -- %5d) Last:%5d(%4d),  Bid:%5d(%3d) Vol:%4.2f,  Ask:%5d(%3d) Size:%.4f Vol:%4.2f,  (%.2f)\n",
		bk_open, bk_low, bk_high, bk_last, bk_last-bk_last_prev,
		bk_bid, bk_bid-bk_last, bk_bidSize, 
		bk_ask, bk_ask-bk_last, bk_askSize, bk_volume,
		0.20664391 * bk_last
		);

	bk_last_prev = bk_last;
#endif

	int btc = doc["last"];
	Serial.printf("Value BTC is %d euro.\n", btc);

	displayInit();

	display.setTextColor(SSD1306_WHITE);	// White text
	display.clearDisplay();

	// "BTC-Live"
	display.setCursor(SCREEN_WIDTH-8*6, SCREEN_HEIGHT-8);		// Text RB
	display.printf("BTC-Live");

#ifdef DEBUG
	display.setCursor(40, SCREEN_HEIGHT-2*8);
	display.printf("%d", bootCount);
#endif

	// Draw the boxes
	display.drawRect(0,  0, display.width(), display.height()-9   , SSD1306_WHITE);
	display.drawFastHLine(0, 24, display.width(), SSD1306_WHITE);

	// Box Graph: x0=1, y0=25, x1=126, y1=55

	displayTime();
	displayBTC(btc, 4);
	displayGraph(btc, 1, 25, 126, 53);

	display.display();
}

void displayTime()
{
	struct tm timeinfo;
	getLocalTime(&timeinfo);
	display.setCursor(0, SCREEN_HEIGHT-8+1);
	display.print(&timeinfo, "%H:%M");
}

void displayGraph(int nr, int x0, int y0, int x1, int y1)
{
	const int graphArrayLength = sizeof(graphDataPionts) / sizeof(graphDataPionts[0]);

	// Make room for the new value if needed
	if (graphUsedLength < graphArrayLength) {
		graphDataPionts[graphUsedLength++] = nr;
	} else {
		for(int i = 0; i < graphArrayLength-1; i++) {
			graphDataPionts[i] = graphDataPionts[i+1];
		}
		graphDataPionts[graphArrayLength-1] = nr;
	}

	log_d("BLOCK: x0=%d y0=%d x1=%d y1=%d, AL=%d", x0, y0, x1, y1, graphArrayLength);

	// Find min and max values
	int min = 10000000;
	int max = 0;
	for (int i = 0; i < graphUsedLength; i++) 
	{
		if (graphDataPionts[i] > max) max = graphDataPionts[i];
		if (graphDataPionts[i] < min) min = graphDataPionts[i];
	}

	log_d("BLOCK: min=%d max=%d, range=%d", min, max, max-min);
	if (min == max)	return;

	display.setCursor(7*6, SCREEN_HEIGHT-8);
	display.printf("[%d]", max - min);

	for (int i = 0; i < graphUsedLength; i += 1) 
	{
		int x = map(i, 0, graphArrayLength-1, x0, x1);		// Map the point to the screen graph box
		int y = map(graphDataPionts[i], min, max, y1, y0);	// both x and y. The y is reversed.

		if (graphUsedLength >= graphArrayLength)
			log_d("BLOCK%03d: x=%d, y=%d", i, x, y);

		display.drawPixel(x, y, SSD1306_INVERSE);
	}
}

void displayBTC(int nr, int y0)
{
	int16_t		x1,y1;
	uint16_t	w1,h1,w2,h2;
	char		n1[8], n2[8];

	snprintf(n1, 8, "%d.", nr / 1000);
	snprintf(n2, 8, "%03d", nr % 1000);

	display.setFont(&BtcFont2);
	display.getTextBounds(n2, 20, 20, &x1, &y1, &w2, &h2);

	display.setFont(&BtcFont1);
	display.getTextBounds(n1, 20, 20, &x1, &y1, &w1, &h1);

	Serial.printf("y0=%d, w1=%d, h1=%d, w2=%d, h2=%d\n", y0, w1,h1, w2,h2);

	int x = (display.width() - w1 - w2) / 2;
	if (x < 0) x = 0;

	display.setCursor(x, y0 + h1);
	display.print(n1);

	display.setFont(&BtcFont2);
	display.print(n2);

	display.setFont();
}

void displayInit()
{
	static bool init = false;

	if (!init)
	{
		init = true;

		// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
		if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
			Serial.printf("SSD1306 display initialize failed....\n");
			while(1);
		}

		if (bootCount == 1)
		{
			display.setTextColor(SSD1306_WHITE);	// White text
			display.setTextSize(1);					// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
			display.clearDisplay();
			display.setCursor(0, 1*8);	display.printf("== Show BTC Price");
			display.setCursor(0, 2*8);	display.printf("==PE0FKO:" __DATE__);
			display.setCursor(0, 4*8);	display.printf("== API-URL:\n%s", https_host);
			display.display();
			delay(2000);
		}
	}
}

void displayMessage(int ms, const char line[])
{
	Serial.printf("Message: %s\n", line);

	displayInit();

	Message_Timer = millis();
	Message_Value = ms;

	display.setTextColor(SSD1306_WHITE);	// White text
	display.clearDisplay();
	display.setCursor(0, 2*8);
	display.print(line);
	display.display();
}

 // Needed for the WiFiWPS class
 void message(WiFiEvent_t event, WiFiEventInfo_t& info)
 {
	switch (event)
	{
		case ARDUINO_EVENT_WIFI_STA_START:
			// Serial.printf("MSG: Station started.\n");
			break; 

		case ARDUINO_EVENT_WIFI_STA_STOP:
			// Serial.printf("MSG: Station stop.\n");
			break;

		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
			// Serial.printf("MSG: Connected %.*s\n"
			// 	,	info.wifi_sta_connected.ssid_len
			// 	,	info.wifi_sta_connected.ssid
			// );
			break;

		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			Serial.printf("MSG: WiFi IP: %s, GW: %s, NM: %s\n", 
				IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str(),
				IPAddress(info.got_ip.ip_info.gw.addr).toString().c_str(),
				IPAddress(info.got_ip.ip_info.netmask.addr).toString().c_str()
			);
		break;

		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
			Serial.printf("MSG: WiFi disconnected, ssid: %-.32s, reason: %d, rssi: %d\n",
				info.wifi_sta_disconnected.ssid,
				info.wifi_sta_disconnected.reason,
				info.wifi_sta_disconnected.rssi
				);
		break;

		case ARDUINO_EVENT_WPS_ER_SUCCESS:
			Serial.printf("MSG: WPS Success.\n");
			break;

		case ARDUINO_EVENT_WPS_ER_FAILED:
			Serial.printf("MSG: WPS failed, reason %d.\n",
				info.wps_fail_reason
			);
			break;

		case ARDUINO_EVENT_WPS_ER_TIMEOUT:
			Serial.printf("MSG: WPS timeout.\n");
			break;

		case ARDUINO_EVENT_WPS_ER_PIN:
			Serial.printf("MSG: WPS PIN code %.8s.\n",
				info.wps_er_pin.pin_code
			);
			break;

		case ARDUINO_EVENT_OTA_START:		// **No info struct**
			Serial.printf("MSG: OTA Started.\n");
			break;

		case ARDUINO_EVENT_OTA_END:			// **No info struct**
			Serial.printf("MSG: OTA Ended, REBOOT.\n");
			break;

		default:
			break;
	}
 }
