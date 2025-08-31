#include <HTTP_Method.h>
#include <Middlewares.h>
#include <Uri.h>
#include <WebServer.h>

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>  //библеотеки для вайфай сервера

#include <Adafruit_BMP280.h>
#include <DHT.h>  //библеотеки

String HTTP_req;  // для хранения HTTP запроса

const byte pin_DHT = 17;
const byte pin_flame = 26;
const byte pin_water = 2;
const byte pin_gass = 4;
const byte pin_vibr = 25;
const byte pin_Light = 13;  
const byte pin_LED = 18;    //пины датчиков

Adafruit_BMP280 bmp280;  // A4-SDA/SDI A5-SCL/SCK
DHT dht(pin_DHT, DHT11);

String result_flame = " ";
String result_light = " ";
String result_water = " ";
String result_dht_hum = " ";
String result_dht_tem = " ";
String result_prs = " ";
String result_gass = " ";
String result_vibr = "нет";

const bool norm_val_flame = 1;
const bool norm_val_water = 1;
 bool norm_val_vibr = 1;
const bool norm_val_gass_co2 = 1;
const bool norm_light = 1; 

int time_vibr_to_none = 1000;
unsigned long time_vibr = 0;
bool last_vibr = 0;
bool vibr = false;

const int timer = 2500;
const int timer_check_sistem = 2500;  //интервал

unsigned long millis_time_check_sistem = 0;
unsigned long millis_time = 0;  //сохранение предыдущих показаний millis();

// Set these to your desired credentials.
const char *ssid = "DB";
const char *password = "12345678";

NetworkServer server(80);

///****************функции чтения****************////

void readSensorFlame(NetworkClient clflame) {  // чтение датчика огня
  if (digitalRead(pin_flame) == norm_val_flame) result_flame = "есть";
  else result_flame = "нет";
  clflame.println(result_flame);
}

void readSensorWater(NetworkClient clwater) {  // чтение датчика воды
  clwater.print(map(analogRead(pin_water),0,4096,0,50));
  clwater.println(" mm");
}

void readSensorVibr(NetworkClient clvibr) {  // чтение датчика вибрации
  if (digitalRead(pin_vibr) != norm_val_vibr) {
    time_vibr = millis();
    result_vibr = "есть";
    norm_val_vibr = digitalRead(pin_vibr);
  } else {
    if (millis() - time_vibr >= time_vibr_to_none) {
      result_vibr = "нет";
    }
  }
  clvibr.println(result_vibr);
}

void readSensorPrs(NetworkClient clprs) {  // чтение датчика давления
  result_prs = String(bmp280.readPressure()/100);
  result_prs += " mbar";

  clprs.println(result_prs);
}

void readSensorDHTtem(NetworkClient clDHT) {  // чтение датчика DHT
  result_dht_tem = String(dht.readTemperature());
  result_dht_tem += " °C";

  clDHT.println(result_dht_tem);
}

void readSensorDHThum(NetworkClient clDHT) {  // чтение датчика DHT
  result_dht_hum = String(dht.readHumidity());
  result_dht_hum += " %";

  clDHT.println(result_dht_hum);
}
void readSensorMQ2(NetworkClient clMQ2) {  // чтение датчика MQ2
  if (digitalRead(pin_gass) == norm_val_gass_co2) result_gass = "норма";
  else if (digitalRead(pin_gass) != norm_val_gass_co2) result_gass = "повышенный";

  clMQ2.println(result_gass);
}

void readSensorLight(NetworkClient clLight) {  // чтение датчика освещёности
  if (digitalRead(pin_Light) == 0) result_light = "день";
  else result_light = "ночь";
  
  clLight.println(result_light);
}

void begin() {
  pinMode(pin_flame, INPUT_PULLUP);
  pinMode(pin_water, INPUT_PULLUP);
  pinMode(pin_vibr, INPUT_PULLUP);
  pinMode(pin_DHT, INPUT_PULLUP);
  pinMode(pin_gass, INPUT_PULLUP);
  pinMode(pin_Light, INPUT);  
  pinMode(pin_LED, OUTPUT);//настройка пинов

  digitalWrite(pin_LED, HIGH);

  dht.begin();  //вкл. библеотек

//  pinCheck();  //считываем к-во подключенных датчиков
}

