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


#define BOOT_TIME_FOR_DEEP_SLEEP	6500	// Ms Boot time when deep sleep is enabled
#define USAGE_DEEP_SLEEP_SEC		60		// Deep sleep time in seconds
//#define	USAGE_DEEP_SLEEP_SEC		0		// No Deep sleep
//#define USAGE_DEBUG_JSON						// Debug JSON data
#define GRAPH_STEP			1		// Number of steps in the graphDataPionts

#include <Arduino.h>				// Arduino
//#include <esp32-hal.h>
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

#define SCREEN_WIDTH	128			// OLED display width, in pixels
#define SCREEN_HEIGHT	64			// OLED display height, in pixels
#define OLED_RESET		-1			// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS	0x3C		// Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const		uint32_t	GetHttps_Value	= 60 * 1000;	// 1min in ms
static		uint32_t	GetHttps_Timer	= 0UL;

static		uint32_t	Message_Value	= 0U;
static		uint32_t	Message_Timer	= 0UL;

class		WiFiWPS		wifiWPS;		// Define the WiFi WPS class
static		String		OTAHostName		= "ShowBtc";
static		String		OTAPassword		= "pe0fko";

// RTC_DATA_ATTR, 8Kb of RTC memory
RTC_DATA_ATTR	int		graphDataPionts[SCREEN_WIDTH * GRAPH_STEP];
RTC_DATA_ATTR	int		graphUsedLength;
RTC_DATA_ATTR	int		bootCount;

// Forward references
bool		get_https(const char *https_host, String &payload);
void		JsonDecode(const char* json);
void		displayInit();
void		displayTime();
void		displayBTC(int nr, int y0);
void		displayGraph(int nr, int x0, int y0, int w0, int h0);
void		displayMessage(int ms, const char line[]);
void		print_wakeup_reason();

void setup() 
{
	log_i("Setup start\n");
	
	Wire.begin(5, 4);		// SDA, SCL, Wemos LOLIN32 oLED

	bootCount += 1;			// Keep track of the number of boots

	Serial.begin(115200);
	Serial.setDebugOutput(true);
	while(!Serial) ;

//	Serial.printf("=== %d\n", *(int*)0);

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
	Serial.flush();
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
#ifdef DEBUG
//					Serial.printf("[HTTPS] GET: %s\n", payload.c_str());
//					Serial.flush();
#endif
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
	display.drawRect(0,  0, display.width(), display.height()-9     , WHITE);
	display.drawRect(0, 24, display.width(), display.height()-24-9, WHITE);


	displayTime();
	displayBTC(btc, 2);
	displayGraph(btc, 1, 23, display.width()-2, display.height() - 24 - 9);

	display.display();
}

void displayTime()
{
	struct tm timeinfo;
	getLocalTime(&timeinfo);
	display.setCursor(0, SCREEN_HEIGHT-8+1);
	display.print(&timeinfo, "%H:%M");
}

void displayGraph(int nr, int x0, int y0, int w0, int h0)
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

	// Find min and max values
	int min = 10000000;
	int max = 0;
	for (int i = 0; i < graphUsedLength; i++) 
	{
		if (graphDataPionts[i] > max) max = graphDataPionts[i];
		if (graphDataPionts[i] < min) min = graphDataPionts[i];
	}
	int range = max - min + 1;
//	Serial.printf("BLOCK: range=%d min=%d max=%d\n", range, min, max);

	display.setCursor(7*6, SCREEN_HEIGHT-8);
	display.printf("[%d]", range);

//	Serial.printf("BTC: %d, Range: %d - (min=%d - max=%d)\n", nr, range, min, max);
//	Serial.printf("BLOCK: x0=%d y0=%d w0=%d h0=%d\n", x0, y0, w0, h0);

	for (int i = 0; i < graphUsedLength; i += 1) 
	{
		int x = x0 + i * w0 / graphArrayLength;
		int y = graphDataPionts[i] - min - 1;		// y = 0..range
		y = y * h0 / range;							// y = 0..h0
		y = (y0+h0) - y;

//		Serial.printf("PIXEL%03d x=%d y=%d\n", i, x, y);
		display.drawPixel(x, y, SSD1306_WHITE);
	}
}

void displayBTC(int nr, int y0)
{
	// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setTextSize( 3 );

	int t = 18;
	int w1 = nr >= 1000000 ? 4*t : nr >= 100000 ? 3*t : nr >= 10000 ? 2*t : nr >= 1000 ? 1*t : 0*t;
	int w2 = 6;
	int w3 = 3*t;
//	const int y  = 0;

	int x = (display.width() - (w1 + w2 + w3)) / 2;
	if (x < 0) x = 0;
	display.setCursor(x, y0);
	display.printf("%d", nr / 1000);

	// Draw dot
	x += w1;
	display.drawFastHLine(x, y0+24-5-2, 3, SSD1306_WHITE);
	display.drawFastHLine(x, y0+24-4-2, 3, SSD1306_WHITE);
	display.drawFastHLine(x, y0+24-3-2, 3, SSD1306_WHITE);

	x += w2;
	display.setCursor(x, y0);
	display.printf("%03d", nr % 1000);

	display.setTextSize(1);
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
			display.setCursor(0, 1*8);	display.printf("== Show BTC Price ==");
			display.setCursor(0, 2*8);	display.printf(" PE0FKO:" __DATE__);
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
