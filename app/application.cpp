#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <Libraries/LiquidCrystal/LiquidCrystal_I2C.h>

#include <Phant.h>
#include <Print.h>
//#include <Services/ArduinoJson/include/ArduinoJson/JsonVariant.hpp>

#ifndef WIFI_SSID
    #define WIFI_SSID "SSID" // Put you SSID and Password here
    #define WIFI_PWD "123456Password"
#endif

#define LOCATION "Lisbon"
#define METRICS  "metric"   
#define OWAPIKEY "PUT_YOUR_OWM_KEY_HERE"

#define PHANTSERVER  "192.168.1.17:8080"
#define PHANTPUBKEY  "VmwrzZDPKpFyAddzPX4oIbqXwp9"
#define PHANTPRIVKEY "7QLYlybXMgCAaVVdLp9zsaWdRw6"

// For more information visit useful wiki page: http://arduino-info.wikispaces.com/LCD-Blue-I2C
#define I2C_LCD_ADDR 0x27
LiquidCrystal_I2C lcd(I2C_LCD_ADDR, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


Timer procTimer;
Timer checkTimer;
Timer owTimer;

Phant phant(PHANTSERVER, PHANTPUBKEY, PHANTPRIVKEY);

int sensorValue = 0;
int pressure = 900;
int temperature = 20;

HttpClient phantServer;
HttpClient provServer; // Provisioning server client
HttpClient owServer; // Openweather
String deviceid;

void onDataSent(HttpClient& client, bool successful) {
    if (successful)
        Serial.println("Success sent data to Phant");
    else
        Serial.println("Failed data to Phant");

    String response = client.getResponseString();
    Serial.println("Server response: '" + response + "'");
    
    if (response.length() > 0) {
        int intVal = response.toInt();

        if (intVal == 0)
            Serial.println("Sensor value wasn't accepted. May be we need to wait a little?");
    }
    
}

void onProvSent(HttpClient &client, bool successful) {
    StaticJsonBuffer<2048> jsonBuffer;

    if (successful)
        Serial.println("Registration request sent sucessufully");
    else
        Serial.println("Registration request has failed");

    String response = client.getResponseString();
    //Serial.println("Server response: '" + response + "'");

    JsonObject& config = jsonBuffer.parseObject((char *) response.c_str());
    
    if (!config.success()) {
        Serial.println("Error decoding JSON.");
    } else {

        //Serial.println("Received config: " + config.toJsonString());

        const char* name = config["name"];
        const char* datec = config["datec"];
        const char* data = config["data"];
        int len = strlen(data);

        Serial.println("Device Name: " + String(name));
        Serial.println("Date: " + String(datec));

        JsonArray& confdata = jsonBuffer.parseArray((char *) data);

        if (!confdata.success()) {
            Serial.println("Error decoding data json array");
        } else {
            Serial.println("Array OK");
        }

        confdata.printTo(Serial);
        Serial.println("");

        Serial.println("The array has " + String(confdata.size()));

        for (int i = 0; i < confdata.size(); i++) {
            JsonObject& obj = confdata[i];
            Serial.print("Object " + String(i) + ":");
            obj.printTo(Serial);
            Serial.println("");

            for (JsonObject::iterator it = obj.begin(); it != obj.end(); ++it) {
                Serial.println(it->key);
                Serial.println(it->value.asString());
            }

        }

        // Another way
        //            for (JsonArray::iterator it=confdata.begin(); it!=confdata.end(); ++it )
        //            {
        //                JsonObject obj = JsonObject(*it);
        //
        //                
        //            }

        Serial.println("Done...");
        lcd.setCursor(0,0);
        lcd.print("Done...     ");
    }
}

void registerDevice() {
    String devIP;
    StaticJsonBuffer<256> jsonBuffer;

    devIP = WifiStation.getIP().toString();

    Serial.println("Sending device ID " + deviceid + " to provisioning server...");
    Serial.println("Device IP : " + devIP);

    JsonObject& config = jsonBuffer.createObject();

    //config.addCopy("ipaddr", devIP );
    //config.addCopy("ssid", WIFI_SSID);
    config["ipaddr"] = devIP;
    config["ssid"] = WIFI_SSID;
    
    //Serial.println("Post body: " + config.toJsonString(false));
    provServer.setRequestContentType("application/json");
    //provServer.setPostBody(config.toJsonString(false));
    provServer.downloadString("http://192.168.1.17:3000/api/devices/" + deviceid, onProvSent);
}

void logHeap() {
    int heap;

    if (phantServer.isProcessing()) return; // We need to wait while request processing was completed

    // Read our sensor value :)
    heap = system_get_free_heap_size();

    phant.add("heap", heap);

    //Serial.println("http://192.168.1.17:8080/input/" + String(PHANTPUBKEY) + "?private_key=" + String(PHANTPRIVKEY) + "&heap=" + String(heap));

    phantServer.downloadString("http://192.168.1.17:8080/input/" + String(PHANTPUBKEY) + "?private_key=" + String(PHANTPRIVKEY) + "&heap=" + String(heap), onDataSent);

    //Serial.print("URL: ");
    //Serial.println(phant.url());
    
    //phantServer.downloadString(phant.url(), onDataSent);

}

double getValue(JsonObject& obj, String field) {
    double dval;
    int    ival;
    
    //Serial.println("inicio");
    dval = (double) obj[field].as<double>();
    //Serial.println("dval: " + String(dval));
    
    if ( dval == 0 ) {
        ival = (int) obj[field].as<int>();
        //Serial.println("ival: " + String(ival) );
        if ( ival != 0 ) return (double) ival;
    }
    
    return dval;
}

void weatherInfoCB(HttpClient &client, bool successful) {
    StaticJsonBuffer<2048> jsonBuffer;

    if (!successful)
        Serial.println("Weather Request has failed");
    else {

        Serial.println("Got weather info");
        String response = client.getResponseString();

        JsonObject &weatherResp = jsonBuffer.parseObject((char *) response.c_str());

        logHeap();
        
        if (!weatherResp.success()) {
            Serial.println("Error decoding JSON.");
        } else {
            Serial.println("Success decoding JSON");
            //weatherResp.prettyPrintTo(Serial);

            //Serial.println("--------------");
            JsonArray& list = weatherResp["list"].asArray();
            JsonObject& location = list[0];
            
            //list.prettyPrintTo(Serial);

            //Serial.println("--------------");
            //location.prettyPrintTo(Serial);
            //Serial.println("--------------");


            JsonObject& temps = location["main"];

            //temps.prettyPrintTo(Serial);

            double temp     = getValue( temps , "temp");
            double temp_min = getValue( temps , "temp_min");
            double temp_max = getValue( temps , "temp_max");
            double humidity = getValue( temps , "humidity");

//            Serial.println("......");
//            Serial.println("Temp Now: " + String(temp));
//            Serial.println("Temp Min: " + String(temp_min));
//            Serial.println("Temp Max: " + String(temp_max));
//            Serial.println("Humidity: " + String(humidity));

            double pressure = getValue( temps , "pressure");
//            Serial.println("Pressure: " + String(pressure));
            
            JsonObject &wind = location["wind"];
            
            double wspeed = getValue( wind , "speed");
            
//            Serial.println("Wind: " + String(wspeed)+ "m/s");
            
            JsonArray& weather = location["weather"];
            JsonObject& we = weather[0];
            
//            weather.prettyPrintTo(Serial);

            const char *desc = we["main"];
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Temp: " + String(temp) );
            
            lcd.setCursor(0,1);
            lcd.print(desc);
            Serial.println("Desc:" + String(desc));

            
            logHeap();
        }

    }

}

void getWeather() {
    String url = String("http://api.openweathermap.org/data/2.5/find?q=") + String(LOCATION) + String("&units=") + String(METRICS) + String("&APPID=") + String(OWAPIKEY);
    long heap;
    
    heap = system_get_free_heap_size();
    Serial.println("Heap: " + String(heap));
    if ( heap < 5000 ) system_restart();

    Serial.println("Weather req: " + url);
    owServer.downloadString(url, weatherInfoCB);
}

void ShowInfo() {
    Serial.printf("\r\nSDK: v%s\r\n", system_get_sdk_version());
    Serial.printf("Free Heap: %d\r\n", system_get_free_heap_size());
    Serial.printf("CPU Frequency: %d MHz\r\n", system_get_cpu_freq());
    Serial.printf("System Chip ID: 0x%x\r\n", system_get_chip_id());
    Serial.printf("SPI Flash ID: 0x%x\r\n", spi_flash_get_id());
    Serial.printf("SPI Flash Size: %d\r\n", (1 << ((spi_flash_get_id() >> 16) & 0xff)));
}

void startMain() {
    checkTimer.stop();
    // Register Device on the provisioning Server
    lcd.setCursor(0,0);
    lcd.print("Registering...");
    registerDevice();

    // Start send data loop
    //procTimer.initializeMs(25 * 1000, sendData).start(); // every 25 seconds
    owTimer.initializeMs(30 * 1000, (InterruptCallback)getWeather ).start();
    
}


// Will be called when WiFi station was connected to AP

void connectOk() {
    char data[200];
    char dt[20] = "";

    Serial.println("I'm CONNECTED");

    ShowInfo();

    // Let's get the Mac address, that should be unique...
    deviceid = WifiStation.getMAC();
    deviceid.toUpperCase();
    Serial.println("Device ID: " + deviceid);

    checkTimer.initializeMs(5 * 1000, startMain).start();
}

// Will be called when WiFi station timeout was reached

void connectFail() {
    Serial.println("I'm NOT CONNECTED. Need help :(");

    WifiStation.waitConnection(connectOk, 20, connectFail); // We recommend 20+ seconds for connection timeout at start
}

void networkScanCompleted(bool succeeded, BssList list) {

    if (!succeeded) {
        Serial.println("Failed to scan networks");
        return;
    }

    for (int i = 0; i < list.count(); i++) {
        Serial.print("\tWiFi: ");
        Serial.print(list[i].ssid);
        Serial.print(", ");
        Serial.print(list[i].getAuthorizationMethodName());
        if (list[i].hidden) Serial.print(" (hidden)");
        Serial.println("");
        Serial.println("Power: " + String(list[i].rssi));
        Serial.println();
    }
}

void sysReady() {

    Serial.println("System ready callback called....");
    
    lcd.begin(16, 2);   // initialize the lcd for 16 chars 2 lines, turn on backlight

        // ------- Quick 3 blinks of backlight  -------------
        for(int i = 0; i< 3; i++)
        {
                lcd.backlight();
                delay(150);
                lcd.noBacklight();
                delay(250);
        }
        lcd.backlight(); // finish with backlight on

}

void init() {

    system_set_os_print(0);

    Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
    Serial.systemDebugOutput(false); // Disable debug output to serial

    WifiStation.config(WIFI_SSID, WIFI_PWD);
    WifiStation.enable(true);
    WifiAccessPoint.enable(false);

    //WifiStation.startScan(networkScanCompleted);

    // Run our method when station was connected to AP (or not connected)
    WifiStation.waitConnection(connectOk, 20, connectFail); // We recommend 20+ seconds for connection timeout at start

    System.onReady(sysReady);
       
}
