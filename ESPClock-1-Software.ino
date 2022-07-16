#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include "time.h"
const char* ssid = "";
const char* password = "";

// paste your api key
const String openWeatherMapApiKey = "";

// Replace with your country code and city
const String city = "New York";
const String countryCode = "US";


unsigned long lastTime = 0;
unsigned long timerDelay = 120000;
unsigned long screenChangeLoopCount = 0;
int CurrentScreen = 0;
// Time Zone in Hours 
// negative numbers for a GMT- offset. positive for a GMT+ offset
// ex. -5 (GMT-5 aka Eastern Standard Time)
const int tzOffsetHours = -5;
// 0 if it is not dst, 1 if it is
const bool dst = true;

bool PM = false;
bool smin = false;
bool shr = false;
int h = 0;

int datasetsAvailable = 39;
// uses automatic time server, set the correct server for your country for faster boot
const char* timeSrv = "pool.ntp.org";
// if this is set to true delay will be bypassed on the first request
bool firstRun = true;
LiquidCrystal_I2C lcd (0x27, 20,4);
// This contains a parsed version of the current weather data buffer
DynamicJsonDocument currentDataDocument(24576);
// This contains a parsed version of the current forecast data buffer
DynamicJsonDocument forecastsDocument(24576);
 
int forecastedTemps[4];

byte wifiActive[] = {
  B00000,
  B00000,
  B01110,
  B10001,
  B00100,
  B01010,
  B00000,
  B00100
};

byte wifiLost[] = {
  B00000,
  B01110,
  B11001,
  B10101,
  B10011,
  B01110,
  B00000,
  B00000
};
int ForecastDaysFound = 0;
int ForecastDaysAvailable = 4;
struct tm timeinfo;
//main weather api URL (gets current weather data data)
const String CurrentWeatherAPI_URL = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
const String ForecastAPI_URL = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;


