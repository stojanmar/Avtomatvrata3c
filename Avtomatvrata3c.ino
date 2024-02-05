/*********
  Web server za avtomatska vrata  Stojan Markic
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-web-server-timer-pulse/
 ver01 osnovni test timer pulsa na pin 2 ledica sveti invertirano na Nodemcu ver1.0
 ver3a ima spremenjen slider do 5 max
 ver3b dodam reconect v primeru lost connection , moti s piskanjem vrat
 ver3c reconnect brez reboota
*********/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "XXXXXX"
#define STAPSK  "xxxxxx"
#endif

// Replace with your network credentials

const char* ssid = STASSID;
const char* password = STAPSK;

const char* PARAM_INPUT_1 = "state";
const char* PARAM_INPUT_2 = "value";
const char* PARAM_INPUT_3 = "input3";
const char* PARAM_INPUT_4 = "input4";
const char* PARAM_INPUT_5 = "input5";
const char* PARAM_INPUT_6 = "input6";

const int stopout = 12;   //D6 je LOW na zacetku in high po casu
const int openout = 13;   //D7
int statemachine2 = 0;  //0-release everithing-zapre v avto 1-open 2-delni open 3-delno zapri
int direction2 = 0;
int changed = 0;
int pavza = 100;  //milisekund
float pulselength2 = 0;
unsigned long previousMillis = 0;
unsigned long interval = 60000;  // 1 minuta

String timerSliderValue = "0.0";

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Avtomatska vrata</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.2rem;}
    h1 {font-size: 1.5rem;}
    p {font-size: 1.0rem;}
    body {background-color: #eda88e; max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .slider2 { -webkit-appearance: none; margin: 14px; width: 300px; height: 20px; background: #ccc;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider2::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 30px; height: 30px; background: #2f4468; cursor: pointer;}
    .slider2::-moz-range-thumb { width: 30px; height: 30px; background: #2f4468; cursor: pointer; } 
 input[type=submit] {
  width: 80px;
  height: 33px;
  background-color: #128F76;
  color: white;
  text-align: center;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  cursor: pointer;
  border-radius: 5px;
}
input[type="text"] {
   margin: 0;
   height: 28px;
   background: white;
   font-size: 16px;
   width: 60px;
   border-radius: 5px;
   -webkit-border-radius: 5px;
   -moz-border-radius: 5px;
   -webkit-appearance: none;
   border: 1px solid black;
   border-radius: 10px;
}
  </style>
</head>
<body>
  <h2>Avtomatska vrata</h2>
  <h1><span id="timerValue">%TIMERVALUE%</span> s</h1>
  <p><input type="range" onchange="updateSliderTimer(this)" id="timerSlider" min="0.0" max="5" value="%TIMERVALUE%" data-format="#.0" step="0.1" class="slider2"></p>
  %BUTTONPLACEHOLDER%
<h1>** Kontrola pinov **</h1>
  <form action="/get">
    STOP pin <input type="text" name="input3" value="1">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    STOP pin <input type="text" name="input4" value="0">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    OPEN pin <input type="text" name="input5" value="1">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    OPEN pin <input type="text" name="input6" value="0">
    <input type="submit" value="Submit">
  </form>
   <p>Stran kreirana Okt 2023 : Stojan Markic </p>  
<script>
function toggleCheckbox(element) {
  var sliderValue = document.getElementById("timerSlider").value;
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); xhr.send();
    var count = sliderValue, timer = setInterval(function() {
      count -= 0.1; document.getElementById("timerValue").innerHTML = parseFloat(count).toFixed(1);
      if(count <= 0){ clearInterval(timer); document.getElementById("timerValue").innerHTML = document.getElementById("timerSlider").value; }
    }, 100);
    sliderValue = sliderValue*1000;
    setTimeout(function(){ xhr.open("GET", "/update?state=0", true); 
    document.getElementById(element.id).checked = false; xhr.send(); }, sliderValue);
  }
}
function updateSliderTimer(element) {
  var sliderValue = document.getElementById("timerSlider").value;
  document.getElementById("timerValue").innerHTML = sliderValue;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    String outputStateValue = outputState();
    buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  else if(var == "TIMERVALUE"){
    return timerSliderValue;
  }
  return String();
}

