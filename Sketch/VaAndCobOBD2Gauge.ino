/*=========  Va&Cob OBD2 Gauge ==========
Programmed:  by Ratthanin W.
Hardware:
- Hardware 2.8" TFT ESP32 LVGL https://www.youtube.com/watch?v=d2OXlVcRYrU
Configuration:
- Partition scheme
  option 1 Huge APP 3MB /NO OTA (size is too big)
  option 2 Minimal SPIFFS 1.9MB APP/ 190kB SPIFFS /OTA (for OTA ,must reduce code size)

Important Note:
- New device must flash "gauge_factory_init.ino" first to get it work by checking 'serialno' in pref.
- CPU temp read never work until Bluetooth or Wifi is connected

*/

//--- FLAG SETTING ----- for debugging
//#define TERMINAL //temrinal mode (no gauge)
//#define SERIAL_DEBUG //to show data in serial port
//#define SKIP_CONNECTION //skip elm327 BT connection to view meter
//#define TEST_DTC //test DTC


//Intermal temperature sensor function declaration
#ifdef __cplusplus
extern "C" {
#endif
  uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();  //declare intermal temperature sensor functio

//Macro
#define array_length(x) (sizeof(x) / sizeof(x[0]))  //macro to calculate array length

//library load
#include <Preferences.h>  //save permanent data
Preferences pref;         //create preference
#include <BluetoothSerial.h>
#include <TFT_eSPI.h>  //Hardware-specific library


//Pin configuration
//#define SCA_PIN 21//not used same pin with TFT_BL
//#define SCL_PIN 35//not used (avaialable) CAN_TX
#define ADC_PIN 27  //not used (available)  CAN_RX
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17
#define LDR_PIN 34       //LDR sensor
#define BEEP_PIN 22      //speaker
#define WATER_PIN 35     //speaker
#define BUZZER_PIN 26    //speaker
#define SELECTOR_PIN 35  //push button
// setting PWM properties
#define backlightChannel 0
#define buzzerChannel 2
#include "esp32_compat.h"
//DISPLAY
TFT_eSPI tft = TFT_eSPI();
#define TFT_GREY 0x5AEB


//ELM327 init https://www.elmelectronics.com/wp-content/uploads/2016/07/ELM327DS.pdf
/*const uint8_t elm327InitCount = 13;
const String elm327Init[elm327InitCount] = {
  "ATZ",
  "ATD",
  "ATBRD23",
  "ATSP0",
  "ATAT2",
  "ATL0",
  "ATH0",
  "ATE0",
  "0100",
  "ATSH 7E1",
}; */ //agressive wait response time from ECU
//const String elm327Init[elm327InitCount] = {"ATZ","ATPP0CSV35","ATPP0CON","ATSP0","ATAT2","ATL0","ATH0","ATE0","0100","ATSH07E1"};//agressive wait response time from ECU
const String header[2] = { "ATSH7E0", "ATSH7E1" };

const uint8_t elm327InitCount = 12;
const String elm327Init[elm327InitCount] =  { "ATWS", "ATBRD23", "ATAT2", "ATL0", "ATH0", "ATE0", "ATSP0",};//agressive wait response time from ECU
//const String elm327Init[elm327InitCount] = {"ATZ","ATPP0CSV35","ATPP0CON","ATSP0","ATAT2","ATL0","ATH0","ATE0","0100","ATSH7E0"};//agressive wait response time from ECU


/*------ PIDS total 7 now -------------------
Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit, warn }
label = Pid label show on meter
unit = unit for pid
pid = string pid 0104
formula = formula no to calculate 
min = lowest value
max = highest value
skip = delay reading 0 - 3; 3 = max delay read
digit = want to show digit or not
warn = default warning value
*/

const String pidConfig[8][9] = {
  //[pid][data]
  //{ Label     , unit,    pid,  fomula, min,    max, ,skip, digit, warn }
  { "CVT Temp"   , "`C" , "2210D2", "5", "60" , "120" , "3", "0", "99" },   // 0 -> CVT Temp (Linha 1 Esquerda)
  { "Coolant"    , "`C" , "0105"  , "1", "60" , "120" , "1", "0", "105" },  // 1 -> Coolant (Linha 1 Direita)
  { "Short Trim" , "%"  , "0106"  , "6", "-30", "30"  , "0", "1", "760" },  // 2 -> Short Trim (Linha 2 Esquerda)
  { "AFR"        , "A/F", "0134"  , "2", "0"  , "2"   , "1", "2", "200" },  // 3 -> AFR (Linha 2 Centro)
  { "Long Trim"  , "%"  , "0107"  , "6", "-30", "30"  , "0", "1", "760" },  // 4 -> Long Trim (Linha 2 Direita)
  { "Water Level", "IO" , "013C"  , "8", "0"  , "101" , "2", "0", "60" },   // 5 -> Water Level (Linha 3 Esquerda)
  { "Oil Press"  , "bar", "0105"  , "7", "0"  , "10"  , "0", "1", "200" },  // 6 -> Oil Press (Linha 3 Direita)
  { "ENG SPD"    , "rpm", "010C"  , "3", "0"  , "5000", "3", "0", "9000" }  // 7 -> Engine monitor
};

//hold warning value
String warningValue[8] = { "99", "105", "760", "200", "760", "60", "200", "9000" };

/*  User configuration here to change display 
    Linha 1: CVT Temp (esq), Coolant (dir)
    Linha 2: Short Trim (esq), AFR (centro), Long Trim (dir)
    Linha 3: Water Level (esq), Oil Press (dir)
*/
const uint8_t pidInCell[4][8] = {
  { 0, 1, 2, 3, 4, 5, 6, 7 },  //layout 0 (unused)
  { 0, 1, 2, 3, 4, 5, 6, 7 },  //layout 1 (unused)
  { 0, 1, 2, 3, 4, 5, 6, 7 },  //layout 2 -> 7 medidores numericos (3 na linha 2)
  { 0, 1, 2, 3, 4, 5, 6, 7 },  //layout 3 -> 7 medidores de arco (3 na linha 2)
};
// User configuration here to change display  >

/*---------------------------*/
//if NO DATA value will set to 0 to skip reading
uint8_t layout = 2;                                   //pidInCelltype 0-4 EEPROM 0x00
const uint8_t max_layout = array_length(pidInCell);   //total 5 type of display page
uint8_t pidIndex = 0;                                 //point to PID List
const uint8_t maxpidIndex = array_length(pidConfig);  //total 7 pids in picConfig
bool prompt = false;                                  //bt elm ready flag
bool skip = false;                                    //pid skip flag
uint8_t A = 0;                                        //get A
uint8_t B = 0;                                        //get B


//Global variable
const uint32_t SLEEP_DURATION = 180 * 1000000;                         //180 sec* us = 3 min
String line[12] = { "", "", "", "", "", "", "", "", "", "", "", "" };  //buffer for each line in terminal function
bool dim = false;                                                      //back light dim flag
const uint8_t LIGHT_LEVEL = 40;                                        //backlight control by light level Higher = darker light
long runtime = 0;                                                      //to check FPS
long holdtime = 0;                                                     //hold time for button pressed check
const String compile_date = __DATE__ " - " __TIME__;                   //get built date and time
bool press = false;                                                    // button press flag
bool showsystem = false;                                               //show fps and temp EEPROM 0x01
uint8_t pidRead = 0;                                                   //counter that hold number of pids been read to check pid/sec
String serial_no = "";                                                 //keep serial no.

//CPU Temp
const uint8_t tempOverheat = 60;       //max operating cpu temp 60c
const int8_t factoryTempOffset = -50;  //default offset adjustment temp up to each tested esp32 max at -50
float tempRead = 0.0;                  //current reading cpu temp
int8_t tempOffset = 0;                 //offset adjustment temp up to each tested esp32
uint8_t temp_read_delay = 0;           //hold delay counter to read temp
uint8_t temp_overheat_count = 0;       //counter to read continuiosly overheat

//BLUETOOTH
String bt_message = "";                                                     //bluetooth message buffer
String se_message = "";                                                     //serial port message buffer
String deviceName[8] = { "", "", "", "", "", "", "", "" };                  //discoveryBT name
String deviceAddr[8] = { "", "", "", "", "", "", "", "" };                  //discover BT addr
uint8_t btDeviceCount = 0;                                                  //discovered bluetooth devices counter
#define BT_DISCOVER_TIME 5000                                               //bluetooth discoery time
esp_bd_addr_t client_addr = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };         //obdII mac addr
esp_bd_addr_t recent_client_addr = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  //keep last btaddr in RTC memory
const String client_name = "OBDII";                                         //adaptor name to search
esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;                                  // or ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE to request pincode confirmation
esp_spp_role_t role = ESP_SPP_ROLE_SLAVE;                                   // or ESP_SPP_ROLE_MASTER
bool foundOBD2 = true;
BluetoothSerial BTSerial;  //bluetooth serial device

