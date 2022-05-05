#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>
const char* ssid = "";
const char* password = "";

// paste your api key
const String openWeatherMapApiKey = "";

// Replace with your country code and city
const String city = "New York";
const String countryCode = "US";

unsigned long lastTime = 0;
unsigned long timerDelay = 120000;

// Time Zone in Hours 
// negative numbers for a GMT- offset. positive for a GMT+ offset
// ex. -5 (GMT-5 aka Eastern Standard Time)
const int tzOffsetHours = -5;
// 0 if your country doesn't have dst, 1 if it does 
// 1 (US)
const int dstOffsetHours = 1;

bool PM = false;
bool smin = false;
bool shr = false;
int h = 0;

// uses automatic time server, set the correct server for your country for faster boot
const char* timeSrv = "pool.ntp.org";
// data buffer
String jsonBuffer;
// if this is set to true delay will be bypassed on the first request
bool firstRun = true;
LiquidCrystal_I2C lcd (0x27, 20,4);
// This contains a parsed version of the data buffer
JSONVar parsedBuffer;

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
// updates the clock
void UpdateTime() {
  
  int tempAsFh = 1.8 * double(parsedBuffer["main"]["temp"]) - 459.67;
  int wndspdAsMph = int(parsedBuffer["wind"]["speed"]) * 2.23693629;
  Serial.print("Temperature (F): ");
  Serial.println(tempAsFh);
  Serial.print("Pressure (hPA): ");
  Serial.println(parsedBuffer["main"]["pressure"]);
  Serial.print("Humidity: ");
  Serial.println(parsedBuffer["main"]["humidity"]);
  Serial.print("Wind Speed (mph): ");
  Serial.println(wndspdAsMph);
  Serial.print("Weather Report: ");
  Serial.println(parsedBuffer["weather"][0]["description"]);    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(city);
  lcd.print(" - ");
  lcd.print(parsedBuffer["weather"][0]["main"]);
  lcd.setCursor(0, 1);
  lcd.print("Temp ");
  lcd.print(tempAsFh);
  lcd.print(" F ");
  lcd.print(" ");
  lcd.print(parsedBuffer["main"]["humidity"]);
  lcd.print("% Humid");
  lcd.setCursor(0, 2);
  lcd.print("Wind ");
  lcd.print(wndspdAsMph);
  lcd.print("mph ");
  lcd.print(parsedBuffer["main"]["pressure"]);
  lcd.print("hPa");
  if(WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(19, 0);
    lcd.write(byte(1));
  } else {
    lcd.setCursor(19, 0);
    lcd.write(byte(0));
  }
  lcd.setCursor(0,3);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
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

void setup() {
  Serial.begin(115200);
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
  configTime(tzOffsetHours * 3600, dstOffsetHours * 3600, timeSrv);
  delay(2000);
}
void loop() {
  delay(1000);
  if ((millis() - lastTime) > timerDelay || firstRun ) {
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      if (firstRun) {
        firstRun = false;
      }
      Serial.println("Sending http request to API endpoint " + serverPath);
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.print("JSON Buffer ->");
      Serial.println(jsonBuffer);
      parsedBuffer = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(parsedBuffer) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
      /*
      int tempAsFh = 1.8 * double(parsedBuffer["main"]["temp"]) - 459.67;
      int wndspdAsMph = int(parsedBuffer["wind"]["speed"]) * 2.23693629;
      
      Serial.print("Temperature (F): ");
      Serial.println(tempAsFh);
      Serial.print("Pressure (hPA): ");
      Serial.println(parsedBuffer["main"]["pressure"]);
      Serial.print("Humidity: ");
      Serial.println(parsedBuffer["main"]["humidity"]);
      Serial.print("Wind Speed (mph): ");
      Serial.println(wndspdAsMph);
      Serial.print("Weather Report: ");
      Serial.println(parsedBuffer["weather"][0]["description"]);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(city);
      lcd.print(" - ");
      lcd.print(parsedBuffer["weather"][0]["main"]);
      lcd.setCursor(0, 1);
      lcd.print("Temp ");
      lcd.print(tempAsFh);
      lcd.print(" F ");
      lcd.print(" ");
      lcd.print(parsedBuffer["main"]["humidity"]);
      lcd.print("% Humid");
      lcd.setCursor(0, 2);
      lcd.print("Wind ");
      lcd.print(wndspdAsMph);
      lcd.print("mph ");
      lcd.print(parsedBuffer["main"]["pressure"]);
      lcd.print("hPa");
      */
    }
    else {
      Serial.println("WiFi Disconnected");
      WiFi.begin(ssid, password);
    }
    lastTime = millis();
  }
  UpdateTime();
}

String httpGETRequest(const char* serverName) {
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