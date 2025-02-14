//=====================================================================
// Show BTC Price from the BitVavo provider.
// 2018/10/14 - Fred Krom, pe0fko.
//
// V0.1	- Initial version
//
// Arduino Tools menu:
// Board:		ESP-WROOM-32, Wemos D1 R32, and other WSP32
// Processor:	ESP32 Dev Module
//
// Ports used:
//
//=====================================================================

//#define USAGE_DEEP_SLEEP_SEC			60		// Deep sleep time in seconds
#define	USAGE_DEEP_SLEEP_SEC			0		// No Deep sleep
//#define	USAGE_DEEP_SLEEP_SEC			20		// Deep sleep debug time DEBUG
//#define USAGE_DEBUG_JSON			// Debug JSON data
#define GRAPH_STEP			4		// Number of steps in the graphDataPionts

#include <Arduino.h>				// Arduino
#include <WiFi.h>					// WiFi
#include <time.h>					// time
#include <esp_sntp.h>					// sNTP
#include <HTTPClient.h>				// HTTPClient
#include <WiFiClientSecure.h>		// WiFiClientSecure
#include <ArduinoJson.h>			// ArduinoJson
#include <SPI.h>					// SPI
#include <Wire.h>					// I2C
#include <Adafruit_SSD1306.h>		// Adafruit SSD1306 Wemos Mini OLED
#include <ESPmDNS.h>				// mDNS
#include <ArduinoOTA.h>				// OTA
#include <WiFi_SSID.h>				// WiFi SSID and password
#include "certificate.h"			// Root certificate for https definded

#define SCREEN_WIDTH	128			// OLED display width, in pixels
#define SCREEN_HEIGHT	32			// OLED display height, in pixels

#define OLED_RESET		-1			// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS	0x3C		// Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const		uint32_t	Access_Value	= 60UL * 1000;	// 1min in ms
static		uint32_t	Access_Timer	= 0UL;

static		uint32_t	Message_Value	= 0U;
static		uint32_t	Message_Timer	= 0UL;

//static		uint32_t	Process_Timer	= 0UL;
const		uint32_t	Deep_Sleep_Time	= (USAGE_DEEP_SLEEP_SEC) * 1000000;	// 1min in us
static		WiFiMulti	wifiMulti;
static		String		HostName		= "ShowBtc";
volatile	bool		ntp_time_sync	= false;

// RTC_DATA_ATTR, 8Kb of RTC memory
RTC_DATA_ATTR	int		graphDataPionts[SCREEN_WIDTH * GRAPH_STEP];
RTC_DATA_ATTR	int		graphUsedLength;
RTC_DATA_ATTR	int		bootCount;

// Forward references
bool		getHTTPs(const char *https_host, String &payload);
void		JsonDecode(const char* json);
void		displayInit();
void		displayTime();
void		displayBTC(int nr);
void		displayGraph(int nr);
void		displayMessage(int ms, const char line[]);
void		print_wakeup_reason();


#include "sdkconfig.h"
#if CONFIG_ESP_WIFI_REMOTE_ENABLED
#error "WPS is only supported in SoCs with native Wi-Fi support"
#endif



void setup() 
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	while(!Serial) ;

	Serial.printf("=== PE0FKO: Show BTC Price\n");
	Serial.printf("=== Build: %s %s\n", __DATE__, __TIME__);
	Serial.printf("=== Boot : %d times\n", ++bootCount);

	if (bootCount == 1) {
		Serial.printf("=== First boot, clear RTC memory\n");
		memset(graphDataPionts, 0, sizeof(graphDataPionts));
		graphUsedLength = 0;
		displayInit();		
	}

	// set time sync notification call-back function
	sntp_set_time_sync_notification_cb(
		[](struct timeval *t)
		{
//			Serial.println("Got time adjustment from NTP!");
			ntp_time_sync = true;

			struct tm timeinfo;
			getLocalTime(&timeinfo);
			Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
		}
	);

	// Trigger sNTP on the event of WiFi connected
	WiFi.onEvent(
		[](WiFiEvent_t event, WiFiEventInfo_t info) 
		{
			Serial.print("WiFi connected, IP address: ");
			Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));

			// Set the timezone for TZ_Europe_Amsterdam
			setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);
			tzset();
			configTime(0, 0, "time.google.com", "pool.ntp.org" );
			sntp_init();								// Start sNTP
		}
		, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP
	);

	WiFi.mode(WIFI_STA);

	for(uint8_t i = 0; i < WifiApListNumber; i++)
		wifiMulti.addAP(WifiApList[i].ssid, WifiApList[i].passphrase);

	// Start mDNS service
	MDNS.begin(HostName);

	// Start OTA server.
	ArduinoOTA.setHostname(static_cast<const char*>(HostName.c_str()));
	ArduinoOTA.setPassword(OtaPassword);
	ArduinoOTA.begin();

	Access_Timer = millis() - Access_Value;
}