//---- Include Header File ---
#include "image.h"
#include "touchscreen.h"

/*----------- global function ---------*/

//------------------------------------------------------------------------------------------
void checkCPUTemp() {  //temperature can read only when BT or Wifi Connected
/*temp_read_delay++; 
  if ((temp_read_delay >= 50) && (foundOBD2)) {//delay loop 50 then read temp, avoid error eading
    temp_read_delay = 0;
    uint8_t temperature = temprature_sens_read();//read internal temp sensor
    tempRead = (temperature -32)/1.8 + tempOffset;//change unit F to C
    #ifdef SERIAL_DEBUG
    Serial.print(tempRead,1);   //print Celsius temperature and tab
    Serial.println("°c");
    #endif 
    //read 10 times to confirm it's really overtemp.
    if (tempRead >= tempOverheat) temp_overheat_count++;//count overheat time
    else temp_overheat_count = 0;//reset
    if (temp_overheat_count > 10) {//overheat read more than 10 times.
      Serial.printf("CPU Overheat Shutdown at %d°C\n",tempOverheat);
      tft.fillRectVGradient(0, 0, 320, 240, TFT_RED, TFT_BLACK);
      tft.pushImage(127,5,64,64,overheat,TFT_RED);//show sdcard icon
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("CPU Overheat Shutdown!",159,75,4);
      tft.setTextColor(TFT_WHITE,TFT_BLUE);
      tft.drawCentreString("Auto-restart within 3 minutes",159,140,4);
      tft.drawCentreString("OR",159,170,4);
      tft.drawCentreString("Press button to restart",159,200,4);
      ledcWriteTone(buzzerChannel, 1500);
      delay(5000);
      ledcWriteTone(buzzerChannel, 0);
      BTSerial.disconnect();//disconnect bluetooth
      tft.fillScreen(TFT_BLACK);
      tft.writecommand(0x10); //TFT sleep
      //enter deep sleep
      esp_sleep_enable_timer_wakeup(SLEEP_DURATION);//sleep timer 3 min
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_27,LOW); //wake when button pressed
      esp_deep_sleep_start();  //sleep shutdown backlight auto off with esp32
    }//if temp_overheat_coun
  }//if temp_read_delay
*/}//check CPU Temp
/*------------------*/

