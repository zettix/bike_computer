#include "DS3232RTC.h"
#include "Time.h"
#include "EEPROM.h"
#include "Wire.h"

#include "U8glib.h"
#include "buttonface.h"
#include "TimeBoard.h"



// simple button OLED test rig.
/*
   [6]
[5]   [3]   [7]
   [4]
   */
   
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST);

TimeBoard *timeboard = NULL;

int mode = 0;
int num_states = 6;  /// CHANGE FOR NUMBER OF VIEWS/DISPLAYS

char *months[] = {
   "Jan",
   "Feb",
   "Mar",
   "Apr",
   "May",
   "Jun",
   "Jul",
   "Aug",
   "Sep",
   "Oct",
   "Nov",
   "Dec"};

ButtonFace fire_b;
ButtonFace dpad_u;
ButtonFace dpad_d;
ButtonFace dpad_l;
ButtonFace dpad_r;

int rcount = 0;
int delta_time = 0;

float speed = 0.0;

char timeBUFFf[25];
char time_5[16];
char time_12[16];
char speedBUFFf[16];
char tempBUFFf[16];
char space1;
char space2;
int param_selection;
int time_param_selection;

// Dimensions of display:
int xres = 128;
int yres = 64;

// unsaved, per power session
float c1, c2, c3;
int mph, mph_offset;
int display_cycle_timeout;
int display_timeout;
int display_select;
unsigned long ago;

int xpos, ypos;

// saved to EEPROM
char time_standard = '1';
char distance_standard = 'M'; // U for USA, miles
char degree_standard = 'M'; // U for USA, faranheit
float odo = 0.0;
unsigned long icnt = 0;
unsigned long distance = 0;
unsigned long wheel = 2067;

int oldsec = 0;
int timeo = 0;

int save_at(int address, int data_len, char *p) {
  static char debug[30];
  int x;
  for (x = 0; x < data_len; x++) {
    EEPROM.write(address, *p);
    snprintf(debug, 30, "Wrote %d: %x\n", address, *p);
    Serial.print(debug);
    address++;
    p++;
  }
  return address;
}

int load_at(int address, int data_len, char *p) {
  int x;
  static char debug[30];
  
  for (x = 0; x < data_len; x++) {
    *p = EEPROM.read(address);
    snprintf(debug, 30, "Read %d: %x\n", address, *p);
    Serial.print(debug);
    address++;
    p++;
  }
  return address;
}

void save_conf(void) {
  int address = 0;
  address += save_at(address, sizeof(char), (char *) &time_standard);
  address += save_at(address, sizeof(char), (char *) &distance_standard);
  address += save_at(address, sizeof(char), (char *) &degree_standard);
  address += save_at(address, sizeof(float), (char *) &odo);
  address += save_at(address, sizeof(unsigned long), (char *) &icnt);
  address += save_at(address, sizeof(unsigned long), (char *) &distance);
  address += save_at(address, sizeof(unsigned long), (char *) &wheel);
}

void load_conf(void) {
  int address = 0;
  address += load_at(address, sizeof(char), (char *) &time_standard);
  address += load_at(address, sizeof(char), (char *) &distance_standard);
  address += load_at(address, sizeof(char), (char *) &degree_standard);
  address += load_at(address, sizeof(float), (char *) &odo);
  address += load_at(address, sizeof(unsigned long), (char *) &icnt);
  address += load_at(address, sizeof(unsigned long), (char *) &distance);
  address += load_at(address, sizeof(unsigned long), (char *) &wheel);
}

char *gettime() {   
  return (timeboard->GetTime());
}
char *gettime5() {   
  return(timeboard->GetTime5());
}

char *gettime12() {
  return (timeboard->GetTime12(time_12));
}

void getspeed(void) {
  if (mph < 10) {
    mph_offset = 38;
    snprintf(speedBUFFf,7, " %d", mph);
  } else {
    mph_offset = 0;
    snprintf(speedBUFFf,7, "%02dm", mph);
  }
}

char cursorchars[2] = {0xde, 0x00};

// char cursorchars[2] = ">";


void SetCursor(uint8_t x, uint8_t y, bool enablex) {
    bool enbled = enablex;
    if (enablex) {
      //u8g.enableCursor();
      // u8g.setFont(u8g_font_cursor);
      //u8g.setCursorStyle(0x90);
      u8g.setColorIndex(1);
    } else {
      u8g.setColorIndex(0);
    }
      u8g.drawStr(x, y, cursorchars);
      //u8g.disableCursor();
      //return;
    //u8g.setCursorPos(x, y);
        
    u8g.setColorIndex(1);
}

