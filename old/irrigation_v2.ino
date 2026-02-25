//#include <Arduino.h>
//#include <SPI.h>
#include "irrigation_v2.h"
#include <U8g2lib.h>
#include "soil_sensor.h"
#include "timer.h"
#include "timerRTC.h"
#include "air_sensor.h"
#include "button.h"

#define BT_LEFT                 0
#define BT_RIGHT                1
#define BT_OK                   2
#define BT_CANCEL               3

#define PIN_AIR_SENSOR          A2
#define PIN_BUTTON_UP           5
#define PIN_BUTTON_DOWN         6
#define PIN_BUTTON_OK           2
#define PIN_BUTTON_CANCEL       7

U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI oled(U8G2_MIRROR, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

SystemState systemState {0, 0, 0};

Timer timer(5000);
static TimerRTC rtc(555);
unsigned long seconds;
Ds1302::DateTime _dateTime;

Button buttons[4] = {
    Button(PIN_BUTTON_UP),
    Button(PIN_BUTTON_DOWN),
    Button(PIN_BUTTON_OK),
    Button(PIN_BUTTON_CANCEL)
};

SoilSensor soilSensor = SoilSensor(A0, A1, 2000);
AirSensor airSensor = AirSensor(PIN_AIR_SENSOR);
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
    

//    _dateTime = rtc.getDateTime();
 
    oled.firstPage();
    do {
        oled.setFont(u8g2_font_6x13_t_cyrillic);
        oled.setCursor(0,10);
        oled.print(systemState.soilHum);
        oled.setCursor(0,20);
        oled.print(timer.getTime());
        oled.setCursor(0,30);
        if (period) oled.print("PERIOD");
/*        oled.setCursor(0,40);
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
*/
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

    for (uint8_t _=0; _<4; _++) {
        buttons[_].begin();
    }

}

int temperature = 0;
int humidity = 0;
Timer tmr = Timer(500);

void loop(void) {
    systemState.soilHum = soilSensor.getSensorData();
    seconds = rtc.getTime();
    airSensor.getSensorData(&temperature, &humidity);
    for (uint8_t _=0; _<4; _++) {
        buttons[_].check();
    }

    if (tmr.isReady()) {
        Serial.print("Soil humidity, raw: ");
        Serial.print(soilSensor.getRawSensorData());
        Serial.print(", rel: ");
        Serial.println(systemState.soilHum);

        Serial.print("Air: t = ");
        Serial.print(temperature);
        Serial.print(", H = ");
        Serial.println(humidity);
        
        Serial.print("Buttons:");
        for (uint8_t _=0; _<4; _++) {
            Serial.print(" ");
            Serial.print(buttons[_].getState());
        }
        Serial.println();
        Serial.println();
//    showMainScreen();
//    Serial.println("completed");
    }
}