//TERMINAL terminal for debugging display text
void Terminal(String texts, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint8_t max_line = round((h - y) / 16.0);
  tft.fillRect(x, y, x + w, y + h, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  for (uint8_t i = 0; i < max_line; i++) {
    if (i <= max_line - 2) {
      line[i] = line[i + 1];
    } else {
      line[i] = texts;
    }
    tft.drawString(line[i], x, i * 20 + y, 2);
  }                 //for
  bt_message = "";  //reset bt message
}
/*------------------*/
/*
void checkGenuine() {//check if genuine obd2 gauge - must flash "gauge_factory_init.ino" first
//security {serialno: "V&C-OBDII-001"}
  pref.begin("security", true);//read only
  serial_no = pref.getString("serialno","");//if no serial no, then empty
  Serial.print(F("Serial No: "));
  if (serial_no == "") {
    tft.setTextColor(TFT_WHITE,TFT_RED);
    tft.drawString("Not genuine Va&Cob Gauge!",0,115,4);
    Serial.println(F("Not genuine Va&Cob Gauge!"));
    while(true) {//beep
      ledcWriteTone(buzzerChannel,2000);
      digitalWrite(LED_RED_PIN, LOW);//red on
      delay(500);
      ledcWriteTone(buzzerChannel,0);
      digitalWrite(LED_RED_PIN, HIGH);//red off
      delay(500);
    }
  } else {
    Serial.println(serial_no);//print serial no
    pref.end();
  }
}
//----------------------------------------------
*/
//---- Include Header File --------------------
//#include "image.h"
#include "bluetooth.h"
#include "meter.h"
#include "config.h"

//---- Inicializacao do ELM327 -----------------
void initELM327() {
  Terminal("Initializing ELM327...", 0, 48, 320, 191);
  bt_message = "";
  for (uint8_t initIndex = 0; initIndex < elm327InitCount; initIndex++) {  //send init ELM327 AT command
    BTSerial.print(elm327Init[initIndex] + "\r");
    if (initIndex == elm327InitCount - 1) delay(2000);  //wait for protocol search
    else delay(100);
    while (BTSerial.available() > 0) {
      char incomingChar = BTSerial.read();
      if (incomingChar == '>') {  //check prompt
        BTSerial.flush();         //clear buffer
#ifdef SERIAL_DEBUG
        Serial.println(bt_message);
#endif
#ifdef TERMINAL
        Terminal(bt_message, 0, 48, 320, 191);
#endif
        if (initIndex == 1) BTSerial.print("\r");  //ATBRD23 response
        bt_message = "";
      } else {
        bt_message.concat(incomingChar);
      }
    }  //while
  }    //for
}

//---- Rotina de Monitoramento e Reconexao Bluetooth ----
void checkConnection() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 1000) return;  //verifica apenas 1 vez por segundo
  lastCheck = millis();

  // Testa EXCLUSIVAMENTE se a conexao Bluetooth esta ativa
  if (!BTSerial.connected()) {
    Serial.println(F("\n[BT] Conexao Bluetooth perdida!"));

    // Alerta visual no topo da tela
    tft.fillRect(0, 0, 320, 26, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawCentreString("BT DESCONECTADO - Reconectando...", 160, 4, 2);

    BTSerial.disconnect();
    delay(300);

    // Tenta reconexao rapida ao ultimo dispositivo conhecido
    connectLastOBDII();

    if (foundOBD2 && BTSerial.connected()) {
      Serial.println(F("[BT] Reconectado com sucesso!"));
      initELM327();
      initScreen();
      // Reseta variaveis e contadores
      for (uint8_t i = 0; i < maxpidIndex; i++) old_data[i] = 0.0;
      engine_off_count = 0;
      BTSerial.flush();
      while (BTSerial.available() > 0) BTSerial.read();
      bt_message = "";
      skip = false;
      pidIndex = 0;
      prompt = true;
    } else {
      // Se a reconexao imediata falhar, reiniciar o ESP32 e a opcao mais leve e confiavel
      Serial.println(F("[BT] Falha ao reconectar. Reiniciando ESP32..."));
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED);
      tft.drawCentreString("Bluetooth Perdido!", 160, 90, 4);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("Reiniciando ESP32...", 160, 135, 2);
      delay(1000);
      ESP.restart();
    }
  }
}