void loop() 
{
//	bool enter_deep_sleep = false;

	ArduinoOTA.handle();

	if (Message_Value != 0 && ((millis() - Message_Timer) >= Message_Value))	// Clear message after time
	{
		Message_Value = 0;
	}
	else
	if ((wifiMulti.run(5000) != WL_CONNECTED)) 
	{
		Serial.printf("Error: WiFI Multi connect, not connected.\n");
		displayMessage(1000, "WiFi Connect");
	}
	else
	if (ntp_time_sync == false)				// Wait for NTP time sync
	{
		/* Wait untill the sNTP is completed and the correct time
		 * is received. De message is only shown after 4 seconds
		 * with a 1 second interval.
		 */
		static int ntp_wait = 0;			// Dont show every time

		if (Message_Value == 0) 
		{
			if (ntp_wait++ == 4)
			{
				displayMessage(1000, "Wait sNTP");
				ntp_wait = 0;
			}
			else
			{
				Message_Timer = millis();
				Message_Value = 1000;
			}
		}
	}
	else
	if (millis() - Access_Timer > Access_Value)
	{
		Access_Timer = millis();

//		Process_Timer = millis();
//		Process_Timer -= millis();
//		Serial.printf("Process time: %d ms\n", -Process_Timer);

		String payload;
		if (getHTTPs(https_host, payload))				// Get the API for the BTC price
		{
#if USAGE_DEEP_SLEEP_SEC > 0
			sntp_stop();								// Stop sNTP
			WiFi.disconnect(true);						// Disconnect WiFi for clean restart

			JsonDecode(payload.c_str());				// Decode the JSON data, the BTC price, and display it

			Serial.println("Deep sleep enter.......");
			Serial.flush(); 

			esp_sleep_enable_timer_wakeup(Deep_Sleep_Time);	// 1 minute in microseconds
			esp_deep_sleep_start();
#else
			JsonDecode(payload.c_str());				// Decode the JSON data, the BTC price, and display it
#endif
		}
	}
}

bool getHTTPs(const char *https_host, String &payload)
{
	HTTPClient https;	        		// Add a scoping block for HTTPClient https to 
											// make sure it is destroyed before WiFiClientSecure *client is 
	WiFiClientSecure *client = new WiFiClientSecure;

//	if (!client) return false;

	if(client) 
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
//					String payload = https.getString();
//					JsonDecode(payload.c_str());
					payload = https.getString();
					return true;
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
        }
		delete client;
	} 
	else 
	{
		Serial.printf("[HTTPS] Unable to create https client\n");
		displayMessage(1000, "E: HTTPS client");
	}

	return false;
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

	displayInit();
	display.setTextColor(SSD1306_WHITE);	// White text
	display.clearDisplay();

	displayTime();
	displayBTC(btc);
	displayGraph(btc);

	display.display();
}

void displayTime()
{
	struct tm timeinfo;
	time_t now = time(nullptr);
	localtime_r(&now, &timeinfo);

	display.setTextSize(1);      // text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(4, 32-8+1);
	display.printf("%2d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

	// Display the boot count, debug info
	display.printf("  [%d]", bootCount);
}


void displayGraph(int nr)
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

	display.setTextSize(1);      			// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(128-3*6-6-4*6, 32-8+1);
	display.printf("%4d", range);

	Serial.printf("BTC: %d, Range: %d - (min=%d - max=%d)\n", nr, range, min, max);

#if 1

	for (int i = 0; i < graphUsedLength; i += 1) 
	{
		int y = graphDataPionts[i] - min;			// y = 0..range
		y = y * SCREEN_HEIGHT / range;	// y = 0..SCREEN_HEIGHT
		y = SCREEN_HEIGHT - y - 1;		// y = SCREEN_HEIGHT..0

		int x = i * SCREEN_WIDTH / graphArrayLength;
//		Serial.printf("PIXEL%03d x=%d y=%d\n", i, x, y);

		display.drawPixel(x, y, SSD1306_INVERSE);
//		display.drawPixel(x, y, SSD1306_WHITE);
	}

#else
	for (int i = 0; i < graphUsedLength; i++) {
		int n = graphDataPionts[i];
		n -= min;
		n *= 31;
		n /= range;
		if (range == 0) n = 0;
		display.drawPixel(i * SCREEN_WIDTH / graphArrayLength, 31-n, SSD1306_INVERSE);
	}
#endif
}

