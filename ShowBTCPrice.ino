/**
   BasicHTTPSClient.ino

    Created on: 14.10.2018

*/

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
//#include <TZ.h>

#include <HTTPClient.h>

#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#if 1

const char* https_host = "https://api.bitvavo.com/v2/ticker/24h?market=BTC-EUR";

const char rootCACertificate [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
)CERT";

#elif 1

const char* https_host = "https://ca.ict.nl/4/";

const char rootCACertificate [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIGEzCCA/ugAwIBAgIQfVtRJrR2uhHbdBYLvFMNpzANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTgx
MTAyMDAwMDAwWhcNMzAxMjMxMjM1OTU5WjCBjzELMAkGA1UEBhMCR0IxGzAZBgNV
BAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UE
ChMPU2VjdGlnbyBMaW1pdGVkMTcwNQYDVQQDEy5TZWN0aWdvIFJTQSBEb21haW4g
VmFsaWRhdGlvbiBTZWN1cmUgU2VydmVyIENBMIIBIjANBgkqhkiG9w0BAQEFAAOC
AQ8AMIIBCgKCAQEA1nMz1tc8INAA0hdFuNY+B6I/x0HuMjDJsGz99J/LEpgPLT+N
TQEMgg8Xf2Iu6bhIefsWg06t1zIlk7cHv7lQP6lMw0Aq6Tn/2YHKHxYyQdqAJrkj
eocgHuP/IJo8lURvh3UGkEC0MpMWCRAIIz7S3YcPb11RFGoKacVPAXJpz9OTTG0E
oKMbgn6xmrntxZ7FN3ifmgg0+1YuWMQJDgZkW7w33PGfKGioVrCSo1yfu4iYCBsk
Haswha6vsC6eep3BwEIc4gLw6uBK0u+QDrTBQBbwb4VCSmT3pDCg/r8uoydajotY
uK3DGReEY+1vVv2Dy2A0xHS+5p3b4eTlygxfFQIDAQABo4IBbjCCAWowHwYDVR0j
BBgwFoAUU3m/WqorSs9UgOHYm8Cd8rIDZsswHQYDVR0OBBYEFI2MXsRUrYrhd+mb
+ZsF4bgBjWHhMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMBAf8ECDAGAQH/AgEAMB0G
A1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAbBgNVHSAEFDASMAYGBFUdIAAw
CAYGZ4EMAQIBMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRydXN0
LmNvbS9VU0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB2Bggr
BgEFBQcBAQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNv
bS9VU0VSVHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZaHR0cDov
L29jc3AudXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEAMr9hvQ5Iw0/H
ukdN+Jx4GQHcEx2Ab/zDcLRSmjEzmldS+zGea6TvVKqJjUAXaPgREHzSyrHxVYbH
7rM2kYb2OVG/Rr8PoLq0935JxCo2F57kaDl6r5ROVm+yezu/Coa9zcV3HAO4OLGi
H19+24rcRki2aArPsrW04jTkZ6k4Zgle0rj8nSg6F0AnwnJOKf0hPHzPE/uWLMUx
RP0T7dWbqWlod3zu4f+k+TY4CFM5ooQ0nBnzvg6s1SQ36yOoeNDT5++SR2RiOSLv
xvcRviKFxmZEJCaOEDKNyJOuB56DPi/Z+fVGjmO+wea03KbNIaiGCpXZLoUmGv38
sbZXQm2V0TP2ORQGgkE49Y9Y3IBbpNV9lXj9p5v//cWoaasm56ekBYdbqbe4oyAL
l6lFhd2zi+WJN44pDfwGF/Y4QA5C5BIG+3vzxhFoYt/jmPQT2BVPi7Fp2RBgvGQq
6jG35LWjOhSbJuMLe/0CjraZwTiXWTb2qHSihrZe68Zk6s+go/lunrotEbaGmAhY
LcmsJWTyXnW0OMGuf1pGg+pRyrbxmRE1a6Vqe8YAsOf4vmSyrcjC8azjUeqkk+B5
yOGBQMkKW+ESPMFgKuOXwIlCypTPRpgSabuY0MLTDXJLR27lk8QyKGOHQ+SwMj4K
00u/I5sUKUErmgQfky3xxzlIPK1aEn8=
-----END CERTIFICATE-----
)CERT";

#endif

const   uint32_t  Access_Value     = 20 * 1000;
static  uint32_t  Access_Timer     = 0;

// ./.arduino15/packages/esp8266/hardware/esp8266/3.1.2/cores/esp8266/TZ.h
//#define TZ_Europe_Amsterdam     PSTR("CET-1CEST,M3.5.0,M10.5.0/3")

// Not sure if WiFiClientSecure checks the validity date of the certificate. 
// Setting clock just to be sure...
void setClock() {
//  configTime(0, 0, "pool.ntp.org");
  configTime(0, 0, "time.google.com");
//	configTime(TZ_Europe_Amsterdam, "time.google.com","nl.pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}


WiFiMulti WiFiMulti;

void setup() 
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  while(!Serial) ;

  Serial.println();
  Serial.println("Starting...........");
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("pe0fko_ziggo", "NetwerkBeheer114");

  // wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    Serial.print(".");
  }
  Serial.println(" connected");

//  setClock();  

	time_t now = time(nullptr);			// get UNIX timestamp
	Serial.printf("Time: %s\n", ctime(&now));					// convert timestamp and display

  Serial.print("Access host: ");
  Serial.println(https_host);

  Access_Timer = millis() - Access_Value;
}

void loop() 
{
  if (millis() - Access_Timer > Access_Value)
  {
    Access_Timer += Access_Value;

    WiFiClientSecure *client = new WiFiClientSecure;
    if(client) 
    {
      client -> setCACert(rootCACertificate);

      {
        // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
        HTTPClient https;
    
//        Serial.print("[HTTPS] begin...\n");

        if (https.begin(*client, https_host))  // HTTPS
        {
//          Serial.print("[HTTPS] GET...\n");
          // start connection and send HTTP header
          int httpCode = https.GET();
    
          // httpCode will be negative on error
          if (httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
//            Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
    
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              String payload = https.getString();
 //             Serial.println(payload);

              JsonDecode(payload.c_str());
            }
          } else {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
          }
    
          https.end();
        } else {
          Serial.printf("[HTTPS] Unable to connect\n");
        }

        // End extra scoping block
      }
    
      delete client;
    } else {
      Serial.println("Unable to create client");
    }
  }

#if 1
#elif 0
  Serial.println();
  Serial.println("Waiting 10s before the next round...");
  delay(30 * 1000);
#else
  while(1)
  {
    Serial.println();
//    Serial.println("Waiting 10s before the next round...");
    delay(10000);
    yield();
  }
#endif
}

void JsonDecode(const char* json)
{
  // Allocate the JSON document
  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json);

  // Test if parsing succeeds
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
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

/*
  const char* sensor = doc["last"];
  Serial.println(sensor);

  // Fetch the values
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  const char* sensor = doc["sensor"];
  long time = doc["time"];
  double latitude = doc["data"][0];
  double longitude = doc["data"][1];

  // Print the values
  Serial.println(sensor);
  Serial.println(time);
  Serial.println(latitude, 6);
  Serial.println(longitude, 6);
*/
}