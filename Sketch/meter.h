// All Meters display function 

const uint16_t screenWidth = tft.height();//landscape
const uint16_t screenHeight = tft.width();
/*---- METER ------_*
 Type 0:numericMeter
 Type 1:arcMeters
 Type 2:vBarMeter
coordinate for each cell (numieric meter & arc meter)
1 , 4 , 7
2 , 5 , 8
3 , 6 , 9 */
const uint16_t cell1_x[9] = { 0, 0, 0, 80, 80, 80, 160, 160, 160 };  //cell type 1 x origin cooridate
const uint16_t cell1_y[9] = { 0, 80, 160, 0, 80, 160, 0, 80, 160 };  //cell type 1 y origin coordiate
bool blinkCell[10] = {false};                                         //flag for blinking (cell 1..9)
const uint16_t cellNarrow_x[3] = { 0, 107, 214 }; // cell narrow x origin (Linha 2: 3 medidores)
const uint16_t cellNarrow_y[3] = { 80, 80, 80 };  // cell narrow y origin (Linha 2)
const uint16_t cellNarrow_w = 106;
const uint16_t cellNarrow_h = 80;
bool blinkCellNarrow[3] = {false, false, false};  // flag for narrow blinking

String pidList[maxpidIndex] = {};          //{"0104","0105","010C","010B","010C","0142","015C"}
uint8_t pidReadSkip[maxpidIndex] = {};     //skip reading 0 - 3; 3 = max delay read
uint8_t pidCurrentSkip[maxpidIndex] = {};  //current skip counter
uint8_t engine_off_count = 0;//counter to indicate engine is off then sleep
float old_data[maxpidIndex] = {};//keep old data for each gauge to improve speed for pidIndex 0-6
String old_numeric_str[10] = {}; // cache do texto exibido nos medidores numericos largos (evita flicker)
String old_numeric_narrow_str[3] = {}; // cache do texto exibido nos medidores numericos estreitos

//backlight variable average counter
uint8_t low_count = 0;
uint8_t high_count = 0; 

const float factoryECUOff = 10.5;//default off voltage
float ecu_off_volt = factoryECUOff;//turn engine off if voltage lower than 12.4

