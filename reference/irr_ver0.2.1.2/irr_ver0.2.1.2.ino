#include <OLED_I2C.h>

// Объявление пинов для устройств

// Пины на сдвиговых регистрах
#define Q0 0x100
#define Q1 0x101
#define Q2 0x102
#define Q3 0x103
#define Q4 0x104
#define Q5 0x105
#define Q6 0x106
#define Q7 0x107

// Определение пинов
// Аналоговые ножки 4 и 5 используются для I2C интерфейса, по которому подключен OLED-дисплей
#define PIN_CLOCK_RXTXCTL   A0
#define PIN_CLOCK_DATA      A1
#define PIN_SOIL_RH_DATA    A2
#define PIN_OLED_SDA        A3
#define PIN_OLED_SCL        A4
#define PIN_CLOCK_SYNC      A5
#define PIN_SHIFT_SYNC_RX   A5
#define PIN_SHIFT_SYNC_TX   A5
#define PIN_PUMP            2 //Q? - управление помпой
#define PIN_SOIL_RH         3 //Q? - аналоговый емкостной датчик влажности
#define PIN_SHIFT_RX        5
#define PIN_SHIFT_TX        6
#define PIN_SHIFT_LATCH_RX  11
#define PIN_SHIFT_LATCH_TX  12


// Коды статусов и ошибок:
#define ER_OK               0x00          // успех
#define ER_DRYSOIL          0x01          // сухая земля, подготовка к поливу
#define ER_IRRIGATION       0x02          // производится полив
#define ER_OUTOFWATER       0x03          // кончилась вода, почва не увлажнилась после полива
#define ER_UNREACH          0x10          // дисплей недоступен

#define ANALOG_HUM_ZERO     500         // показатели аналогового датчика при сухой почве (0%)
#define ANALOG_HUM_FULL     180         // показатели аналогового датчика при залитой почве (100%)

#define PULSE_WIDTH_USEC    5           // задержка при считывании данных от сдвиговых регистров (микросекунды)
#define STAB_PAUSE          60          // время стабилизации датчика после включения питания (миллисекунды)

#define IRRIGATE_TIME       18000        // время одного полива (миллисекунды)
#define IRRIGATE_COUNT      6            // количество поливов на одну ирригацию
#define IRRIGATE_PERIOD     600         // пауза между поливами и опросами датчика (секунды)
#define MIN_HUMIDITY        58          // минимальная нормальная влажность почвы в процентах


// макросы
#define FOR(cnt) for (int i = 0; i < (cnt); i++)

uint8_t ERRORCODE;

// состояние вводов и выводов, обслуживаемых сдвиговыми регистрами
uint8_t shiftInput, shiftOutput;

extern uint8_t RusFont[];
extern uint8_t MediumNumbers[];
extern uint8_t BigNumbers[];


// данные датчиков
int analogSoilRH;

// переменные для контроля за временем
uint32_t timeNow = 0;                            // время с момента включения в секундах

// управление поливом
uint32_t lastIrrigation = 0 - IRRIGATE_PERIOD;    // время последнего полива
uint32_t lastScreenUpdate = -1;
uint8_t irrigationCount;                         // количество произведённых поливов за сессию
int beforeIrrigationRH;                          // влажность до полива

OLED display(PIN_OLED_SDA, PIN_OLED_SCL); // создаем класс display

void setup() {
  Serial.begin(9600);

  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_SOIL_RH, OUTPUT);
  pinMode(PIN_SOIL_RH_DATA, INPUT);
  
  digitalWrite(PIN_PUMP,LOW);
  digitalWrite(PIN_SOIL_RH,LOW);

  ERRORCODE = display.begin() ? ER_OK : ER_UNREACH;
  logEvent("Initializing display");
  
  if (ERRORCODE < 0x10) display.setFont(RusFont);

  logEvent("Initializing timer");
  initTimer();
  Serial.println(F("OK"));
  

}

void loop() {
  if (ERRORCODE >= 0x10) return;
  if (timeNow - lastIrrigation >= IRRIGATE_PERIOD) {
    lastIrrigation = timeNow;
    readSoilSensors();
    irrigate();
  }
  if (lastScreenUpdate != timeNow) showDisplay();

}

void initTimer() {
    cli();  // отключить глобальные прерывания
    TCCR1A = 0;   // установить регистры в 0
    TCCR1B = 0;

    OCR1A = 15624; // установка регистра совпадения, 1 сек = 15625 тактов, минус один такт на сброс в 0

    TCCR1B |= (1 << WGM12);  // включить CTC режим 
    TCCR1B |= (1 << CS10); // Установить биты на коэффициент деления 1024
    TCCR1B |= (1 << CS12);

    TIMSK1 |= (1 << OCIE1A);  // включить прерывание по совпадению таймера 
    sei(); // включить глобальные прерывания
}