//----SET UP -----------------------------------
void setup() {
  //pin configuration
  analogSetAttenuation(ADC_11db);       // 0dB(1.0 ครั้ง) 0~800mV   for LDR
  pinMode(LDR_PIN, ANALOG);             //ldr analog input read brightness
  pinMode(BUZZER_PIN, OUTPUT);          //speaker
  pinMode(BEEP_PIN, OUTPUT);            //BEEPER
  pinMode(SELECTOR_PIN, INPUT_PULLUP);  //button
  pinMode(WATER_PIN, INPUT_PULLUP);     //button
  pinMode(LED_RED_PIN, OUTPUT);         //red
  pinMode(LED_GREEN_PIN, OUTPUT);       //green
  pinMode(LED_BLUE_PIN, OUTPUT);        //blue
  digitalWrite(LED_RED_PIN, LOW);       //on
  digitalWrite(LED_GREEN_PIN, HIGH);    //off
  digitalWrite(LED_BLUE_PIN, HIGH);     //off
  digitalWrite(BEEP_PIN, LOW);          //off
  //pwm setup
  ledcSetup(buzzerChannel, 1500, 10);        //buzzer 10 bit
  ledcSetup(backlightChannel, 12000, 8);     //backlight 8 bit
  ledcAttachPin(BUZZER_PIN, buzzerChannel);  //attach buzzer

  //init communication
  Serial.begin(115200);
  //memory check
  psramInit();
  Serial.println(F("\n---------------------------------------\n"));
  Serial.printf("Free Internal Heap %d/%d\n", ESP.getFreeHeap(), ESP.getHeapSize());
  Serial.printf("Free SPIRam Heap %d/%d\n", ESP.getFreePsram(), ESP.getPsramSize());
  Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.println(F("\n---------------------------------------\n"));
  Serial.println(F("<<  Va&Cob OBDII Gauge  >>"));
  Serial.print(F("by Ratthanin W. BUILD -> "));
  Serial.println(compile_date);

  //init TFT display
  tft.init();
  tft.setRotation(1);  //landcape

  //Initialize the VSPI bus for touchscreen
  vspi = new SPIClass(VSPI);
  vspi->begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(*vspi);  // Initialize the touch screen with the custom SPI instance
  ts.setRotation(1);
  //touch_calibrate();//hold button at start to calibrate touch
  //testTouch();//test touchscreen

  tft.setSwapBytes(true);                   //to display correct image color
  tft.pushImage(0, 0, 320, 240, vaandcob);  //show logo
  delay(3000);
  //backlight ledcAttachPin must be set after tft.init()
  ledcAttachPin(TFT_BL, backlightChannel);  //attach backlight
  for (uint8_t i = 255; i > 0; i--) {       //fading effect
    ledcWrite(backlightChannel, i);         //full bright
    delay(5);
  }
  digitalWrite(LED_RED_PIN, HIGH);  //red off
  tft.fillScreen(TFT_BLACK);        //show start page
  tft.fillRectVGradient(0, 0, 320, 26, TFT_YELLOW, 0x8400);
  tft.setTextColor(TFT_BLACK);
  tft.pushImage(0, 0, 320, 25, obd2gauge);  //show logo
  ledcWrite(backlightChannel, 255);         //full bright
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawRightString("   Config button ->", 319, 26, 2);
  String txt = " BUILD : " + compile_date;
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString(txt, 0, 26, 2);

  //checkGenuine();//check guniune
  //init variable
  pref.begin("setting", false);
  /* Create a namespace called "setting" with read/write mode
{ pref.get...(key,default)
layout : UShort 
showsystem: bool
tempOffset : uint16_t
warning value: uint16_t
recent_client_addr : {0x00,0x00,0x00,0x00,0x00,0x00} array of bytes[6]
}*/
  layout = pref.getUShort("layout", 2);  //load layout setting from NVR (default 2)
  if (layout != 2 && layout != 3) layout = 2;
  showsystem = pref.getBool("showsystem", false);             //load showsytem
  tempOffset = pref.getInt("tempOffset", factoryTempOffset);  //load defaultTempoffset
#ifdef SERIAL_DEBUG
  Serial.printf("Show System: %d\n", showsystem);
  Serial.printf("Temp Offset: %d\n", tempOffset);
#endif
  for (uint8_t i = 0; i < maxpidIndex; i++) {  //read warning value from pid name if not found load from default
    warningValue[i] = pref.getString(pidConfig[i][0].c_str(), pidConfig[i][8]);
  }
  pref.getBytes("recent_client", recent_client_addr, pref.getBytesLength("recent_client"));  //read last bt address
  ecu_off_volt = pref.getFloat("ecu_off_volt", factoryECUOff);                               //read ecu 0ff voltage


  //start bluetooth
  if (!BTSerial.begin("Va&Cob OBDII Gauge", true)) {
    Serial.println(F("Bluetooth..error!"));
    Terminal("Bluetooth..error!", 0, 48, 320, 191);
    abort();
  } else {
    Serial.println(F("Bluetooth..OK"));
    Terminal("Bluetooth..OK", 0, 48, 320, 191);
  }
  runtime = millis();
#ifdef SKIP_CONNECTION
  foundOBD2 = true;
#endif

  //Connect to ELM327

  connectLastOBDII();   //try connect last BT
  while (!foundOBD2) {  //not success try scan and connect another OBD2
    scanBTdevice();
    autoDim();  //auto backlight handle
    //checkCPUTemp(); //check temp never work if BT or WIFI not connected
    if (digitalRead(SELECTOR_PIN) == LOW) {  //button pressed
      if (!press) {
        press = true;                          //set flag
        holdtime = millis();                   //set timer
      } else if (holdtime - millis() > 500) {  //press once and longer
        configMenu();                          //open config menu
        press = false;                         //reset flag
      }                                        //else if holdtimer > 30000
      delay(200);                              //delay avoid bounce
    }
  }

  //Initialize ELM327
  initELM327();
  initScreen();
  beepbeep();
  prompt = true;
  //----------------------
}  //setup