/* ####################################
 draw digital numeric meter  160x80 px
####################################
cell 
1 , 2 , 3
4 , 5 , 6
7 , 8 , 9 */
void numericMeter(uint8_t cell, uint8_t pid) {
  int w = screenWidth / 2;
  int h = screenHeight / 3;
  int x = cell1_x[cell - 1];
  int y = cell1_y[cell - 1];
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  String label = pidConfig[pid][0];  //get PID labels to label
  tft.drawRect(x, y, w, h, TFT_GREY);
  tft.fillRect(x + 2, y + 2, w - 4, h - 4, TFT_BLACK);
  tft.fillRectVGradient(x + 2, y + 2, w - 4, h * 0.33, TFT_BLUE, 0x0011);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(label, x + w / 2, y + 2, 4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawRightString("---", x + w - 5, y + 31, 6);
  tft.drawRect(x, y, w, h, TFT_GREY);
  if (cell < 10) old_numeric_str[cell] = "";
}  //numericMeter
/*---------------------------*/
//plot number on digital Meter
void plotNumeric(uint8_t cell, float data, int warn, unsigned int digit) {
  int compare;
  if (cell == 3) {
    compare = (warn == 0) ? 1 : (data / warn);
  } else {
    if (data <= 0.001f) compare = 1;
    else compare = warn / data;
  }
  int w = screenWidth / 2;
  int h = screenHeight / 3;
  int x = cell1_x[cell - 1];
  int y = cell1_y[cell - 1];

  String result = String(data, digit);
  switch (result.length()) {
    case 2: result = "    " + result; break;
    case 3: result = "   " + result; break;
    case 4: result = "  " + result; break;
  }

  if (compare == 0) { // warning
    if (blinkCell[cell]) { // show
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, HIGH);
      digitalWrite(BEEP_PIN, HIGH);
      old_data[pidIndex] = data;
      tft.fillRect(x + 2, y + 28, w - 4, 49, TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawRightString(result.c_str(), x + w - 5, y + 31, 6);
    } else { // hiding make blinking effect
      tft.fillRect(x + 2, y + 28, w - 4, 49, TFT_BLACK);
    }
    blinkCell[cell] = !blinkCell[cell];
    if (cell < 10) old_numeric_str[cell] = ""; // forca redesenhar ao sair do alarme
  } else { // no warning
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(BEEP_PIN, LOW);
    digitalWrite(LED_BLUE_PIN, LOW);
    if (cell >= 10 || result != old_numeric_str[cell]) {
      tft.fillRect(x + 2, y + 28, w - 4, 49, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawRightString(result.c_str(), x + w - 5, y + 31, 6);
      if (cell < 10) old_numeric_str[cell] = result;
      old_data[pidIndex] = data;
    }
  }
}

/* ####################################
 draw digital numeric meter estreito (106x80 px) - Linha 2
####################################
 cell 0: esq, cell 1: centro, cell 2: dir */
void numericMeterNarrow(uint8_t cell, uint8_t pid) {
  int w = cellNarrow_w;
  int h = cellNarrow_h;
  int x = cellNarrow_x[cell];
  int y = cellNarrow_y[cell];
  String label = pidConfig[pid][0];
  String titleLabel = label;
  if (label == "Short Trim") titleLabel = "S. Trim";
  else if (label == "Long Trim") titleLabel = "L. Trim";
  tft.drawRect(x, y, w, h, TFT_GREY);
  tft.fillRect(x + 2, y + 2, w - 4, h - 4, TFT_BLACK);
  tft.fillRectVGradient(x + 2, y + 2, w - 4, 25, TFT_BLUE, 0x0011);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(titleLabel, x + w / 2, y + 2, 4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("---", x + w / 2, y + 30, 6);
  tft.drawRect(x, y, w, h, TFT_GREY);
  if (cell < 3) old_numeric_narrow_str[cell] = "";
}

// plot number on narrow digital Meter
void plotNumericNarrow(uint8_t cell, float data, int warn, unsigned int digit) {
  int compare;
  if (data <= 0.001f) compare = 1;
  else compare = warn / data;
  int w = cellNarrow_w;
  int h = cellNarrow_h;
  int x = cellNarrow_x[cell];
  int y = cellNarrow_y[cell];

  String result;
  if (data <= -10.0 && digit) {
    result = String(data, 0);
  } else {
    result = String(data, digit);
  }

  if (compare == 0) { // warning
    if (blinkCellNarrow[cell]) {
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, HIGH);
      digitalWrite(BEEP_PIN, HIGH);
      old_data[pidIndex] = data;
      tft.fillRect(x + 2, y + 27, w - 4, 49, TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString(result.c_str(), x + w / 2, y + 30, 6);
    } else {
      tft.fillRect(x + 2, y + 27, w - 4, 49, TFT_BLACK);
    }
    blinkCellNarrow[cell] = !blinkCellNarrow[cell];
    if (cell < 3) old_numeric_narrow_str[cell] = ""; // forca redesenhar ao sair do alarme
  } else { // no warning
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(BEEP_PIN, LOW);
    digitalWrite(LED_BLUE_PIN, LOW);
    if (cell >= 3 || result != old_numeric_narrow_str[cell]) {
      tft.fillRect(x + 2, y + 27, w - 4, 49, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawCentreString(result.c_str(), x + w / 2, y + 30, 6);
      if (cell < 3) old_numeric_narrow_str[cell] = result;
      old_data[pidIndex] = data;
    }
  }
}

/* ##########################
draw analog meter 160x80 px
 ##########################
cell 
1 , 2 , 3
4 , 5 , 6
7 , 8 , 9 */
void arcMeter(int cell, byte pid) {
  int w = screenWidth / 2;
  int h = screenHeight / 3;
  int x = cell1_x[cell - 1];
  int y = cell1_y[cell - 1];
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  String label = pidConfig[pid][0];     //get PID to label
  String unit = pidConfig[pid][1];      //get PID to unit
  int min = pidConfig[pid][4].toInt();  //get PID to min
  int max = pidConfig[pid][5].toInt();  //get PID to max
  // Meter outline
  tft.fillRect(x, y, w, h, TFT_GREY);
  tft.fillRect(x + 2, y + 2, 156, 76, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);  // Text colour
  tft.drawCentreString(String(min).c_str(), x + 16, y + 10, 2);
  tft.drawCentreString(String(max).c_str(), x + w - 16, y + 10, 2);
  tft.drawRightString(unit.c_str(), x + 85, y + 60, 2);   // Units at bottom right
  tft.drawCentreString(label.c_str(), x + 120, y + 60, 2);  // Comment out to avoid font 4
  tft.setTextColor(TFT_GREEN);
  tft.drawCentreString("---", x + w / 2, y + 35, 4);  // data
  //draw arc animation
  for (int angle = 135; angle < 225; angle++) {
    tft.drawSmoothArc(x + 80, y + 90, 80, 60, 134, angle, TFT_GREEN, TFT_DARKGREY, false);
   }
  for (int angle = 224; angle > 135; angle--) {
    tft.drawSmoothArc(x + 80, y + 90, 80, 60, angle + 1, 226, TFT_WHITE, TFT_DARKGREY, false);
  }
}
// #########################################################################
void plotArc(uint8_t cell, String label, float data, int warn, int min, int max, unsigned int digit) {
  bool isWarning = false;
  if (cell == 3 || label == "Water Level") {
    if (data < warn)
      isWarning = true;   // abaixo do valor warn -> nivel baixo -> alerta vermelho
    else
      isWarning = false;  // acima ou igual ao valor warn -> nivel ok -> verde
  } else {
    int compare = warn / data; // compare data and warn
    if (compare == 0)
      isWarning = true;   // warning
    else
      isWarning = false;  // no warn
  }

  int arcColor = isWarning ? TFT_RED : TFT_GREEN;

  if (isWarning) {
    if (blinkCell[cell]) {
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, HIGH);
      digitalWrite(BEEP_PIN, HIGH);
    } else {
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(BEEP_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, LOW);
    }
    blinkCell[cell] = !blinkCell[cell];
  } else {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(BEEP_PIN, LOW);
    digitalWrite(LED_BLUE_PIN, LOW);
  }

  if (isWarning || (data != old_data[pidIndex])) {
    int x = cell1_x[cell - 1];
    int y = cell1_y[cell - 1];

    String result = String(data, digit);

    if (data < min) data = min;
    if (data > max) data = max;

    tft.fillRect(x + 56, y + 36, 48, 23, TFT_BLACK);

    if (cell == 3 || label == "Water Level") {
      if (isWarning) {
        // Alarme de nivel de agua: arco fica vermelho em todo o comprimento
        tft.drawSmoothArc(x + 80, y + 90, 80, 60, 134, 225, TFT_RED, TFT_DARKGREY, false);
      } else {
        // Nivel de agua normal: arco verde cheio
        int angle = (data >= 100) ? 225 : map(data, min, max, 135, 225);
        if (angle > 225) angle = 225;
        tft.drawSmoothArc(x + 80, y + 90, 80, 60, 134, angle, TFT_GREEN, TFT_DARKGREY, false);
        if (angle < 225) {
          tft.drawSmoothArc(x + 80, y + 90, 80, 60, angle + 1, 226, TFT_WHITE, TFT_DARKGREY, false);
        }
      }
    } else {
      int angle = map(data, min, max, 135, 225);
      if (angle < 135) angle = 135;
      if (angle > 225) angle = 225;
      tft.drawSmoothArc(x + 80, y + 90, 80, 60, 134, angle, arcColor, TFT_DARKGREY, false);
      if (angle < 225) {
        tft.drawSmoothArc(x + 80, y + 90, 80, 60, angle + 1, 226, TFT_WHITE, TFT_DARKGREY, false);
      }
    }

    tft.setTextColor(arcColor, TFT_BLACK);
    tft.drawCentreString(result.c_str(), x + 80, y + 35, 4);  // data

    old_data[pidIndex] = data;//update old data
  }
}

/* ####################################
 draw arc meter estreito (106x80 px) - Linha 2
####################################
 cell 0: esq, cell 1: centro, cell 2: dir */
void arcMeterNarrow(uint8_t cell, byte pid) {
  int w = cellNarrow_w;
  int h = cellNarrow_h;
  int x = cellNarrow_x[cell];
  int y = cellNarrow_y[cell];
  String label = pidConfig[pid][0];
  String unit = pidConfig[pid][1];
  int min = pidConfig[pid][4].toInt();
  int max = pidConfig[pid][5].toInt();

  tft.fillRect(x, y, w, h, TFT_GREY);
  tft.fillRect(x + 2, y + 2, w - 4, h - 4, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(String(min).c_str(), x + 14, y + 6, 2);
  tft.drawCentreString(String(max).c_str(), x + w - 14, y + 6, 2);
  tft.drawCentreString(label.c_str(), x + w / 2, y + 63, 2);
  tft.setTextColor(TFT_GREEN);
  tft.drawCentreString("---", x + w / 2, y + 39, 4);

  // draw arc sweep animation
  for (int angle = 135; angle < 225; angle++) {
    tft.drawSmoothArc(x + 53, y + 62, 52, 38, 134, angle, TFT_GREEN, TFT_DARKGREY, false);
  }
  for (int angle = 224; angle > 135; angle--) {
    tft.drawSmoothArc(x + 53, y + 62, 52, 38, angle + 1, 226, TFT_WHITE, TFT_DARKGREY, false);
  }
}

// plot arc on narrow arc Meter
void plotArcNarrow(uint8_t cell, String label, float data, int warn, int min, int max, unsigned int digit) {
  int compare = warn / data;
  bool isWarning = (compare == 0);
  int arcColor = isWarning ? TFT_RED : TFT_GREEN;

  if (isWarning) {
    if (blinkCellNarrow[cell]) {
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, HIGH);
      digitalWrite(BEEP_PIN, HIGH);
    } else {
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(BEEP_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, LOW);
    }
    blinkCellNarrow[cell] = !blinkCellNarrow[cell];
  } else {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(BEEP_PIN, LOW);
    digitalWrite(LED_BLUE_PIN, LOW);
  }

  if (isWarning || (data != old_data[pidIndex])) {
    int w = cellNarrow_w;
    int x = cellNarrow_x[cell];
    int y = cellNarrow_y[cell];

    String result = String(data, digit);

    if (data < min) data = min;
    if (data > max) data = max;

    tft.fillRect(x + 20, y + 38, 66, 24, TFT_BLACK);

    int angle = 135 + (int)(((data - min) * 90.0f) / (max - min) + 0.5f);
    if (angle < 135) angle = 135;
    if (angle > 225) angle = 225;

    tft.drawSmoothArc(x + 53, y + 62, 52, 38, 134, angle, arcColor, TFT_DARKGREY, false);
    if (angle < 225) {
      tft.drawSmoothArc(x + 53, y + 62, 52, 38, angle + 1, 226, TFT_WHITE, TFT_DARKGREY, false);
    }

    tft.setTextColor(arcColor, TFT_BLACK);
    tft.drawCentreString(result.c_str(), x + 53, y + 39, 4);

    old_data[pidIndex] = data;
  }
}

/*-------------------*/
//initialize dislay by layout choose (2: numeric, 3: arc)
void initScreen() {
  #ifdef SERIAL_DEBUG    
  Serial.printf("Display layout -> %d\n",layout);
  #endif
  for (int i = 0; i < maxpidIndex; i++) {
    pidList[i] = pidConfig[pidInCell[layout][i]][2];                 //get PID to list
    pidReadSkip[i] = pidConfig[pidInCell[layout][i]][6].toInt();     //get read skip
    pidCurrentSkip[i] = pidReadSkip[i];  //skip for skip reading pid
    old_data[i] = 0;//clear old data
  }
  for (int j = 0; j < 10; j++) old_numeric_str[j] = "";
  for (int j = 0; j < 3; j++) old_numeric_narrow_str[j] = "";
  tft.fillScreen(TFT_BLACK);  //clear screen
  switch (layout) {
    case 2:
      {  // 7 numeric meters (Linha 1: 2, Linha 2: 3 estreitos, Linha 3: 2)
        numericMeter(1, pidInCell[layout][0]);          // Linha 1 Esq: CVT Temp
        numericMeter(7, pidInCell[layout][1]);          // Linha 1 Dir: Coolant
        numericMeterNarrow(0, pidInCell[layout][2]);    // Linha 2 Esq: Short Trim
        numericMeterNarrow(1, pidInCell[layout][3]);    // Linha 2 Centro: AFR
        numericMeterNarrow(2, pidInCell[layout][4]);    // Linha 2 Dir: Long Trim
        numericMeter(3, pidInCell[layout][5]);          // Linha 3 Esq: Water Level
        numericMeter(9, pidInCell[layout][6]);          // Linha 3 Dir: Oil Press
      }
      break;      
    case 3:
      {  // 7 arc meters (Linha 1: 2, Linha 2: 3 estreitos, Linha 3: 2)
        arcMeter(1, pidInCell[layout][0]);              // Linha 1 Esq: CVT Temp
        arcMeter(7, pidInCell[layout][1]);              // Linha 1 Dir: Coolant
        arcMeterNarrow(0, pidInCell[layout][2]);        // Linha 2 Esq: Short Trim
        arcMeterNarrow(1, pidInCell[layout][3]);        // Linha 2 Centro: AFR
        arcMeterNarrow(2, pidInCell[layout][4]);        // Linha 2 Dir: Long Trim
        arcMeter(3, pidInCell[layout][5]);              // Linha 3 Esq: Water Level
        arcMeter(9, pidInCell[layout][6]);              // Linha 3 Dir: Oil Press
      }
      break; 
  }  //switch page
}
/*-------------------------------*/

//get A B from response return in global A B variable
void getAB(String elm_rsp) {  //41 05 2A 3C 01>
  if (elm_rsp == "NO DATA\r\r>") {//pid no data
    A = 0;
    B = 0;

  } else {
    //check if pid is 2 or 3 command length.
    uint8_t charcnt = pidList[pidIndex].length() / 2;  //0105->2,221E1C->3
    String strs[8];
    uint8_t StringCount = 0;
    while (elm_rsp.length() > 0) {//keep reading each char
      int index = elm_rsp.indexOf(' ');//check space
      if (index == -1)  // No space found
      { //only data without space is now in strs {41,05,aa,bb}
        strs[StringCount++] = elm_rsp; //last byte
          A = strtol(strs[charcnt].c_str(),NULL,16);      //byte 3 save to A
          B = strtol(strs[charcnt + 1].c_str(),NULL,16);  //byte 4 save to B
        break;
      } else {//found space
        strs[StringCount++] = elm_rsp.substring(0, index);//copy strings from 0 to space to strs
        elm_rsp = elm_rsp.substring(index + 1);//copy the rest behind space to elm_rsp
      }
    }  //while
  }    //else
}//getAB
/*-------------------------------*/
//check engine status if off then turn off gauge
void engine_onoff(float data, uint8_t pid) {  //use pcm volt
//check if pid is 010C eng speed and value = 0
//String t = pidList[pid] + " - " + (String)value;
if (pidList[pid] == "0142") { //pis = 010C engine speed RPM

  int compare = ecu_off_volt/data;
  if (compare > 0)  //check lower voltage
    engine_off_count++;
  else
    engine_off_count = 0;  //reset
  if (engine_off_count > 20) {  //engine is stop
    if(pref.getUShort("layout",false) != layout) {//if current layout not NVR load layout
      pref.putUShort("layout", layout);//save current layout to NVR
    }      
    show_spiffs_jpeg_image("/mypic.jpg", 0, 0);//show mypic
    ledcWriteTone(buzzerChannel,3136);
    delay(100);
    ledcWriteTone(buzzerChannel,2637);
    delay(100);
    ledcWriteTone(buzzerChannel,2093);
    delay(100);
    ledcWriteTone(buzzerChannel,0);
    for (int i=255;i>0;i--) {//fading effect
      ledcWrite(backlightChannel, i);//full bright
      delay(10);
    }//turn off backlight
    BTSerial.print("ATLP\r");//set ELM327 to Low Power
    BTSerial.disconnect();//disconnect bluetooth
    tft.fillScreen(TFT_BLACK);
    tft.writecommand(0x10); //TFT sleep
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27,LOW); //wake when button pressed
    esp_deep_sleep_start();//sleep shutdown backlight auto off with esp32

    }//if engine_off_count>20
  }//if PID = "010C"
}
/*-------------------------------*/
//coolant volt oiltemp vaporpressure TFT Load
// {"01051","01421","015C1","01321","221E1C1","01041"};
void updateMeter(uint8_t pidNo, String response) {  //update parameter on screen
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  String label = pidConfig[pidInCell[layout][pidNo]][0];
  //String unit = pidConfig[pidInCell[layout][pidNo]][1];
  uint8_t formula = pidConfig[pidInCell[layout][pidNo]][3].toInt();
  int min = pidConfig[pidInCell[layout][pidNo]][4].toInt();
  int max = pidConfig[pidInCell[layout][pidNo]][5].toInt();
  unsigned int digit = pidConfig[pidInCell[layout][pidNo]][7].toInt();
  int warn = warningValue[pidInCell[layout][pidNo]].toInt();

  getAB(response);  //get AB
  float data = 0.0;
  
  switch (formula) {                  //choose fomula
    case 0: data = A * 0.145; break;  //psi
    case 1: data = A - 40; break;     //temp
    case 2: data = (A * 256 + B) / 32768.0; break;//(A*256+B)/32768*14.7; break;
    //case 2: data = (A * 256 + B) / 32768.0 * 14.7; break;//(A*256+B)/32768*14.7; break;
    case 3: data = (256 * A + B) / 4.0; break;
    case 4: data = ((256 * A + B) / 10.0)-40.0; break; 
    case 5: data = A-50; break; //((A*9)/5)-58; break; // SEGUNDA FORMULA = FARENHEIT
    case 6: // Short Trim / Long Trim: limit exclusively to [-30, +30]
      data = (A / 1.28) - 100.0;
      if (data < -30.0) data = -30.0;
      else if (data > 30.0) data = 30.0;
      break;
    case 7: {
      long adcSum = 0;
      for (int i = 0; i < 5; i++) {
        adcSum += analogRead(ADC_PIN);
        if (i < 4) delay(10);
      }
      float adcAvg = adcSum / 5.0f;
      data = (adcAvg - 600.0f) / (145.0f * 1.5f);
      if (data < 0.0f) data = 0.0f;
      break;
    }
      
    case 8: data = 100*digitalRead(WATER_PIN) ; break;//((analogRead(ADC_PIN))/41);data=data/10.0f; break;
    //case 8: data = ((digitalRead(WATER_PIN))-1)*(-100) ; break;
      //more formula
  }  //switch fomula



  switch (layout) {
    case 2:
      {  // 7 numeric meters (Linha 1: 2, Linha 2: 3 estreitos, Linha 3: 2)
        switch (pidNo) {
          case 0: plotNumeric(1, data, warn, digit); break;          // Linha 1 Esq: CVT Temp
          case 1: plotNumeric(7, data, warn, digit); break;          // Linha 1 Dir: Coolant
          case 2: plotNumericNarrow(0, data, warn, digit); break;    // Linha 2 Esq: Short Trim
          case 3: plotNumericNarrow(1, data, warn, digit); break;    // Linha 2 Centro: AFR
          case 4: plotNumericNarrow(2, data, warn, digit); break;    // Linha 2 Dir: Long Trim
          case 5: plotNumeric(3, data, warn, digit); break;          // Linha 3 Esq: Water Level
          case 6: plotNumeric(9, data, warn, digit); break;          // Linha 3 Dir: Oil Press
          case 7: engine_onoff(data, pidNo); break;                  // Engine monitor
        }
      }
      break;    
    case 3:
      {  // 7 arc meters (Linha 1: 2, Linha 2: 3 estreitos, Linha 3: 2)
        switch (pidNo) {
          case 0: plotArc(1, label, data, warn, min, max, digit); break;          // Linha 1 Esq: CVT Temp
          case 1: plotArc(7, label, data, warn, min, max, digit); break;          // Linha 1 Dir: Coolant
          case 2: plotArcNarrow(0, label, data, warn, min, max, digit); break;    // Linha 2 Esq: Short Trim
          case 3: plotArcNarrow(1, label, data, warn, min, max, digit); break;    // Linha 2 Centro: AFR
          case 4: plotArcNarrow(2, label, data, warn, min, max, digit); break;    // Linha 2 Dir: Long Trim
          case 5: plotArc(3, label, data, warn, min, max, digit); break;          // Linha 3 Esq: Water Level
          case 6: plotArc(9, label, data, warn, min, max, digit); break;          // Linha 3 Dir: Oil Press
          case 7: engine_onoff(data, pidNo); break;                               // Engine monitor
        }
      }
      break;
  }                 //switch page
  bt_message = "";  //reset
}  //updatemeter
/*-------------------------------*/

//auto dim backlight function
void autoDim() {
  int light = analogRead(LDR_PIN);//read light
  if (light > LIGHT_LEVEL) {//low light
    low_count++;
    if (low_count > 10) {
      low_count = 10;//low amb light 10 times count
      if (!dim) {
        ledcWrite(backlightChannel, 50);//dim light
        dim = true;
        #ifdef SERIAL_DEBUG
        Serial.println(F("Backlight -> dim"));
        #endif
      }//if !dim
    }//if low_count
    high_count = 0;
  
  } else {
    high_count++;
    if (high_count >10) {
      high_count = 10;//high amb light 10 times count
      if (dim) {
        ledcWrite(backlightChannel, 255);//brightest
        dim = false;
        #ifdef SERIAL_DEBUG
        Serial.println(F("Backlight -> bright"));
        #endif
      }//if dim
    }//if high_count  
    low_count = 0;
  }//if light > lightlevel
}//autodim
//---------------------------
/*############################################*/