uint8_t offset = 11;
uint8_t ystep = 16;
uint8_t coffset = 0;

uint8_t param_y_calc(int param_num) {
  return (uint8_t) ((param_num % 4 * ystep + ystep - 1) % 64);
}

void layout_settime(int current_param) {
  char buf[32];
  char colon[2] = {':', 0x00};
  int xh = hour();
  int xm = minute();
  int xs = second();
  int xM = month();
  int xD = day();
  int xY = year();
  static int old_param = 0;
  bool flash = true;
  int ppp;  // local param
  u8g.setFont(u8g_font_courB14);
  if (second() % 2 == 0) {
    flash = false;
  }
  if (current_param != old_param) {
     SetCursor((uint8_t) coffset, param_y_calc(old_param), false);
     old_param = current_param;
  }
         
  SetCursor((uint8_t) coffset, param_y_calc(current_param), flash);

  if (current_param < 4) {
    // Param 0 Hour
    ppp = 0;
    snprintf(buf, 32, "Hr: %d", xh);
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    
    ppp = 1;
    snprintf(buf, 32, "Min: %d", xm);
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    
    ppp = 2;
    snprintf(buf, 32, "Sec: %d", xs);
    u8g.drawStr(offset, param_y_calc(ppp), buf);

    ppp = 3;
    snprintf(buf, 32, "Year: %d", xY);
    u8g.drawStr(offset, param_y_calc(ppp), buf);
  }
  
  if ((current_param > 3) && (current_param < 8)) {
    // Param 4
    ppp = 4;
    snprintf(buf, 32, "Month: %d", xM);
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    
    ppp = 5;
    snprintf(buf, 32, "Day: %d", xD);
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    
    ppp = 6;
    snprintf(buf, 32, "Set");
    u8g.drawStr(offset, param_y_calc(ppp), buf);
  }
}


void layout_params(int current_param) {
  char buf[32];
  static int old_param = 0;
  bool flash = true;
  int ppp;  // local param
  u8g.setFont(u8g_font_courB14);
  if (second() % 2 == 0) {
    flash = false;
  }
  if (current_param != old_param) {
     SetCursor((uint8_t) coffset, param_y_calc(old_param), false);
     old_param = current_param;
  }
         
  SetCursor((uint8_t) coffset, param_y_calc(current_param), flash);

  if (current_param < 4) {
    // Param 0 Wheel
    ppp = 0;
    snprintf(buf, 32, "Wheel:");
    u8g.drawStr(offset, ystep * ppp + ystep - 1, buf);
    snprintf(buf, 32, "%04d", wheel);
    u8g.drawStr(xres / 2 + offset - 2, param_y_calc(ppp), buf);
    // Param 1 MPH/KPH
    ppp = 1;
    if (distance_standard == 'M') {
      snprintf(buf, 32, "Meters");
    } else {
      snprintf(buf, 32, "Miles");
    }
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 2  C/F temp
    ppp = 2;
    if (degree_standard == 'M') {
      snprintf(buf, 32, "Celcius");
    } else {
      snprintf(buf, 32, "Faranheit");
    }
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 3  12/24 hour
    ppp = 3;
    if (time_standard == '1') {
      snprintf(buf, 32, "12 Hour");
    } else {
      snprintf(buf, 32, "24 Hour");
    }
    u8g.drawStr(offset, param_y_calc(ppp), buf);
  } 
  if ((current_param > 3) && (current_param < 8)) {
    // Param 4
    ppp = 4;
    snprintf(buf, 32, "T: %d", icnt);
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 5
    ppp = 5;
    snprintf(buf, 32, "SerialLogger");
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 6 Load
    ppp = 6;
    snprintf(buf, 32, "Load");
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 7 Save
    ppp = 7;
    snprintf(buf, 32, "Save");
    u8g.drawStr(offset, param_y_calc(ppp), buf);
  }
  if ((current_param > 7) && (current_param < 12)) {
    // Param 8
    ppp = 8;
    snprintf(buf, 32, "Set Time");
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 9
  /*  ppp = 9;
    snprintf(buf, 32, "Diggigly");
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 10 Load
    ppp = 10;
    snprintf(buf, 32, "Iggily");
    u8g.drawStr(offset, param_y_calc(ppp), buf);
    // Param 11 Save
    ppp = 11;
    snprintf(buf, 32, "Piggily");
    u8g.drawStr(offset, param_y_calc(ppp), buf); */
  }
}