//void pinCheck() {
//  test_flame = digitalRead(pin_flame);
//  test_vibr = digitalRead(pin_vibr);
//  test_water = digitalRead(pin_water);
//  test_dht = dht.readHumidity();
//  test_gass = digitalRead(pin_gass);
//  test_light = digitalRead(pin_Light);
//
//  if (test_flame == test_true_flame) {
//    pin_flame_correct = 1;
//  }
//
//  if (test_light == test_true_light) {
//    pin_light_correct = 1;
//  }
//
//  if (test_vibr == test_true_vibr) {
//    pin_vibr_correct = 1;
//  }
//
//  if (test_water != test_true_water) {
//    pin_water_correct = 1;
//  }
//
//  if (test_gass == test_true_gass) {
//    pin_gass_correct = 1;
//  }
//
//  if (isnan(test_dht)) {
//  } else {
//    pin_DHT_correct = 1;
//  }
//
//  if (bmp280.begin(BMP280_ADDRESS - 1)) pin_prs_correct = 1;
//}
//

void setup() {
  begin();

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (true)
      ;
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void web_site_main() {
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    while (client.connected()) {    // loop while the client's connected
      if (client.available()) {     // if there's bytes to read from the client,
        char c = client.read();     // read a byte, then
        Serial.write(c);            // print it out the serial monitor
        HTTP_req += c;
        if (c == '\n') {  // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // webEventSensors();
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            // the content of the HTTP response follows the header:

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: keep-alive");
            client.println();

            if (HTTP_req.indexOf("ajax_flame") > -1) {
              readSensorFlame(client);
            }  // новая функция для отправки данных датчика
            else if (HTTP_req.indexOf("ajax_light") > -1) {
              readSensorLight(client);
            }  // новая функция для отправки данных датчика
            else if (HTTP_req.indexOf("ajax_water") > -1) {
              readSensorWater(client);
            }  // новая функция для отправки данных датчика
            else if (HTTP_req.indexOf("ajax_vibr") > -1) {
              readSensorVibr(client);
            }  // новая функция для отправки данных датчика
            else if (HTTP_req.indexOf("ajax_prs") > -1) {
              readSensorPrs(client);
            }  // новая функция для отправки данных датчика
            else if (HTTP_req.indexOf("ajax_DHTtem") > -1) {
              readSensorDHTtem(client);
            }  // новая функция для отправки данных датчика
            else if (HTTP_req.indexOf("ajax_DHThum") > -1) {
              readSensorDHThum(client);
            }  // новая функция для отправки данных датчика
            else if (HTTP_req.indexOf("ajax_MQ2") > -1) {
              readSensorMQ2(client);
            }  // новая функция для отправки данных датчика
            else {
              client.println();
              client.println("<!DOCTYPE html>");
              client.println("<html lang='ru' style='height: 100%;'>");
              client.println("<head>");
              client.println("    <!-- Базовые HTML настройки -->");
              client.println("    <meta charset='UTF-8'>");
              client.println("    <meta name='viewport' content='width=device-width, initial-scale=1.0'>");
              client.println("    <title>Disater Box Help Website</title>");
              client.println("<script>");
              client.println("function readSensorFlame() {");
              client.println("  nocache = \"&nocache=\"\
                                                    + Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_flame_1\")\.innerHTML = this.responseText;");
              client.println("document.getElementById(\"switch_flame_2\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_flame\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("function readSensorLight() {");
              client.println("  nocache = \"&nocache=\"\+ Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_light\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_light\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("function readSensorWater() {");
              client.println("  nocache = \"&nocache=\"\+ Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_water_1\")\.innerHTML = this.responseText;");
              client.println("document.getElementById(\"switch_water_2\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_water\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("function readSensorVibr() {");
              client.println("  nocache = \"&nocache=\"\+ Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_vibr_1\")\.innerHTML = this.responseText;");
              client.println("document.getElementById(\"switch_vibr_2\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_vibr\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("function readSensorPrs() {");
              client.println("  nocache = \"&nocache=\"\+ Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_prs_1\")\.innerHTML = this.responseText;");
              client.println("document.getElementById(\"switch_prs_2\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_prs\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("function readSensorDHTtem() {");
              client.println("  nocache = \"&nocache=\"\+ Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_DHTtem_1\")\.innerHTML = this.responseText;");
              client.println("document.getElementById(\"switch_DHTtem_2\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_DHTtem\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("function readSensorDHThum() {");
              client.println("  nocache = \"&nocache=\"\+ Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_DHThum_1\")\.innerHTML = this.responseText;");
              client.println("document.getElementById(\"switch_DHThum_2\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_DHThum\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("function readSensorMQ2() {");
              client.println("  nocache = \"&nocache=\"\+ Math.random() * 1000000;");
              client.println("  var request = new XMLHttpRequest();");
              client.println("  request.onreadystatechange = function() {");
              client.println("if (this.readyState == 4) {");
              client.println("if (this.status == 200) {");
              client.println("if (this.responseText != null) {");
              client.println("document.getElementById(\"switch_MQ2_1\")\.innerHTML = this.responseText;");
              client.println("document.getElementById(\"switch_MQ2_2\")\.innerHTML = this.responseText;");
              client.println("}}}}");
              client.println("  request.open(\"GET\", \"ajax_MQ2\" + nocache, true);");
              client.println("  request.send(null);");
              client.println("}");
              client.println("</script>");
              client.println("    <style>");
              client.println("        :root{");
              client.println("            --textColor: #000000;");
              client.println("            --have: #ff0000;");
              client.println("            --havent: #008000;");
              client.println("        }");
              client.println("        *{");
              client.println("            margin: 0;");
              client.println("            padding: 0;");
              client.println("            box-sizing: border-box;");
              client.println("            list-style: none;");
              client.println("        }");
              client.println("        html,");
              client.println("        body{");
              client.println("            height: 100%;");
              client.println("        }");
              client.println("        body{");
              client.println("            background: white;");
              client.println("            display: flex;");
              client.println("            justify-content: space-evenly;");
              client.println("        }");
              client.println("        @media screen and (min-width:768px) {");
              client.println("            body .menu > .showing button{");
              client.println("                font-size: 30px;");
              client.println("                width: 200px;");
              client.println("                overflow: hidden;");
              client.println("            }");
              client.println("            .settings{");
              client.println("                height: 50px;");
              client.println("            }");
              client.println("            .settings > a{");
              client.println("                font-size: 20px;");
              client.println("            }");
              client.println("            .menu > .showing span{");
              client.println("                font-size: 25px;");
              client.println("            }");
              client.println("            .leftMain > .showing > li > span, .rightMain > .showingimgs > ul > li > span{");
              client.println("                font-size: 24px;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (min-width: 320px){");
              client.println("            body .showingimgs li {");
              client.println("                border-right: 2px solid gray;");
              client.println("            }");
              client.println("            body .showing span{");
              client.println("                font-size: 100%;");
              client.println("            }");
              client.println("            body .showing > li{");
              client.println("                flex-wrap: wrap;");
              client.println("                width: 100%;");
              client.println("            }");
              client.println("            .showingimgs li > .img > img{");
              client.println("                height: 25%;");
              client.println("                width: 50%;");
              client.println("            }");
              client.println("            .showingimgs li > span{");
              client.println("                width: 100%;");
              client.println("                display: flex;");
              client.println("                flex-wrap: wrap;");
              client.println("                height: 12%;");
              client.println("            }");
              client.println("            body .showingimgs li:nth-child(1) > span > span{");
              client.println("                text-align: center;");
              client.println("                height: 50%;");
              client.println("                overflow-wrap: anywhere;");
              client.println("                white-space: normal;");
              client.println("            }");
              client.println("            body .value{");
              client.println("                display: flex;");
              client.println("                justify-content: center;");
              client.println("                align-items: center;");
              client.println("                font-size: 20px;");
              client.println("            }");
              client.println("            body .settings{");
              client.println("                width: 100%;");
              client.println("            }");
              client.println("            .back{");
              client.println("                width: 100px;");
              client.println("            }");
              client.println("            .menu > .showing button{");
              client.println("                width: 40px;");
              client.println("                overflow: hidden;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (min-width: 1024px){");
              client.println("            .showingimgs li > .img > img{");
              client.println("                width: 20%;");
              client.println("                height: 40%;");
              client.println("            }");
              client.println("            .showing span{");
              client.println("                font-size: 200%;");
              client.println("            }");
              client.println("            .showingimgs span{");
              client.println("                font-size: 100%;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (min-width:1440px) {");
              client.println("            body .showing span{");
              client.println("                font-size: 240%;");
              client.println("            }");
              client.println("            body .showingimgs span{");
              client.println("                font-size: 120%;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (min-width:769px) {");
              client.println("            body .showingimgs > ul > li > .img > img{");
              client.println("                width: 50%;");
              client.println("                height: 50%;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (max-width:425px) {");
              client.println("            body .up{");
              client.println("                border-bottom: 2px solid gray;");
              client.println("            }");
              client.println("            body .showingimgs > ul > li{");
              client.println("                border-right: 2px solid gray;");
              client.println("            }");
              client.println("            body > .rightMain{");
              client.println("                padding: 0;");
              client.println("            }");
              client.println("            body .date{");
              client.println("                margin-bottom: 10px;");
              client.println("            }");
              client.println("            .menu > button{");
              client.println("                width: 100px;");
              client.println("            }");
              client.println("            body > .leftMain > .showing > li > span{");
              client.println("                font-size: 22.5px;");
              client.println("            }");
              client.println("            .showingimgs li > .img > img{");
              client.println("                height: 25%;");
              client.println("                width: 100%;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (max-width:768px) {");
              client.println("            .leftMain > .showing > li > span, .rightMain > .showingimgs > ul > li > span{");
              client.println("                font-size: 22px;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (width:768px) {");
              client.println("            .showingimgs li > .img > img{");
              client.println("                height: 40%;");
              client.println("            }");
              client.println("        }");
              client.println("        @media screen and (width:425px) {");
              client.println("            body .showingimgs > ul > li > .img > img{");
              client.println("                width: 50%;");
              client.println("                height: 50%;");
              client.println("            }");
              client.println("        }");
              client.println("        .showingimgs > ul > li > .titttle{");
              client.println("            overflow-wrap: anywhere;");
              client.println("            white-space: normal;");
              client.println("            text-align: center;");
              client.println("        }");
              client.println("        .leftMain{");
              client.println("            padding: 10px;");
              client.println("            display: flex;");
              client.println("            flex-direction: column;");
              client.println("            width: 30%;");
              client.println("            height: 100%;");
              client.println("            border-right: 2px solid gray;");
              client.println("        }");
              client.println("        .rightMain{");
              client.println("            width: 70%;");
              client.println("            height: 100%;");
              client.println("            padding: 10px;");
              client.println("        }");
              client.println("            .logo { margin-bottom: 10px; }");
              client.println("            .date { margin-bottom: 50px; }");
              client.println("            .showing { height: 75%; display: flex; flex-direction: column; justify-content: space-between; flex-wrap: nowrap; }");
              client.println("            .showingimgs > ul > li > .titttle { text-align: center; }");
              client.println("            .showing > li { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid gray; }");
              client.println("            .showing span { font-size: 100%; }");
              client.println("            .showing .pokasanie { text-align: center; }");
              client.println("            .settings { width: 50%; position: static; }");
              client.println("            .showing { margin-bottom: 30px; }");
              client.println("            .showingimgs { width: 100%; height: 100%; display: flex; flex-direction: column; justify-content: space-evenly; border: 1px solid gray; }");
              client.println("            .up, .bottom { width: 100%; height: 49%; display: flex; justify-content: space-evenly; }");
              client.println("            .up { border-bottom: 5px solid gray; }");
              client.println("            .showingimgs li { height: 100%; width: 30%; padding:5%;display: flex; flex-direction: column; justify-content: space-between; align-items: center; border-right: 5px solid gray; }");
              client.println("            .showingimgs > .up > li:nth-child(3), .showingimgs > .bottom > li:nth-child(3) { border-right: none; }");
              client.println("            .showingimgs .img { width: 100%; height: 75%; display: flex; flex-direction: column; justify-content: space-evenly; align-items: center; }");
              client.println("            .img > img { width: 10%; height: 70%; }");
              client.println(".img>p {font-size: 100px;}");
              client.println("            .values { width: 100%; height: 25%; }");
              client.println("            .showingimgs > .bottom > li:nth-child(1) > .img { height: 66%; }");
              client.println("            .value { text-align: center; width: 100%; height: 50%; }");
              client.println("            .have { text-align: center; width: 100%; height: 50%; background: var(--havent); border: 2px solid black; }");
              client.println("            body .value_tem { border: 2px solid black; border-bottom: none; }");
              client.println("            body .value_hum { border: 2px solid black; }");
              client.println("            body .have_flame { height: 50%; }");
              client.println("            body .have_rain, .have_wibro, .have_wind { border-bottom: none; }");
              client.println("            body .value_rain,.value_gass,.value_flame,.have_gas, .value_wibro, .value_wind { border: 2px solid black; }");
              client.println("            body .have_gas { height: 50%; border-bottom: none;}");
              client.println("            .menu { display: none; position: absolute; z-index: 52; height: 90%; width: 90%; top: 5%; left: 5%; border: 2px solid black; background: white; }");
              client.println("            .bloored { filter: blur(100%); }");
              client.println("            .back { width: 15%; height: 5%; }");
              client.println("            .on, .off { width: 100px; height: 100%; }");
              client.println("            .tittle { width: 150px; }");
              client.println("        </style>");
              client.println("    </head>");
              client.println("    <body>");
              client.println("        <div class='menu'>");
              client.println("            <button class='back'><a href='#'>Назад</a></button>");
              client.println("            <ul class='showing'>");
              client.println("                <li><span class='tittle'>Датчик осадков</span> <span class='pokasanie firstt'>Выкл</span> <button class='on'>Включить</button> <button class='off'>Выключить</button></li>");       // done
              client.println("                <li><span class='tittle'>Датчик освещёности</span> <span class='pokasanie ninthh'>Выкл</span> <button class='on'>Включить</button> <button class='off'>Выключить</button></li>");   // done
              client.println("                <li><span class='tittle'>Датчик газа</span> <span class='pokasanie fourthh'>Выкл</span><button class='on'>Включить</button> <button class='off'>Выключить</button></li>");          //done
              client.println("                <li><span class='tittle'>Датчик влажности</span> <span class='pokasanie fifthh'>Выкл</span><button class='on'>Включить</button> <button class='off'>Выключить</button></li>");      //done
              client.println("                <li><span class='tittle'>Датчик температуры</span> <span class='pokasanie secondd'>Выкл</span><button class='on'>Включить</button> <button class='off'>Выключить</button></li>");   //done
              client.println("                <li><span class='tittle'>Датчик вибрации</span> <span class='pokasanie sixthh'>Выкл</span><button class='on'>Включить</button> <button class='off'>Выключить</button></li>");       //done
              client.println("                <li><span class='tittle'>Датчик огня</span> <span class='pokasanie seventhh'>Выкл</span><button class='on'>Включить</button> <button class='off'>Выключить</button></li>");         //done
              client.println("                <li><span class='tittle'>Датчик давления</span> <span class='pokasanie eighteenthh'>Выкл</span><button class='on'>Включить</button> <button class='off'>Выключить</button></li>");  //error
              client.println("        </ul>");
              client.println("    </div>");
              client.println("    <div class='leftMain bloored'>");
              client.println("        <div class='logo'>");
              client.println("            <span class='title'>DisasterBOX</span>");
              client.println("        </div>");
              client.println("        <span class='date'></span>");
              client.println("        <ul class='showing'>");
              client.println("            <li><span class='tittle'>Осадки</span > <span class='pokasanie first' >");  //done
              client.println(
                "<p id=\"switch_water_1\">Loading...</p>");
              client.println("</span></li>");
              client.println("            <li><span class='tittle'>Температура</span> <span class='pokasanie second'>");
              client.println(
                "<p id=\"switch_DHTtem_1\">Loading...</p>");  //done
              client.println("</span></li>");
              client.print("<li><span class='tittle'>Крит. доза газов</span> <span class=\'pokasanie fourth\'>");
              client.println(
                "<p id=\"switch_MQ2_1\">Switch state: Not requested...</p>");  //done
              client.println("</span></li>");
              client.println("            <li><span class='tittle'>Влажность</span> <span class='pokasanie fifth'>");
              client.println(
                "<p id=\"switch_DHThum_1\">Loading...</p>");  //done
              client.println("</span></li>");
              client.println("            <li><span class='tittle'>Опасность землетрясения</span> <span class='pokasanie sixth'>");
              client.println(
                "<p id=\"switch_vibr_1\">Loading...</p>");  //done
              client.println("</span></li>");
              client.println("            <li><span class='tittle'>Огонь</span> <span class='pokasanie seventh' ><p id=\"switch_flame_1\">");  //done
              client.println("</p></span></li>");
              client.println("            <li><span class='tittle'>Давление</span> <span class='pokasanie third'>");
              client.println(
                "<p id=\"switch_prs_1\">Loading...</p>");  //done

              client.println("</span></li>");
              client.println("            <li><span class='tittle'>Освещение</span> <span class='pokasanie ninth'>");
              client.println(
                "<p id=\"switch_light\">Loading...</p>");  //error

              client.println("</span></li>");
              client.println("        </ul>");
              client.println("        <button class='settings'><a href='#'>Настройка датчиков</a></button>");
              client.println("    </div>");
              client.println("    <div class='rightMain'>");
              client.println("        <div class='showingimgs'>");
              client.println("            <ul class='up'>");
              client.println("                <li>");
              client.println("                    <span class='titttle needfix'><span>Темп. ,</span><span>влажн.</span></span>");
              client.println("                    <div class='img'>");
              client.println("                        <p>🌡️</p>");
              client.println("                    </div>");
              client.println("                    <div class='values'>");
              client.println("                        <div class='value value_tem'><span>");
              client.println(
                "<p id=\"switch_DHTtem_2\">Loading...</p>");
              client.println("</span></div>");
              client.println("                        <div class='value value_hum'><span>");
              client.println(
                "<p id=\"switch_DHThum_2\">Loading...</p>");
              client.println("</span></div>");
              client.println("                    </div>");
              client.println("                </li>");
              client.println("                <li>");
              client.println("                    <span class='titttle'>Осадки</span>");
              client.println("                    <div class='img'>");
              client.println("                        <p>🌧️</p>");
              client.println("                    </div>");
              client.println("                    <div class='values'>");
              client.println("                        <div class='have have_rain'></div>");
              client.println("                        <div class='value value_rain'>");
              client.println(
                "<p id=\"switch_water_2\">Loading...</p>");
              client.println("  <span>");
              client.println("</span></div>");
              client.println("                    </div>");
              client.println("                </li>");
              client.println("                <li>");
              client.println("                    <span class='titttle'>Опасность землетрясения</span>");
              client.println("                    <div class='img'>");
              client.println("                        <p>♒</p>");
              client.println("                    </div>");
              client.println("                    <div class='values'>");
              client.println("                        <div class='have have_wibro'></div>");
              client.println("                        <div class='value value_wibro'><span>");
              client.println(
                "<p id=\"switch_vibr_2\">Loading...</p>");
              client.println("</span></div>");
              client.println("                    </div>");
              client.println("                </li>");
              client.println("            </ul>");
              client.println("            <ul class='bottom'>");
              client.println("                <li>");
              client.println("                    <span class='titttle'>Огонь</span>");
              client.println("                    <div class='img'>");
              client.println("                        <p>🔥</p>");
              client.println("                    </div>");
              client.println("                    <div class='values'>");
              client.println("                        <div class='have have_flame'>");
              client.println("</div>");
              client.println("                        <div class='value value_flame'><span>");
              client.println(
                "<p id=\"switch_flame_2\">Loading...</p>");
              client.println("</span></div>");
              client.println("                    </div>");
              client.println("                </li>");
              client.println("                <li>");
              client.println("                    <span class='titttle'>Газ</span>");
              client.println("                    <div class='img'>");
              client.println("                        <p>💨</p>");
              client.println("                    </div>");
              client.println("                    <div class='values'>");
              client.println("                <div class='have have_gass'>");
              client.println("</div>");
              client.println("                        <div class='value value_gass'><span>");
              client.println(
                "<p id=\"switch_MQ2_2\">Loading...</p>");
              client.println("</span></div>");
              client.println("            </div>");
              client.println("            </li>");
              client.println("            <li>");
              client.println("            <span class='titttle'>Давление</span>");
              client.println("            <div class='img'>");
              client.println("                <p>🧊</p>");
              client.println("            </div>");
              client.println("            <div class='values'>");
              client.println("                <div class='have have_wind'></div>");
              client.println("                <div class='value value_wind'><span>");
              client.println(
                "<p id=\"switch_prs_2\">Loading...</p>");
              client.println("</span></div>");
              client.println("            </div>");
              client.println("            </li>");
              client.println("        </ul>");
              client.println("        </div>");
              client.println("        <script>");
              client.println("            let date = document.querySelector('.date')");
              client.println("            setInterval(() => {");
              client.println("                date.innerHTML = `${(new Date).getDay()}.${(new Date).getMonth()}.${(new Date).getFullYear()} ${(new Date).getHours()}:${(new Date).getMinutes()}:${(new Date).getSeconds()}`");
              client.println("            },1000)");
              client.println("            let have_tem = document.querySelector('.value_tem')");       //clear
              client.println("            let have_rain = document.querySelector('.have_rain')");      //clear
              client.println("            let value_rain = document.querySelector('.value_rain')");    //clear
              client.println("            let have_wibro = document.querySelector('.have_wibro')");    //clear
              client.println("            let value_wibro = document.querySelector('.value_wibro')");  //clear
              client.println("            let have_flame = document.querySelector('.have_flame')");    //clear
              client.println("            let value_flame = document.querySelector('.value_flame')");  //clear
              client.println("            let have_gas = document.querySelector('.have_gass')");       //done
              client.println("            let value_gas = document.querySelector('.value_gass')");     //done
              client.println("            let have_hum= document.querySelector('.value_hum')");        //clear
              client.println("            let have_wind = document.querySelector('.have_wind')");      //clear
              client.println("            let value_wind = document.querySelector('.value_wind')");    //clear
              client.println("            let firstv = document.querySelector('.first')");
              client.println("            let ninthv = document.querySelector('.ninth')");
              client.println("            let eighteenthv = document.querySelector('.eighteenth')");
              client.println("            let secondv = document.querySelector('.second')");
              client.println("            let thirdv = document.querySelector('.third')");
              client.println("            let fourthv = document.querySelector('.fourth')");
              client.println("            let fifthv = document.querySelector('.fifth')");
              client.println("            let sixthv = document.querySelector('.sixth')");
              client.println("            let seventhv = document.querySelector('.seventh')");
              client.println("if (+firstv.innerHTML.slice(0, firstv.innerHTML.length - 2) > 20){have_rain.style.background = 'var(--have)';have_rain.innerHTML = 'ОПАСНОСТЬ';}");
              client.println("if (+secondv.innerHTML.slice(0, secondv.innerHTML.length - 4) > 2){have_wibro.style.background = 'var(--have)';have_wibro.innerHTML = 'ОПАСНОСТЬ';}");
              client.println("if (+thirdv.innerText.slice(0, thirdv.innerText.length - 3) > 150 || +thirdv.innerText.slice(0, thirdv.innerText.length - 3) < 50){have_wind.style.background = 'var(--have)';have_wind.innerHTML = 'ОПАСНОСТЬ';}");
              client.println("setInterval(() => {");
              client.println("readSensorLight()");
              client.println("readSensorFlame()");
              client.println("readSensorMQ2()");
              client.println("readSensorVibr()");
              client.println("},100)");
              client.println("setInterval(() => {");
              client.println("readSensorWater()");
              client.println("},500)");
              client.println("setInterval(() => {");
              client.println("readSensorDHTtem()");
              client.println("readSensorDHThum()");
              client.println("},1000)");
              client.println("setInterval(() => {");
              client.println("readSensorPrs()");
              client.println("},5000)");
              client.println("setInterval(() => {");
              client.println("if (document.querySelector('.firstt').innerHTML == 'Выкл') {firstv.style.display = 'none'; firstv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through';have_rain.style.display = 'none';value_rain.style.display = 'none'}");
              client.println("else {firstv.style.display = 'inline'; firstv.parentNode.querySelector('.tittle').style.textDecoration = 'none'; have_rain.style.display = 'flex';value_rain.style.display = 'flex'}");
              client.println("if (document.querySelector('.fourthh').innerHTML == 'Выкл') {fourthv.style.display = 'none'; fourthv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through';have_gas.style.display = 'none';value_gas.style.display = 'none'}");
              client.println("else {fourthv.style.display = 'inline'; fourthv.parentNode.querySelector('.tittle').style.textDecoration = 'none';have_gas.style.display = 'flex';value_gas.style.display = 'flex'}");
              client.println("if (document.querySelector('.fifthh').innerHTML == 'Выкл') {fifthv.style.display = 'none'; fifthv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through';have_hum.style.display = 'none'}");
              client.println("else {fifthv.style.display = 'inline'; fifthv.parentNode.querySelector('.tittle').style.textDecoration = 'none';have_hum.style.display = 'flex'}");
              client.println("if (document.querySelector('.secondd').innerHTML == 'Выкл') {secondv.style.display = 'none'; secondv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through';have_tem.style.display = 'none'}");
              client.println("else {secondv.style.display = 'inline'; secondv.parentNode.querySelector('.tittle').style.textDecoration = 'none';have_tem.style.display = 'flex'}");
              client.println("if (document.querySelector('.sixthh').innerHTML == 'Выкл') {sixthv.style.display = 'none'; sixthv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through';have_wibro.style.display = 'none';value_wibro.style.display = 'none'}");
              client.println("else {sixthv.style.display = 'inline'; sixthv.parentNode.querySelector('.tittle').style.textDecoration = 'none';have_wibro.style.display = 'flex';value_wibro.style.display = 'flex'}");
              client.println("if (document.querySelector('.eighteenthh').innerHTML == 'Выкл') {thirdv.style.display = 'none'; thirdv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through';have_wind.style.display = 'none';value_wind.style.display = 'none'}");
              client.println("else {thirdv.style.display = 'inline'; thirdv.parentNode.querySelector('.tittle').style.textDecoration = 'none';have_wind.style.display = 'flex';value_wind.style.display = 'flex'}");
              client.println("if (document.querySelector('.ninthh').innerHTML == 'Выкл') {ninthv.style.display = 'none'; ninthv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through'}");
              client.println("else {ninthv.style.display = 'inline'; ninthv.parentNode.querySelector('.tittle').style.textDecoration = 'none'}");
              client.println("if (document.querySelector('.seventhh').innerHTML == 'Выкл') {seventhv.style.display = 'none'; seventhv.parentNode.querySelector('.tittle').style.textDecoration = 'line-through';have_flame.style.display = 'none';value_flame.style.display = 'none'}");
              client.println("else {seventhv.style.display = 'inline'; seventhv.parentNode.querySelector('.tittle').style.textDecoration = 'none';have_flame.style.display = 'flex';value_flame.style.display = 'flex'}");
              client.println("},10)");
              client.println("            let set = document.querySelector('.settings')");
              client.println("            let menu = document.querySelector('.menu')");
              client.println("            set.addEventListener('click', () => {");
              client.println("                menu.style.display = 'block'");
              client.println("                menu.querySelectorAll('.on').forEach(el => {");
              client.println("                    el.addEventListener('click', () => {");
              client.println("                        el.parentNode.querySelector('.pokasanie').innerHTML = 'Вкл'");
              client.println("                    })");
              client.println("                })");
              client.println("                menu.querySelectorAll('.off').forEach(el => {");
              client.println("                    el.addEventListener('click', () => {");
              client.println("                        el.parentNode.querySelector('.pokasanie').innerHTML = 'Выкл'");
              client.println("                    })");
              client.println("                })");
              client.println("                document.querySelector('.back').addEventListener('click', () => {");
              client.println("                    menu.style.display = 'none'");
              client.println("                })");
              client.println("            })");
              //              client.println(" window.addEventListener('keydown', e => {");
              //              client.println("   if (e.ctrlKey && e.code == 'O'){");
              //              client.println("   menu.style.display = 'block';");
              //              client.println("                 menu.querySelectorAll('.on').forEach(el => {");
              //              client.println("                     el.addEventListener('click', () => {");
              //              client.println("                         el.parentNode.querySelector('.pokasanie').innerHTML = 'Вкл';");
              //              client.println("                     })");
              //              client.println("                 }) ");
              //              client.println("       menu.querySelectorAll('.off').forEach(el => {");
              //              client.println("        el.addEventListener('click', () => {");
              //              client.println("               el.parentNode.querySelector('.pokasanie').innerHTML = 'Выкл';");
              //              client.println("        })");
              //              client.println("    })");
              //              client.println("   document.querySelector('.back').addEventListener('click', () => {");
              //              client.println("        menu.style.display = 'none';");
              //              client.println("    })");
              //              client.println("   }");
              //              client.println("   if (e.ctrlKey && e.code == 'T'){");
              //              client.println("     document.querySelector('.pokasanie').forEach(el => el.innerHTML = 'Вкл')");
              //              client.println(" }}");
              client.println("if (window.innerWidth == 768) {document.querySelector('.needfix').innerHTML = 'Температур. и влажность'}");
              client.println("if (window.innerWidth <= 425) {");
              client.println("document.querySelectorAll('.on').forEach(el => el.innerHTML = 'Вкл');");
              client.println("document.querySelectorAll('.off').forEach(el => el.innerHTML = 'Выкл');");
              client.println("document.querySelector('.needfix').innerHTML = 'Темп., влаж.';");
              client.println("secondv.parentNode.querySelector('.tittle').innerHTML = 'Темп.';");
              client.println("fifthv.parentNode.querySelector('.tittle').innerHTML = 'Влажн.';");
              client.println("sixthv.parentNode.querySelector('.tittle').innerHTML = 'Землетрясения';");
              client.println("}");
              client.println("if (window.innerWidth <= 768) {document.querySelectorAll('.on').forEach(el => el.innerHTML = 'Вкл');");
              client.println("document.querySelectorAll('.off').forEach(el => el.innerHTML = 'Выкл');}");
              client.println("            if (window.innerWidth <= 425){");
              client.println("                document.querySelectorAll('.on').forEach(el => el.innerHTML = 'Вкл')");
              client.println("                document.querySelectorAll('.off').forEach(el => el.innerHTML = 'Выкл')");
              client.println("                document.querySelector('.needfix').innerHTML = 'Темп., влаж.'");
              client.println("                secondv.parentNode.querySelector('.tittle').innerHTML = 'Темп.'");
              client.println("                fifthv.parentNode.querySelector('.tittle').innerHTML = 'Влажн.'");
              client.println("                sixthv.parentNode.querySelector('.tittle').innerHTML = 'Толчки'");
              client.println("            }");
              client.println("            window.addEventListener('resize', () => {");
              client.println("                if (window.innerWidth <= 425){");
              client.println("                    document.querySelectorAll('.on').forEach(el => el.innerHTML = 'Вкл')");
              client.println("                    document.querySelectorAll('.off').forEach(el => el.innerHTML = 'Выкл')");
              client.println("                    document.querySelector('.needfix').innerHTML = 'Темп., влаж.'");
              client.println("                    secondv.parentNode.querySelector('.tittle').innerHTML = 'Темп.'");
              client.println("                    fifthv.parentNode.querySelector('.tittle').innerHTML = 'Влажн.'");
              client.println("                    sixthv.parentNode.querySelector('.tittle').innerHTML = 'Толчки'");
              client.println("                }");
              client.println("if (window.innerWidth == 768) {document.querySelector('.needfix').innerHTML = 'Температур. и влажность'}");
              client.println("if (window.innerWidth <= 768) {document.querySelectorAll('.on').forEach(el => el.innerHTML = 'Вкл');");
              client.println("document.querySelectorAll('.off').forEach(el => el.innerHTML = 'Выкл');}");
              client.println("            })");
              client.println("        </script>");
              client.println("    </body>");
              client.println("</html>");
              delay(0);
              // The HTTP response ends with another blank line:
              // break out of the while loop:
            }
            Serial.print(HTTP_req);
            HTTP_req = "";
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
      }
    }
    // close the connection:
    delay(1);
    client.stop();
    Serial.println("Client Disconnected.");
  }
  return;
}

void loop() {
  web_site_main();
}