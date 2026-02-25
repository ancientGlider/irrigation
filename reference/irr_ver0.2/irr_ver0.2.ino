#include <OLED_I2C.h>
#include <AltSoftSerial.h>  // библиотека для работы с последовательным интерфейсом, здесь RS-485

#define NPKADDR 0x02        // адрес сенсора NPK, pH, EC, RH, temp
#define SENSORMAXDATA 11    // максимальное количество байт в ответе датчика

#define PIN_OLED_SDA        A3
#define PIN_OLED_SCL        A4
#define PIN_485_RXTXCTL     7
#define PIN_485_RX          8
#define PIN_485_TX          9
#define PIN_NPK             2

// Коды ошибок:
#define ER_OK 0             // успех
#define ER_UNREACH 1        // датчик недоступен
#define ER_MANYDATA 2       // слишком большое количество информации от датчика (максимум определен в SENSORMAXDATA)
#define ER_CRC 3            // неверная котрольная сумма
#define ER_TIMEOUT 4        // превышено время ожидания ответа от датчика
#define ERRORCODE 30000     // значение параметра, свидетельствующее об ошибке

extern uint8_t RusFont[];

// структуры для данных, считываемых с датчиков
struct dataTempRH {
  uint16_t RH;
  int temp;
  uint8_t count;
};

struct dataNPK {
  uint16_t N, P, K;
  uint8_t count;
};

struct dataEC {
  uint16_t EC;
  uint8_t count;
};

struct dataPH {
  uint16_t pH;
  uint8_t count;
};

struct Soil {
  uint16_t pH, EC, N, P, K, RH;
  int temp;
} soil;

byte values[SENSORMAXDATA];         // буфер для хранения данных с датчиков
AltSoftSerial mod;                  // сущность для общения с датчиками по протоколу Modbus через последовательный интерфейс по шине RS-485
OLED display(PIN_OLED_SDA, PIN_OLED_SCL); // создаем класс display

void setup() {
  Serial.begin(9600);
  mod.begin(9600);

  pinMode(PIN_485_RXTXCTL, OUTPUT);
  pinMode(PIN_NPK, OUTPUT);
  
  digitalWrite(PIN_485_RXTXCTL, LOW);
  digitalWrite(PIN_NPK,LOW);

  logEvent(display.begin() ? ER_OK : ER_UNREACH, "Initializing display");
  display.setFont(RusFont);
}

void loop() {
  readSoilSensors();
  showDisplay();
  delay(600000);
}

void powerOn(uint8_t pin) {
  digitalWrite(pin, HIGH);
}

void powerOff(uint8_t pin) {
  digitalWrite(pin, LOW);
}

byte readSoilSensors() {
  dataTempRH tempRH = {0, 0, 0};
  dataNPK NPK = {0, 0, 0, 0};
  dataEC EC = {0, 0};
  dataPH pH = {0, 0};
  byte res;
  powerOn(PIN_NPK);        // включаем питание датчика NPK
  for (byte i = 0; i < 3; i++) {    // снимаем показания 3 раза через 2 сек для повышения точности
    delay(15000);          // пауза 15 сек для стабилизации показаний
    res = GetData(NPKADDR, 0x06, 0x01);    // считываем pH
    logEvent(res, "Getting pH from NPK soil sensor");
    if (res == 0) {
      pH.count++;
      pH.pH += (values[3] << 8) | values[4];
#ifdef DEBUG_1
      message = "Got pH: value=";
      message += String(pH.pH);
      message += ", count=";
      message += String(pH.count);
      message += ", calculated value=";
      message += String(pH.pH / pH.count);
      logEvent(ER_OK, message);
#endif
    }
    res = GetData(NPKADDR, 0x12, 0x02);    // считываем влажность и температуру
    logEvent(res, "Getting RH and temp from NPK soil sensor");
    if (res == 0) {
      tempRH.count++;
      tempRH.RH += (values[3] << 8) | values[4];
      tempRH.temp += (values[5] << 8) | values[6];
#ifdef DEBUG_1
      message = "Got RH: value=";
      message += String(tempRH.RH);
      message += ", count=";
      message += String(tempRH.count);
      message += ", calculated value=";
      message += String(tempRH.RH / tempRH.count);
      logEvent(ER_OK, message);
      message = "Got temp: value=";
      message += String(tempRH.temp);
      message += ", count=";
      message += String(tempRH.count);
      message += ", calculated value=";
      message += String(tempRH.temp / tempRH.count);
      logEvent(ER_OK, message);
#endif
    }
    res = GetData(NPKADDR, 0x15, 0x01);    // считываем EC
    logEvent(res, "Getting EC from NPK soil sensor");
    if (res == 0) {
      EC.count++;
      EC.EC += (values[3] << 8) | values[4];
#ifdef DEBUG_1
      message = "Got EC: value=";
      message += String(EC.EC);
      message += ", count=";
      message += String(EC.count);
      message += ", calculated value=";
      message += String(EC.EC / EC.count);
      logEvent(ER_OK, message);
#endif
    }
    res = GetData(NPKADDR, 0x1e, 0x03);    // считываем NPK
    logEvent(res, "Getting NPK from NPK soil sensor");
    if (res == 0) {
      NPK.count++;
      NPK.N += (values[3] << 8) | values[4];
      NPK.P += (values[5] << 8) | values[6];
      NPK.K += (values[7] << 8) | values[8];
    }
  }
  powerOff(PIN_NPK);     // выключаем питание датчика NPK

  soil.pH = pH.count ? pH.pH / pH.count : ERRORCODE;    // проверяем на ошибки и записываем средние значения в переменную soil
  soil.EC = EC.count ? EC.EC / EC.count : ERRORCODE;
  soil.N = NPK.count ? NPK.N / NPK.count : ERRORCODE;
  soil.P = NPK.count ? NPK.P / NPK.count : ERRORCODE;
  soil.K = NPK.count ? NPK.K / NPK.count : ERRORCODE;
  soil.RH = tempRH.count ? tempRH.RH / tempRH.count : ERRORCODE;
  soil.temp = tempRH.count ? tempRH.temp / tempRH.count : ERRORCODE;
  if (soil.RH == 1000) soil.RH--;
}


