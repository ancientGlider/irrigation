//#include <Arduino.h>
//#include <SPI.h>
#include "irrigation_v2.h"
#include <U8g2lib.h>
#include "soil_sensor.h"
#include "timer.h"
#include "timerRTC.h"
#include "air_sensor.h"
//#include <DHT11.h>

#define PIN_AIR_SENSOR          A2

U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI oled(U8G2_MIRROR, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

SystemState systemState {0, 0, 0};

Timer timer(5000);
static TimerRTC rtc(555);
unsigned long seconds;
Ds1302::DateTime _dateTime;

SoilSensor soilSensor(A0, A1);
AirSensor airSensor(PIN_AIR_SENSOR);
//DHT11 dht11(PIN_AIR_SENSOR);


void showMainScreen() {
    static Timer latency(2000);
    
    static bool period = false;
//    if (!timer.is_ready()) return;
    if (timer.isReady()) {
        period = true;
        latency.drop();
    }
    if (latency.isReady()) period = false;
    

    _dateTime = rtc.getDateTime();
 
    oled.firstPage();
    do {
        oled.setFont(u8g2_font_6x13_t_cyrillic);
        oled.setCursor(0,10);
        oled.print(systemState.soilHum);
        oled.setCursor(0,20);
        oled.print(timer.getTime());
        oled.setCursor(0,30);
        if (period) oled.print("PERIOD");
        oled.setCursor(0,40);
        oled.print(_dateTime.hour);
        oled.print(":");
        oled.print(_dateTime.minute);
        oled.print(":");
        oled.print(_dateTime.second);
        oled.setCursor(0,50);
        oled.print(_dateTime.day);
        oled.print("-");
        oled.print(_dateTime.month);
        oled.print("-");
        oled.print(_dateTime.year);
        oled.setCursor(0,60);
        oled.print(seconds);
    } while (oled.nextPage());
}

void setup(void) {
    Serial.begin(9600);

    Serial.print("Initial setup...");
    soilSensor.begin();
    oled.begin(2, 3, 4, 5, 6, 7);
    oled.enableUTF8Print();
    Serial.println("completed");

    Serial.print("RTC setup...");
    TimerRTC::begin(12, 4, 3);
    rtc.setTime(3155759998UL);
    Serial.println("completed");

    Serial.print("Air sensor setup...");
    airSensor.begin();
    Serial.println("completed");

}

int temperature = 0;
int humidity = 0;

void loop(void) {
    Serial.print("Getting soil humidity...");
    systemState.soilHum = soilSensor.getHumidity();
    Serial.println("completed");

    Serial.print("Getting system time...");
    seconds = rtc.getTime();
    Serial.println("completed");
    
    Serial.print("Getting air condition...");
    airSensor.getSensorData(temperature, humidity);
//    dht11.readTemperatureHumidity(temperature, humidity);
    Serial.print("completed, t = ");
    Serial.print(temperature);
    Serial.print(", H = ");
    Serial.println(humidity);
    
    Serial.print("Calling show_main_screen...");
    showMainScreen();
    Serial.println("completed");
    delay(1000);
}
