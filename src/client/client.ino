// Подключаем библиотеки
#include "WiFi.h"
#include "AsyncUDP.h"

#include <Adafruit_BMP280.h>
#include <DHT.h>  //библеотеки

// Создаём переменную состояния кнопки
bool keyState = false;

// Массив данных для отправки
int DATA[9];

// Определяем название и пароль точки доступа
const char *ssid = "DB";
const char *password = "12345678";

// Определяем порт
const uint16_t PORT = 49152;

// Создаём объект с IP-адресом точки доступа
IPAddress ADDR(192, 168, 4, 1);

// Создаём объект UDP соединения
AsyncUDP udp;

const byte pin_DHT = 17;
const byte pin_flame = 26;
const byte pin_water = 2;
const byte pin_gass = 4;
const byte pin_vibr = 25;
const byte pin_Light = 13;
const byte pin_LED = 18;  //пины датчиков

Adafruit_BMP280 bmp280;  // A4-SDA/SDI A5-SCL/SCK
DHT dht(pin_DHT, DHT11);

bool norm_val_vibr = 1;

int time_vibr_to_none = 1000;
unsigned long time_vibr = 0;
bool last_vibr = 0;
bool vibr = false;

const int timer = 2500;
const int timer_check_sistem = 2500;  //интервал

unsigned long millis_time_check_sistem = 0;
unsigned long millis_time = 0;  //сохранение предыдущих показаний millis();


NetworkServer server(80);

///****************функции чтения****************////

int readSensorVibr() {  // чтение датчика вибрации
  if (digitalRead(pin_vibr) != norm_val_vibr) {
    time_vibr = millis();
    norm_val_vibr = digitalRead(pin_vibr);
    return 1;
  } else {
    if (millis() - time_vibr >= time_vibr_to_none) {
      return 0;
    }
  }
}

void readAllSensors() {
  DATA[0] = digitalRead(pin_flame);
  DATA[1] = map(analogRead(pin_water), 0, 4096, 0, 50);
  DATA[2] = readSensorVibr();
  DATA[3] = digitalRead(pin_flame);
  DATA[4] = bmp280.readPressure() / 100;
  DATA[5] = dht.readTemperature();
  DATA[6] = dht.readHumidity();
  DATA[7] = digitalRead(pin_gass);
  DATA[8] = digitalRead(pin_Light);
}

void begin() {
  pinMode(pin_flame, INPUT_PULLUP);
  pinMode(pin_water, INPUT_PULLUP);
  pinMode(pin_vibr, INPUT_PULLUP);
  pinMode(pin_DHT, INPUT_PULLUP);
  pinMode(pin_gass, INPUT_PULLUP);
  pinMode(pin_Light, INPUT);
  pinMode(pin_LED, OUTPUT);  //настройка пинов

  digitalWrite(pin_LED, HIGH);
  bmp280.begin(0x76);
  dht.begin();  //вкл. библеотек
}

void setup() {
  begin();
  // Инициируем последовательный порт
  Serial.begin(115200);

  // Устанавливаем режим работы в качестве клиента
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Ждём подключения WiFi
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  // Если удалось подключиться по UDP
  if (udp.connect(ADDR, PORT)) {
    Serial.println("UDP подключён");
  }

  // Если подключение не удалось
  else {
    Serial.println("UDP не подключён");
  }
}

void loop() {
  readAllSensors();
  // Отправляем данные серверу побайтово
  udp.broadcastTo((uint8_t *)&DATA, sizeof(DATA), PORT);

  // Если соединение WiFi прервано
  if (WiFi.status() != WL_CONNECTED) {
    // Вызываем функцию setup(), для повторного подключения
    setup();
  }

  delay(1000);
}