byte GetData(uint8_t addr, uint16_t regNo, uint16_t regCount) {
  
/* функция GetData считывает данные с датчиков.
 *  на входе: 
 *            addr - адрес датчика в шине RS-485 (протокол modbus)
 *            regNo - адрес регистра в датчике для чтения
 *            regCount - количество регистров для чтения
 *            *data - буфер для считывания
 *  функция возвращает код ошибки
 */

  uint8_t message[8];                 // набор данных для отправки в датчик

// формирование пакета данных для отправки в датчик по протоколу Modbus
  message[0] = addr;                  // адрес контроллера
  message[1] = 0x03;                  // команда чтения регистра
  message[2] = byte(regNo >> 8);      // старший байт номера регистра
  message[3] = byte(regNo & 0xFF);    // младший байт номера регистра
  message[4] = byte(regCount >> 8);   // старший байт количества регистров
  message[5] = byte(regCount & 0xFF); // младший байт количества регистров

  uint16_t crc = GetCrc16(message, 6);// вычисление контрольной суммы

  message[6] = byte(crc & 0xFF);    // младший байт контрольной суммы
  message[7] = byte(crc >> 8);      // старший байт контрольной суммы

// процедура отправки пакета данных

#ifdef DEBUG  
  Serial.print("Sending: ");
  for (byte i = 0; i < 8; i++) {
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println("...");
  Serial.println("Receiving: ");
#endif

  mod.flushInput();
  digitalWrite(PIN_485_RXTXCTL, HIGH);
  delay(1);
  for (byte i = 0; i < 8; i++ ) mod.write(message[i]);
  mod.flush();

// процедура получения ответа
  digitalWrite(PIN_485_RXTXCTL, LOW);
  delay(200);

  // считываем первые 3 байта для проверки корректности ответа и получения сведений о размере ответа
  for (byte i = 0; i < 3; i++) {
    values[i] = mod.read();
#ifdef DEBUG
  Serial.print(values[i], HEX);
  Serial.print(" ");
#endif
  }
#ifdef DEBUG
  Serial.println();
#endif

  // в ответе первые 2 байта должны быть такие же, как и в запросе
  if (values[0] != message[0] || values[1] != message[1]) return ER_UNREACH;

  // количество байт в ответе не должно превышать SENSORMAXDATA, 5 байт уже используются для адреса, номера функции, кол-ва байт в ответе и CRC
  if (values[2] > SENSORMAXDATA-5) return ER_MANYDATA;
  
  for (byte i = 3; i < values[2]+5; i++) {
    values[i] = mod.read();
  }

  crc = GetCrc16(values, values[2]+3);
  if (values[values[2]+3] != byte(crc & 0xFF) || values[values[2]+4] != byte(crc >> 8)) return ER_CRC;

  return ER_OK;
}

// функция вычисления CRC
uint16_t GetCrc16(uint8_t * data, uint16_t len)  {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t) data[pos];
        for (uint16_t i = 8; i != 0; i--) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            }
         else
            crc >>= 1;
        }
    }
    return crc;
}

void logEvent(byte errorCode, String event) {
  Serial.print(event);
  Serial.print("...");
  if (errorCode == ER_OK) Serial.println("OK");
  else {
    Serial.print("ERROR: ");
    Serial.println(errorCode);  
  }
}

void showDisplaySensors() {
  display.clrScr();
  display.print("Sensors", 0, 0);
  display.drawLine(0, 9, 128, 9);
  display.print("Soil:  pH= .  EC=", 0, 11);
  display.print("N=     P=     K=    ", 6, 22);
  display.print("T=  .", 6, 33);
  display.print("RH=  .", 6, 44);

  if (soil.pH != ERRORCODE) {
    display.printNumI(soil.pH / 100, 60, 11);
    display.printNumI(soil.pH / 10 % 10, 72, 11, 1);
  } else display.print("err", 60, 11);

  if (soil.EC != ERRORCODE) display.printNumI(soil.EC, 102, 11);
  else display.print("err", 102, 11);

  if (soil.N != ERRORCODE) {
    display.printNumI(soil.N, 18, 22);
    display.printNumI(soil.P, 60, 22);
    display.printNumI(soil.K, 102, 22);
  } else {
    display.print("err", 18, 22);
    display.print("err", 60, 22);
    display.print("err", 102, 22);
  }

  if (soil.temp != ERRORCODE) {
    display.printNumI(soil.temp / 10, 18, 33);
    display.printNumI(soil.temp % 10, 36, 33);
    display.printNumI(soil.RH / 10, 24, 44, 2);
    display.printNumI(soil.RH % 10, 42, 44);
  } else {
    display.print("err", 18, 33);
    display.print("err", 24, 44);
  }
  
  display.update();
}

void showDisplay() {
  showDisplaySensors();
}