String outputState(){
  if(digitalRead(stopout)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(stopout, OUTPUT);
  digitalWrite(stopout, LOW);
  pinMode(openout, OUTPUT);
  digitalWrite(openout, LOW);

  // Set your Static IP address
  IPAddress local_IP(192, 168, 1, 184);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 1, 1);

  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   //optional
  IPAddress secondaryDNS(8, 8, 4, 4); //optional
   // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("StaticIP Failed to configure");
      }
  wifiMulti.addAP("XXXXXXX", password);   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("YYYYYYY", password);
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");
  Serial.println("Connecting");
  //int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
  }
  /*Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer*/

    if (MDNS.begin("esp8266")) {
    Serial.println("MDNS Responder Started");
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      //digitalWrite(stopout, inputMessage.toInt());
      if(inputMessage.toInt() || (statemachine2 == 0)) {changed = 1;}  //zato ker update naredi tudi ko se button zapre
      //changed = 1;
    }
    else {
      inputMessage = "No change sent";
    }
    Serial.println(changed);
    request->send(200, "text/plain", "OK");
  });
  
  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      timerSliderValue = inputMessage;
      float sliderpos = inputMessage.toFloat();
      pavza = (int) (sliderpos * 1000);
      statemachine2 = 2;  //delno odpiranje
      if (pavza == 0) {statemachine2 = 0;}   //zapri
      if (pavza >= 4800) {statemachine2 = 1;}  //odpri
    }
    else {
      inputMessage = "No slider sent";
    }
    //Serial.println(inputMessage);
    Serial.println(pavza);
    request->send(200, "text/plain", "OK");
  });
  
 // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;  
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      if (inputMessage == "1") {
      digitalWrite(stopout, HIGH);}
    }
    // GET input4 value on <ESP_IP>/get?input4=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = PARAM_INPUT_4;
      if (inputMessage == "0") {
      digitalWrite(stopout, LOW);}
    }
    // GET input5 value on <ESP_IP>/get?input5=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_5)->value();
      inputParam = PARAM_INPUT_5;
      if (inputMessage == "1") {
      digitalWrite(openout, HIGH);} 
    }
    // GET input6 value on <ESP_IP>/get?input6=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_6)) {
      inputMessage = request->getParam(PARAM_INPUT_6)->value();
      inputParam = PARAM_INPUT_6;
      if (inputMessage == "0") {
      digitalWrite(openout, LOW); }
    }
    
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    
    /*request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");*/
      //request->send_P(200, "text/html", index_html); 
      //request->send(200, "text/plain", "OK");
      request->send_P(200, "text/html", index_html, processor);                                                              
  });

  // Start server
  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP Server Started");
  changed = 0;
   ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());
  changed = 0;
}
  
void loop() {
  ArduinoOTA.handle();

  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
  //Serial.print(millis());
  //Serial.println("Reconnecting to WiFi...");
  WiFi.disconnect();
  //WiFi.begin(YOUR_SSID, YOUR_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(500);
    Serial.print('.');
      }
    if (MDNS.begin("esp8266")) {
    Serial.println("MDNS Responder Started");
      }
    previousMillis = currentMillis;
  //ESP.restart();   //more clear reset actually reboot
      }

   if (changed){
      changed = 0;
          switch(statemachine2)
           {
  case 0:   //release everithing tudi zapiranje če je na auto
    digitalWrite(openout, LOW);
    digitalWrite(stopout, LOW);
    break;
  case 1:   //open funkcija - do konca neomejen cas
    digitalWrite(stopout, LOW);
    delay (10);
    digitalWrite(openout, HIGH);
    //delay (100);
    //digitalWrite(openout, LOW);
    break;
  case 2:   //delno odpiranje
    digitalWrite(stopout, LOW);
    delay (10);
    digitalWrite(openout, HIGH);
    delay (100);
    digitalWrite(openout, LOW);
    //pavza = (int) (pulselength2 * 1000);
    delay (pavza);
    digitalWrite(stopout, HIGH);
    break;
  /*case 3:   //delno zapiranje
    digitalWrite(stopout, LOW);
    pavza = (int) (pulselength2 * 1000);
    delay (pavza);
    digitalWrite(stopout, HIGH);
    break;
  case 4:   //takoj casovno neomejen stop
    digitalWrite(stopout, HIGH);
    break;*/
  default:
    // statements
    break;
           }   
        }
    //this is to avoid millis() overlap or WiFi router prohibition
    /*if (millis() > 777600000UL)   //259200000UL is for 3 days  7776....is for 9 days 
      {
      //this is every 3 days
      ESP.restart();   //more clear reset actually reboot
      }*/

}  //end of loop