void gettemp() {
   char *buf = tempBUFFf;
   
   float temp;
   int big, litt;
        temp = timeboard->GetTemp();
        big = (int) temp;
        litt = (int) ((temp - big) * (10));
        if (big < 100) {
          if (big < 10) {
            snprintf(buf,6, "  %d.%d", big, litt);
          } else {
            snprintf(buf,6, " %d.%d", big, litt);
          }
        } else {
          snprintf(buf,6, "%d.%d", big, litt);
        }
}

void layout1(void) {
  // 21:14:16
  // 0 MPH
  //   75F
    float temp;
  gettime12();
  getspeed();
  // gettemp();
  temp = timeboard->GetTemp();
  int dtemp = (int) temp;
  // char mybuf[7];
  if (degree_standard == 'M') {
    snprintf(tempBUFFf, 5, "%dC", dtemp);
  } else {
    snprintf(tempBUFFf, 5, "%dF", dtemp);
  }
  u8g.setFont(u8g_font_fub49n);
  u8g.drawStr(mph_offset, yres - 1, speedBUFFf);
  u8g.setFont(u8g_font_courB14);
  u8g.drawStr(0, yres - 53, time_12);
  u8g.drawStr(84, yres, tempBUFFf);
  if (distance_standard == 'M') {
      u8g.drawStr(86, yres / 2 + 8, "KPH");
  } else {
    u8g.drawStr(86, yres / 2 + 8, "MPH");
  }
}

void AmPmTag() {
  if (time_standard != '1') {
    return;
  } 
  u8g.setFont(u8g_font_courB14);
  u8g.drawStr(114, 58, "M");
  if (timeboard->IsPm()) {
    u8g.drawStr(114, 43, "P");
  } else {
    u8g.drawStr(114, 43, "A");
  }
}

void layout_time(void) {
  gettime5();
  char buf[10];
  AmPmTag();
  snprintf(buf,9, "%s", months[month() - 1]);  
  u8g.setFont(u8g_font_courB14);
  u8g.drawStr(5, yres - 49, buf);
  snprintf(buf,9, "%d", day());  
  u8g.drawStr(55, yres - 49, buf);
  snprintf(buf,9, "%d", year());  
  u8g.drawStr(83, yres - 49, buf);
  u8g.setFont(u8g_font_fub30n);
  u8g.drawStr(8, yres - 1, time_5);
}

void layout_odometer(void) {
  char buf[10];
  u8g.setFont(u8g_font_courB14);
  u8g.drawStr(5, yres - 46, "Odometer");
  snprintf(buf, 9, "t: %ld", icnt);
  u8g.drawStr(5, yres - 30, buf);

}

void layout_temp(void) {
  gettemp();
  char buf[10];
  u8g.setFont(u8g_font_courB14);
  u8g.drawStr(5, yres - 46, "Temperature");
  u8g.drawStr(xres - 32, yres - 25, "o");
  if (timeboard->GetIsMetric()) {
    u8g.drawStr(xres- 17, yres -20, "C");
  } else {
    u8g.drawStr(xres- 17, yres -20, "F");
  }
  u8g.setFont(u8g_font_fub30n);
   float temp;
   int big, litt;
        temp = timeboard->GetTemp();
        big = (int) temp;
        litt = (int) ((temp - big) * (10));
        if (big < 100) {
          if (big < 10) {
            snprintf(buf,6, "  %d.%d", big, litt);
          } else {
            snprintf(buf,6, " %d.%d", big, litt);
          }
        } else {
          snprintf(buf,6, "%d.%d", big, litt);
        }

  u8g.drawStr(10, yres - 1, buf);
}