ISR(TIMER1_COMPA_vect) {  // прерывание по таймеру, 1 раз в 1 сек
  timeNow++;
}

void irrigate() {
  switch (ERRORCODE) {
  
    case ER_OK:
    case ER_OUTOFWATER:
      if (analogSoilRH < MIN_HUMIDITY) ERRORCODE = ER_DRYSOIL;
      break;
    case ER_DRYSOIL:
        if (analogSoilRH < MIN_HUMIDITY) {
          irrigationCount = 0;
          beforeIrrigationRH = analogSoilRH;
          ERRORCODE = ER_IRRIGATION;
        } else {
          ERRORCODE = ER_OK;
        }
      break;
    case ER_IRRIGATION:
        irrigationCount++;
        if (irrigationCount > IRRIGATE_COUNT) {
          readSoilSensors();
          if (analogSoilRH <= beforeIrrigationRH) {
            ERRORCODE = ER_OUTOFWATER;
            logEvent("out of water");
          } else ERRORCODE = ER_OK;
          return;
        }
        logEvent("irrigation start");
        digitalWrite(PIN_PUMP, HIGH);
        delay(IRRIGATE_TIME);
        digitalWrite(PIN_PUMP, LOW);
        logEvent("irrigation stop");
        break;
  }
}

void readSoilSensors() {
  digitalWrite(PIN_SOIL_RH, HIGH);                    // включаем питание датчика
  delay(STAB_PAUSE);                                  // ждём стабилизации показаний
  int analogSoilData = analogRead(PIN_SOIL_RH_DATA);  // считываем показания
  digitalWrite(PIN_SOIL_RH, LOW);                     // выключаем питание датчика
//  if (analogSoilData < ANALOG_HUM_FULL) analogSoilData = ANALOG_HUM_FULL;
//  if (analogSoilData > ANALOG_HUM_ZERO) analogSoilData = ANALOG_HUM_ZERO;
  analogSoilRH = map(analogSoilData, ANALOG_HUM_ZERO, ANALOG_HUM_FULL, 0, 100);   // переводим показания датчика в проценты влажности
//  uint32_t(100) * (ANALOG_HUM_ZERO - analogSoilData) / (ANALOG_HUM_ZERO - ANALOG_HUM_FULL);
}


void logEvent(String event) {
  Serial.print(timeNow);
  Serial.print("s : ");
  Serial.print(event);
  Serial.print("...");
  Serial.print("ERRORCODE: ");
  Serial.println(ERRORCODE);  
}

void showDisplay() {
  lastScreenUpdate = timeNow;
  
  uint16_t secs = lastIrrigation + IRRIGATE_PERIOD - timeNow;
  uint8_t mins = secs / 60;
  secs %= 60;
  
  display.clrScr();

/*display.printNumI(timeNow, 0, 0);
display.printNumI(lastIrrigation, 0, 6);*/
  
  display.print("\xcf\xf0\xee\xe2\xe5\xf0\xea\xe0 \xf7\xe5\xf0\xe5\xe7:   :", 0, 0);
  display.printNumI(mins, mins < 10 ? 17*6 : 16*6, 0);
  if (secs < 10) {
    display.printNumI(secs, 20*6, 0);
    display.printNumI(0, 19*6, 0);
  } else display.printNumI(secs, 19*6, 0);
  display.print("\xc2\xeb\xe0\xe6\xed\xee\xf1\xf2\xfc", 6, 10);


  display.setFont(BigNumbers);
  display.printNumI(analogSoilRH, 50, 26);
  display.setFont(RusFont);

  switch (ERRORCODE) {
    case ER_OK:         display.print("\302\373\360\340\371\350\342\340\355\350\345...", 0, 56); break;
    case ER_DRYSOIL:    display.print("\321\363\365\340\377 \347\345\354\353\377 -> \357\356\353\350\342", 0, 56); break;
    case ER_IRRIGATION: display.print("\317\356\353\350\342...   /", 0, 56); 
                        display.printNumI(irrigationCount, irrigationCount < 10 ? 60 : 54, 56);
                        display.printNumI(IRRIGATE_COUNT, 72, 56);
                        break;
    case ER_OUTOFWATER: display.print("!!! \315\340\353\345\351\362\345 \342\356\344\373 !!!", 0, 56); break;
  }
  
  display.update();
  
}