void displayBTC(int nr)
{
	display.setTextColor(SSD1306_WHITE);	// White text
	display.setCursor(128-3*6, 32-8+1);     // Start at top-left corner
	display.printf("BTC");

	#if 0

	display.setTextSize(3);      			// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
//	display.setCursor(18, 4);				// Start at top-left corner

//	if (nr >= 1000) 
	{
		int x,w1,w2;
		w1 = 0;								// width 
		w2 = 6;
		w3= 3*18;							// width .999 part
		if      (nr >= 1000000)	w1 += 4*18;
		else if (nr >= 100000)	w1 += 3*18;
		else if (nr >= 10000)	w1 += 2*18;
		else if (nr >= 1000)	w1 += 1*18;

		x = (display.width() - w) / 2;
		display.setCursor(x, 4);
		display.printf("%d", nr / 1000);

		// Draw dot
		w = 
		display.drawFastHLine(x, 4+24-5-2, 3, SSD1306_WHITE);
		display.drawFastHLine(x, 4+24-4-2, 3, SSD1306_WHITE);
		display.drawFastHLine(x, 4+24-3-2, 3, SSD1306_WHITE);

		display.setCursor(x+6, 0);
		display.printf("%03d", nr % 1000);
	} else {
		display.setCursor(16+3*18+6, 0);
		display.printf("%3d", nr);
	}


//	void ssd1306_center_string(const char* buffer, uint8_t Y, uint8_t size)
	{
		int16_t   x, y;
		uint16_t  h, w;
	
		display.setTextSize(size);
		display.getTextBounds(buffer, 0, Y, &x, &y, &w, &h);
		display.setCursor((display.width() - w) / 2, y);
		display.print(buffer);
	}
	


	#else

	display.setTextSize(3);      			// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
//	display.setCursor(18, 4);				// Start at top-left corner

	if (nr >= 1000) {
		display.setCursor(8, 0);
		display.printf("%3d", nr / 1000);
		// Draw dot
		int x = display.getCursorX();
		display.drawFastHLine(x, 0+24-5-2, 3, SSD1306_WHITE);
		display.drawFastHLine(x, 0+24-4-2, 3, SSD1306_WHITE);
		display.drawFastHLine(x, 0+24-3-2, 3, SSD1306_WHITE);

		display.setCursor(x+6, 0);
		display.printf("%03d", nr % 1000);
	} else {
		display.setCursor(16+3*18+6, 0);
		display.printf("%3d", nr);
	}
#endif
}

void displayInit()
{
	static bool init = false;

	if (!init)
	{
		init = true;

		// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
		if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
			Serial.printf("SSD1306 display initialize failed....\n");
			while(1);
		}

//		display.clearDisplay();
//		display.invertDisplay(false);
		if (bootCount == 1)
		{
			display.setTextColor(SSD1306_WHITE);	// White text
			display.setTextSize(1);					// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
			display.clearDisplay();
			display.setCursor(0, 4);	display.printf("== Show BTC Price ==");
			display.setCursor(0, 16);	display.printf(" PE0FKO:" __DATE__);
			display.display();
			delay(2000);
			display.clearDisplay();
			display.setCursor(0, 4);	display.printf("URL: %s", https_host);
			display.display();
//			delay(2000);
		}
//		display.clearDisplay();
	}
}

void displayMessage(int ms, const char line[])
{
	Serial.printf("Message: %s\n", line);

	displayInit();

	Message_Timer = millis();
	Message_Value = ms;

//	display.invertDisplay(true);

	display.setTextColor(SSD1306_WHITE);	// White text
	display.clearDisplay();
	display.setTextSize(1);					// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(0, 8);
	display.print(line);
	display.display();

//	display.invertDisplay(false);
}