void layout_speed_time(void) {
  char buf[4];
  AmPmTag();
  getspeed();
  gettime5();
  u8g.setFont(u8g_font_fub30n);
  u8g.drawStr(10, yres - 1, time_5);
  u8g.drawStr(mph_offset + 10, yres - 32, speedBUFFf);
  u8g.setFont(u8g_font_courB14);
  if (distance_standard == 'M') {
      u8g.drawStr(80, yres / 2  - 12, "KPH");
  } else {
    u8g.drawStr(80, yres / 2 - 12, "MPH");
  }
}
void serviceInterrupt() {
  // wheel is 2067 mm in circ.
  // millis is 1000/sec
  // 60 * 60 * 1000 millis / hour
  // 1 mi = 1609344 mm
  // mi / hr =  (1mi / 1609344 mm) * (2067 mm / circ) * (1 circ / millis() secs) * ( 3600000 millis / hr)
  // 462.37 / millis() ??

  unsigned long delta = millis() - ago;
  odo += 2.067;  // TODO(me): make variable.
  timeo = 0;
  if (delta == 0) {
    return;
  }
  ago = millis();
  if (delta > 7000.0) {
    delta = 7000.0;
  }
  mph = int(4623.7 / delta);
  if (mph > 99) {
     mph  = 99;
  }
  icnt++;
}

void SetBoards() {
  //init boards.
  if (degree_standard == 'M') {
    timeboard->SetMetric(true);
  } else {
    timeboard->SetMetric(false);
  }
  if (time_standard == '1') {
    timeboard->Set24Hour(false);
   } else {
    timeboard->Set24Hour(true);
   }
}


void setup() {
  Serial.begin(9600);
    fire_b.setPin(7);
  fire_b.setActive(LOW);
  dpad_u.setPin(6);
  dpad_u.setActive(LOW);
  dpad_d.setPin(4);
  dpad_d.setActive(LOW);
  dpad_l.setPin(5);
  dpad_l.setActive(LOW);
  dpad_r.setPin(3);
  dpad_r.setActive(LOW);
  pinMode(2, INPUT_PULLUP);
  xpos = 100;
  ypos = 100;
  timeboard = new TimeBoard(timeBUFFf, time_5);
  display_timeout = 0;
  display_select = 0;
  u8g.setColorIndex(1);         // pixel on
  mph = 0;
  param_selection = 0;
  time_param_selection = 0;
  ago = millis();
  attachInterrupt(0, serviceInterrupt, RISING);
  load_conf();
  SetBoards();
}

void update() {
  dpad_u.update();
  dpad_d.update();
  dpad_l.update();
  dpad_r.update();
  fire_b.update();
  //fire_b.setPin(7);
  
  // Serial.write("states: ");
  if (dpad_u.isPressed()) {
    //dpad_u.resetPressed();
    ypos--;
     //Serial.write(" U ");
   }
  if (dpad_d.isPressed()) {
    //dpad_d.resetPressed();
    ypos++;
    //Serial.write(" D ");
  }
  if (dpad_l.isPressed()) {
    //dpad_l.resetPressed();
    xpos--;
    //Serial.write(" L ");
  }
  if (dpad_r.isPressed()) {
    //dpad_r.resetPressed();
    xpos++;
    //Serial.write(" R ");
  }
  if (fire_b.isPressed()) {
    // fire_b.resetPressed();
    //Serial.write(" F ");
    xpos = 10;
    ypos = 10;
  }
}

void DefaultButtonHandler() {
    //fire_b.update();
    if (fire_b.isPressed()) {
      display_select++;
      fire_b.resetPressed();
      //Serial.print("INC TO:");
      // Serial.print(display_select);
    }
    if (display_select > num_states - 1) {
      display_select = 0;
      //Serial.print("DISPLAY RESET!");
    }
  }


void ButtInfoToSerial(ButtonFace *b) {
  b->LogToSerial();
}
  
void LogToSerial() {
  char buf[30];
  
  snprintf(buf, 30, "Serial debug thing.\n");
  Serial.print(buf);
  Serial.print("L:"); 
  dpad_l.LogToSerial(); 
  Serial.print("R:");
  dpad_r.LogToSerial();
  Serial.print("U:");
  dpad_u.LogToSerial();
  Serial.print("D:");
  dpad_d.LogToSerial();
  Serial.print("F:");
  fire_b.LogToSerial();
  snprintf(buf, 30, "TB24: %c\n", (timeboard->GetIs24Hour()) ? 'T' : 'F');
  Serial.print(buf);
  snprintf(buf, 30, "TBC: %c.\n", (timeboard->GetIsMetric()) ? 'T' : 'F');
  Serial.print(buf);
}