/*###################################*/
void loop() {
  // 1. Verifica se o Bluetooth esta ativo
  checkConnection();

  // 2. Touch screen: alternar entre tela 2 (medidores numericos) e tela 3 (medidores de arco)
  static bool wasTouched = false;
  static unsigned long lastTouchTime = 0;

  if (ts.touched()) {
    if (!wasTouched && (millis() - lastTouchTime > 400)) {
      wasTouched = true;
      lastTouchTime = millis();

      // Alterna entre tela 2 e tela 3
      if (layout == 2) {
        layout = 3;
      } else {
        layout = 2;
      }
      pref.putUShort("layout", layout);

      ledcWriteTone(buzzerChannel, 5000);  // som de clique
      delay(10);
      ledcWriteTone(buzzerChannel, 0);

      initScreen();  // recarrega a tela selecionada

      // reset variaveis de leitura
      for (uint8_t i = 0; i < maxpidIndex; i++) old_data[i] = 0.0;
      engine_off_count = 0;
      BTSerial.flush();
      while (BTSerial.available() > 0) {
        BTSerial.read();
      }
      bt_message = "";
      skip = false;
      pidIndex = maxpidIndex - 1;
      prompt = true;
    }
  } else {
    wasTouched = false;
  }

  //----------------------
  //BLUETOOTH read
  while (BTSerial.available() > 0) {
    char incomingChar = BTSerial.read();
    if (incomingChar == '>') {  //check prompt
      BTSerial.flush();         //clear tx
#ifdef SERIAL_DEBUG
      Serial.println(bt_message);
      //Serial.println("testeloop)");
#endif
#ifdef TERMINAL  //for degbugging
      getAB(bt_message);
      Terminal(bt_message + "->" + String(A) + "," + String(B), 0, 48, 320, 191);
#endif
      prompt = true;  //pid response ready to calculate & display
    } else {
      bt_message.concat(incomingChar);  //keep elm327 respond
    }
  }
  //----------------------
  //SERIAL PORT read
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();
    //check CR/NL
    if ((incomingChar != '\r') && (incomingChar != '\n')) {
      se_message.concat(incomingChar);
      //Serial.print(incomingChar,HEX);
    } else {

      if (se_message != "") {
#ifdef TERMINAL
        Terminal(se_message, 0, 48, 320, 191);  //display terminal
#endif
        BTSerial.print(se_message + "\r");
        se_message = "";
        Serial.flush();
      }  //if se_message

    }  //else if incomingChar
  }    //if Serial.available

  //Send another PID to elm327
  if (prompt) {      //> ELM327 ready! -> request next PID
    prompt = false;  //no prompt from ELM327
    checkCPUTemp();  //check temperature.
    if (!skip) {
      updateMeter(pidIndex, bt_message);  //update meter screen
      pidRead++;                          //for calculate pid/s
    }
    pidIndex++;  //point to next pid
    if (pidIndex >= maxpidIndex) {
      pidIndex = 0;      //back to pid 0
      autoDim();         //auto backlight handle
      if (showsystem) {  //aqui mostra alguma informação sem utilidade
        String status = String(pidRead * 1000.0 / (millis() - runtime), 0) + " p/s " + String(tempRead, 1) + "`c";
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString(status.c_str(), 159, 0, 2);
        runtime = millis();
      }             //if showsystem
      pidRead = 0;  //reset pid read counter
    }
    if (pidCurrentSkip[pidIndex] <= 0) {                 //end of skip loop , go next index
      pidCurrentSkip[pidIndex] = pidReadSkip[pidIndex];  //read max delay loop to current
      skip = false;

      if(pidList[pidIndex]=="2210D2"){    //troca os headers de acordo com pid 2210D2
        BTSerial.print(header[1] + "\r");
        delay(100);
        BTSerial.print(pidList[pidIndex] + "\r");              //send PID request
        delay(100);
        BTSerial.print(header[0] + "\r");
        delay(100);
      }else{
        BTSerial.print(pidList[pidIndex] + "\r");                 //send PID request
        //delay(100);
      }
      
      //BTSerial.print(pidList[pidIndex] + "\r");                 //send PID request
      
    } else {                                                    //skip sending request
      pidCurrentSkip[pidIndex] = pidCurrentSkip[pidIndex] - 1;  //decrese loop count
      prompt = true;
      skip = true;
    }  //else

  }  //if prompt

  /*------------------*/
}  // loop