// display update function
void UpdateDisplay() {
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  if (CurrentScreen == 0) {
    const int tempAsFh = 1.8 * int(currentDataDocument["main"]["temp"]) - 459.67;
    const int wndspdAsMph = int(currentDataDocument["wind"]["speed"]) * 2.23693629;
    const int humidity = currentDataDocument["main"]["humidity"];
    const char* description = currentDataDocument["weather"][0]["description"];
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(description);
    lcd.setCursor(0, 1);
    lcd.print("Temp ");
    lcd.print(tempAsFh);
    lcd.print(" F ");
    lcd.print(humidity);
    lcd.print("% Humid");
    lcd.setCursor(0, 2);
    lcd.print("Wind ");
    lcd.print(wndspdAsMph);
    lcd.print("mph ");
    if(WiFi.status() != WL_CONNECTED) {
      lcd.setCursor(19, 3);
      lcd.write(byte(1));
    } else {
      lcd.setCursor(19, 3);
      lcd.write(byte(0));
    }
    lcd.setCursor(0,3);
    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    if (timeinfo.tm_hour > 12) {
      h = timeinfo.tm_hour - 12;
      PM = true;
    } else {
      h = timeinfo.tm_hour;
      PM = false;
    }
    if (timeinfo.tm_min < 10) {
      smin = true;
    } else {
      smin = false;
    }
    if (timeinfo.tm_hour < 10) {
      shr = true;
    } else {
      shr = false;
    }

    lcd.print(timeinfo.tm_mon + 1);
    lcd.print("/");
    lcd.print(timeinfo.tm_mday);
    lcd.print("/");
    lcd.print(timeinfo.tm_year + 1900);
    lcd.print(" ");
      
    if (shr) { lcd.print(0); }
    lcd.print(h);
    lcd.print(":");
    if (smin) { lcd.print(0); }
    lcd.print(timeinfo.tm_min);
    if (PM) {
      lcd.print(" PM");
    } else {
      lcd.print(" AM");
    }
  }
  if (CurrentScreen == 1) {
    int month = timeinfo.tm_mon + 1;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Forecasts (");
    lcd.print(ForecastDaysAvailable + 1);
    lcd.print("day(s)");
    if(WiFi.status() != WL_CONNECTED) {
      lcd.setCursor(19, 3);
      lcd.write(byte(1));
    } else {
      lcd.setCursor(19, 3);
      lcd.write(byte(0));
    }
    lcd.setCursor(0,1);
    const int day1 = timeinfo.tm_mday + 1;
    if (day1 > 31) {
      month++;
    }
    lcd.print(month);
    lcd.print("/");
    lcd.print(day1);
    lcd.print(": ");
    lcd.print(forecastedTemps[0]);
    lcd.print("F ");
    const int day2 = timeinfo.tm_mday + 2;
    lcd.print(month);
    lcd.print("/");
    lcd.print(day2);
    lcd.print(": ");
    lcd.print(forecastedTemps[1]);
    lcd.print("F");
    lcd.setCursor(0,2);
    const int day3 = timeinfo.tm_mday + 3;
    lcd.print(month);
    lcd.print("/");
    lcd.print(day3);
    lcd.print(": ");
    lcd.print(forecastedTemps[2]);
    lcd.print("F ");
    const int day4 = timeinfo.tm_mday + 4;
    lcd.print(month);
    lcd.print("/");
    lcd.print(day4);
    lcd.print(": ");
    lcd.print(forecastedTemps[3]);
    lcd.print("F");
    lcd.setCursor(0,3);
    const int day5 = timeinfo.tm_mday + 5;
    lcd.print(month);
    lcd.print("/");
    lcd.print(day5);
    lcd.print(": ");
    lcd.print(forecastedTemps[4]);
    lcd.print("F ");
    if (shr) { lcd.print(0); }
    lcd.print(h);
    lcd.print(":");
    if (smin) { lcd.print(0); }
    lcd.print(timeinfo.tm_min);
    if (PM) {
      lcd.print(" PM");
    } else {
      lcd.print(" AM");
    }
  }
}
void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.createChar(0, wifiActive);
  lcd.createChar(1, wifiLost);
  lcd.setCursor(0,0);
  lcd.print("Connecting to WiFi ");
  lcd.write(byte(1));
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connected to WiFi  ");
  lcd.write(byte(0));
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  if (dst) {
    configTime(tzOffsetHours * 3600 + 3600, 0, timeSrv);
  } else {
    configTime(tzOffsetHours * 3600, 0, timeSrv);
  }
  delay(2000);
}
void loop() {
  delay(1000);
  screenChangeLoopCount++;
  if ((millis() - lastTime) > timerDelay || firstRun ) {
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      Serial.println("Sending http request to API endpoint " + CurrentWeatherAPI_URL);
      const String db1 = httpGETRequest(CurrentWeatherAPI_URL);
      const String db2 = httpGETRequest(ForecastAPI_URL);
      DeserializationError err1 = deserializeJson(currentDataDocument, db1);
      if (err1) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(err1.f_str());
      }       
      DeserializationError err2 = deserializeJson(forecastsDocument, db2);
      if (err2) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(err2.f_str());
      }
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
      }
      if (firstRun) {
        firstRun = false;
      }
      int DT = 0;
      int ftime_day;
      const int ftime_year = timeinfo.tm_year;
      const int ftime_month = timeinfo.tm_mon;
      const int ftime_hour = 11;
      const int ftime_minute = 00;
      const int ftime_sec = 00;
      for (int i = 0; i <= datasetsAvailable; i++) {
        Serial.println(i);
        JsonObject dataset = forecastsDocument["list"][i];
        if (DT == 0) {
          ftime_day = timeinfo.tm_mday + 1; 
          Serial.println(ftime_day);
        }
        struct tm ftime = {0};
        const long datasetDT = dataset["dt"];
        ftime.tm_year = ftime_year;
        ftime.tm_mon = ftime_month;
        ftime.tm_mday = ftime_day;
        ftime.tm_hour = ftime_hour;
        ftime.tm_min = ftime_minute;
        ftime.tm_sec = ftime_sec;
        time_t timeSinceEpoch = mktime(&ftime);
        if (dst) {
          DT = int(timeSinceEpoch) - (tzOffsetHours * 3600) + 3600;
        } else {
          DT = int(timeSinceEpoch) - (tzOffsetHours * 3600);
        }
        Serial.print("DT = ");
        Serial.print(datasetDT);
        Serial.print(" CalculatedDT = ");
        Serial.println(DT);
        if (DT == datasetDT) {
          Serial.println("MATCH!");
          const int tempAsFh = 1.8 * int(dataset["main"]["temp"]) - 459.67;
          if (ForecastDaysFound == 0) {
            forecastedTemps[0] = tempAsFh;
            ForecastDaysFound++;
          } else {
            forecastedTemps[ForecastDaysFound] = tempAsFh;
            ForecastDaysFound++;
          }
          ftime_day++;
        }
      }
    }
    else {
      Serial.println("WiFi Disconnected");
      WiFi.begin(ssid, password);
    }
    lastTime = millis();
  }
  if (screenChangeLoopCount >= 8 && CurrentScreen == 0) {
    CurrentScreen++;
    screenChangeLoopCount = 0;
  }
  if (screenChangeLoopCount >= 8 && CurrentScreen == 1) {
    CurrentScreen--;
    screenChangeLoopCount = 0;
  }
  UpdateDisplay();

  Serial.print("Counter: ");
  Serial.print(screenChangeLoopCount);
  Serial.print(", Current Screen: ");
  Serial.print(CurrentScreen);
  Serial.print("\n");
}

String httpGETRequest(String serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}