void TimeButtonHandler() {
    bool lp = false;
    bool rp = false;
    int max_time_param_index = 6;   /// CHANGE FOR MAX PARAMS
    if (fire_b.isPressed()) {
      fire_b.resetPressed();
      display_select++;
      if (display_select > num_states) {
        display_select = 0;
      }
      return;
    }
    if (dpad_u.isPressed()) {
      time_param_selection--;
      dpad_u.resetPressed();
      if (time_param_selection < 0) {
        time_param_selection = max_time_param_index;
      }
    }
    if (dpad_d.isPressed()) {
      time_param_selection++;
      dpad_d.resetPressed();
      if (time_param_selection > max_time_param_index) {
        time_param_selection = 0;
      }
    }
    if (dpad_l.isPressed()) {
      lp = true;
      dpad_l.resetPressed();
    }
    if (dpad_r.isPressed()) {
      dpad_r.resetPressed();
      rp = true;
    }
    int xh = hour();
    int xm = minute();
    int xs = second();
    int xM = month();
    int xD = day();
    int xY = year();
    if (lp || rp) {
      switch (time_param_selection) {
        case 0:
          if (lp) {
             xh--;
            } else {
             xh++;
            }
            break;
        case 1:
          if (lp) {
             xm--;
            } else {
             xm++;
            }
            break;   
            
         case 2:
            xs = 0;
            break;   
         case 3:  // yr
          if (lp) {
             xY--;
            } else {
             xY++;
            }
            break;
         case 4:  // mon
          if (lp) {
             xM--;
            } else {
             xM++;
            }
            break;
          case 5:  // day
          if (lp) {
             xD--;
            } else {
             xD++;
            }
            break;          
         case 6:  // set
             timeboard->set(now());
             time_param_selection = 0;
            break; 
        default:
           break;
      }
      setTime(xh, xm, xs, xD, xM, xY);
   }
}


void ParamButtonHandler() {
    bool lp = false;
    bool rp = false;
    int max_param_index = 11;   /// CHANGE FOR MAX PARAMS
    if (fire_b.isPressed()) {
      display_select++;
      fire_b.resetPressed();
      //Serial.print("INC TO:");
      //Serial.print(display_select);
      if (display_select > num_states) {
        display_select = 0;
        //Serial.print("DISPLAY RESET!");
      }
      return;
    }
    if (dpad_u.isPressed()) {
      param_selection--;
      dpad_u.resetPressed();
      if (param_selection < 0) {
        param_selection = max_param_index;
      }
    }
    if (dpad_d.isPressed()) {
      param_selection++;
      dpad_d.resetPressed();
      if (param_selection > max_param_index) {
        param_selection = 0;
      }
    }
    if (dpad_l.isPressed()) {
      lp = true;
      dpad_l.resetPressed();
    }
    if (dpad_r.isPressed()) {
      dpad_r.resetPressed();
      rp = true;
    }
    if (lp || rp) {
      switch (param_selection) {
        case 0:
          if (lp) {
             wheel--;
            } else {
             wheel++;
            }
          break;
         case 3:
             time_standard = (time_standard == '1') ? '2' : '1';
             SetBoards();
             break;
           break;
          case 1:
             distance_standard = (distance_standard == 'M')?'U':'M';
             break;
           case 2:
             degree_standard = (degree_standard == 'M') ? 'U' : 'M';
             SetBoards();
             break;
         case 4:
          if (lp) {
               icnt--;
              } else {
               icnt++;
              }
             break;
           case 5:
             LogToSerial();
             break;
           case 6:
            load_conf();
            display_select = 0;
            param_selection = 2;
            SetBoards();
             break;
           case 7:
            save_conf();
            display_select = 0;
            param_selection = 2;
             break;
           case 8:  // Time set.
            display_select = 6;
            param_selection = 2;
           default:
             break;
      }
    }
}

void LayoutSelector() {
   //u8g.drawBitmap(0, 0, 16, 64, bitmap);
       switch (display_select) {
         case 0: layout1();
                 DefaultButtonHandler();
                 break;
         case 1: layout_time();
                 DefaultButtonHandler();
                 break;
         case 2: layout_speed_time();
                 DefaultButtonHandler();
                 break;
         case 3: layout_temp();
                 DefaultButtonHandler();
                 break;     
         case 4: layout_params(param_selection);
                 ParamButtonHandler();
                 break;
         case 5: layout_odometer();
                 DefaultButtonHandler();
                 break;
         case 6: layout_settime(time_param_selection);
                 TimeButtonHandler();
                 break;
         default:
                 layout_speed_time();
                 DefaultButtonHandler();
                 break;
       }    
}

void loop() {
  u8g.firstPage();
  do {
    update();
    //draw();
    LayoutSelector();
  } while( u8g.nextPage() );
  
  delay(5);
}
