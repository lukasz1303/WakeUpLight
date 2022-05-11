#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Arduino.h>
#include <TM1637Display.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define BUTTON_DOWN_PIN 2
#define BUTTON_UP_PIN   3
#define CLK             4
#define DIO             5
#define ALARM_OFF_PIN   6
#define LED_PIN         7
#define BUTTON_MENU     8
#define BUTTON_MODE     9
#define ALARM_PIN       10
#define LED_LIGHT_PIN   11
#define LED_ALARM_PIN   12

#define ALARM_TIME_HOUR_ADDRESS   0
#define ALARM_TIME_MINUTE_ADDRESS 1
#define LIGHT_DURATION_ADDRESS    2
#define ALARM_MODE_ADDRESS        3

#define NUMBER_OF_LEDS 16

TM1637Display display(CLK, DIO);
tmElements_t tm;
int number = 0;
int dots = 0;
int alarm = 0;
int light_duration;
bool menu = false;
bool anim = false;
bool alarm_on = false;
bool light_on = false;
bool next_alarm_off = false;

int alarm_interval = 100;
int step_time;
int dot_time = 500;
int alarm_time;
unsigned long last_step_time = 0;
unsigned long last_dot_time = 0;

int r = 0;
int g = 0;
int b = 0;
int cr = 0;
int cg = 0;
int cb = 0;
int nr = 0;

bool green_led = false;
bool blue_led = false;

int menu_level = 0;
int alarm_level;

int button_MENU = LOW;
int button_MENU_last = LOW;
int button_UP = LOW;
int button_UP_last = LOW;
int button_DOWN = LOW;
int button_DOWN_last = LOW;
unsigned long button_up_time = 0;
unsigned long button_down_time = 0;
int button_up_down_period = 1200;
int button_alarm_mode = LOW;
int button_alarm_mode_last = LOW;
int button_alarm_off = LOW;

unsigned long alarm_sound_time = 0;
unsigned long button_menu_time = 0;

bool change = false;
bool first = true;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_OF_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_LIGHT_PIN, OUTPUT);
  pinMode(LED_ALARM_PIN, OUTPUT);
  pinMode(BUTTON_UP_PIN, INPUT);
  pinMode(BUTTON_MENU, INPUT);
  pixels.begin();
  
  int h = EEPROM.read(ALARM_TIME_HOUR_ADDRESS);
  if(h < 0 || h > 23){
    h = 12;
  }
  int m = EEPROM.read(ALARM_TIME_MINUTE_ADDRESS);
  if(m < 0 || m > 59){
    m = 0;
  }
  alarm_time = h*100 + m;
  
  light_duration = EEPROM.read(LIGHT_DURATION_ADDRESS);
  if (light_duration < 5 || light_duration > 60){
    light_duration = 30;
    }

  step_time = int(light_duration * 100 / 7.27);
    
  alarm_level = EEPROM.read(ALARM_MODE_ADDRESS);
  if (alarm_level > 3 || alarm_level < 0){
    alarm_level = 0;
  }
}

void loop()
{
     
  if (RTC.read(tm)) {
    number = tm.Hour * 100 + tm.Minute;
  }

  button_MENU = digitalRead(BUTTON_MENU);
  if (button_MENU != button_MENU_last) {
    change = true;
  }

  if (button_MENU == HIGH && button_MENU_last == LOW) {
    button_menu_time = millis();
    if (menu) {
      menu_level++;
      first = true;
      if (menu_level > 4) {
        menu = false;
      }
    }
  }
  if (change) {
    if (button_MENU == HIGH && button_MENU_last == HIGH && (millis() - button_menu_time) > 1800) {
      menu = !menu;
      menu_level = 0;
      change = false;
      button_menu_time = millis();
    }
  }
  
  button_MENU_last = button_MENU;
  if (alarm_level >= 2  && number == time_subtraction(alarm_time,light_duration) && tm.Second == 0) {
    light_on = true;
  }
  
  if (light_on) {
    if (number >= time_subtraction(alarm_time,light_duration)) {
      anim = true;
    }
  }
  else{
      clear_leds();
      anim = false;
  }
  
  if (anim) {
    if (millis() - last_step_time > (step_time - 6)) {
      last_step_time = millis();
      update_color();
      pixels.setPixelColor(nr, cr, cg, cb);
      pixels.setPixelColor(15-nr, cr, cg, cb);    
      pixels.show();
    }
  }

  display.setBrightness(0);

  if (menu) {
    switch (menu_level) {
      case 0:
        number = int(number / 100);
        if (button_DOWN == HIGH || button_UP == HIGH) {
          display.showNumberDecEx(number, 0b00000000, true, 2, 0);
        } else {
          set_display(0, 2);
        }
        break;
      case 1:
        if (first) {
          display.showNumberDecEx(number, 0b00000000, true, 4, 0);
          first = false;
        }
        number = number % 100;
        if (button_DOWN == HIGH || button_UP == HIGH) {
          display.showNumberDecEx(number, 0b00000000, true, 2, 2);
        } else {
          set_display(2, 2);
        }
        break;
      case 2:
        if (first) {
          display.showNumberDecEx(alarm_time, 0b00000000, true, 4, 0);
          first = false;
        }
        number = int(alarm_time / 100);
        if (button_DOWN == HIGH || button_UP == HIGH) {
          display.showNumberDecEx(number, 0b00000000, true, 2, 0);
        } else {
          set_display(0, 2);
        }
        break;
      case 3:
        if (first) {
          display.showNumberDecEx(alarm_time, 0b00000000, true, 4, 0);
          first = false;
        }
        number = alarm_time % 100;
        if (button_DOWN == HIGH || button_UP == HIGH) {
          display.showNumberDecEx(number, 0b00000000, true, 2, 2);
        } else {
          set_display(2, 2);
        }
        break;
      case 4:
        if (first) {
          display.showNumberDecEx(light_duration, 0b00000000, false, 4, 0);
          first = false;
        }
        number = light_duration;
        if (button_DOWN == HIGH || button_UP == HIGH) {
          display.showNumberDecEx(number, 0b00000000, true, 2, 2);
        } else {
          set_display(2, 2);
        }
        break;
    }
  }
  else {
    if (millis() - last_dot_time > (dot_time - 6)) {
      display.showNumberDecEx(number, 0b01000000, true, 4, 0);
      if (millis() - last_dot_time > (dot_time - 6) * 2) {
        last_dot_time = millis();
      }
    } else {
      display.showNumberDecEx(number, 0b00000000, true, 4, 0);
    }
  }

  button_alarm_off = digitalRead(ALARM_OFF_PIN);
  
  if(button_alarm_off==HIGH){
    if(light_on && !alarm_on){
      next_alarm_off = true;
    }
    alarm_on = false;
    light_on = false;
    digitalWrite(ALARM_PIN, LOW);
  }
  
  if ((alarm_level == 1 || alarm_level == 2) && number == alarm_time && tm.Second == 0) {
    if(!next_alarm_off){
      alarm_on = true;
      alarm = 4;
    }
  }
  if ((alarm_level == 1 || alarm_level == 2) && number == alarm_time && tm.Second == 1) {
    next_alarm_off = false;
  }
  if(alarm_on){
    if ((millis() - alarm_sound_time) < 75 && alarm <4) {
      digitalWrite(ALARM_PIN, HIGH);
      
    } else {
      if ((millis() - alarm_sound_time) > 150){
        alarm_sound_time = millis();
        alarm++;
        if(alarm>6){
          alarm = 0;
        }
      }
      digitalWrite(ALARM_PIN, LOW);
    }
  }


  button_UP = digitalRead(BUTTON_UP_PIN);
  button_DOWN = digitalRead(BUTTON_DOWN_PIN);

  if (menu) {
    if (button_UP == HIGH && button_UP_last == LOW) {
      button_up_time = millis();
      button_up();
    }
    if (button_UP == HIGH && button_UP_last == HIGH && (millis() - button_up_time) > button_up_down_period) {
      button_up();
      button_up_down_period = 130;
      button_up_time = millis();
    }
    if (button_UP == LOW && button_UP_last == HIGH) {
      button_up_down_period = 1200;
    }

    if (button_DOWN == HIGH && button_DOWN_last == LOW) {
      button_down_time = millis();
      button_down();
    }
    if (button_DOWN == HIGH && button_DOWN_last == HIGH && (millis() - button_down_time) > button_up_down_period) {
      button_down();
      button_up_down_period = 130;
      button_down_time = millis();
    }
    if (button_DOWN == LOW && button_DOWN_last == HIGH) {
      button_up_down_period = 1200;
    }
  }

  button_alarm_mode = digitalRead(BUTTON_MODE);
  if (button_alarm_mode == HIGH && button_alarm_mode_last == LOW) {
    alarm_level++;
    if (alarm_level > 3) {
      alarm_level = 0;
    }
    EEPROM.update(ALARM_MODE_ADDRESS, alarm_level);
  }

  switch (alarm_level) {
    case 0:
      digitalWrite(LED_LIGHT_PIN, LOW);
      digitalWrite(LED_ALARM_PIN, LOW);
      break;
    case 1:
      digitalWrite(LED_LIGHT_PIN, LOW);
      digitalWrite(LED_ALARM_PIN, HIGH);
      break;
    case 2:
      digitalWrite(LED_LIGHT_PIN, HIGH);
      digitalWrite(LED_ALARM_PIN, HIGH);
      break;
    case 3:
      digitalWrite(LED_LIGHT_PIN, HIGH);
      digitalWrite(LED_ALARM_PIN, LOW);
      break;
  }
  
  button_UP_last = button_UP;
  button_DOWN_last = button_DOWN;
  button_alarm_mode_last = button_alarm_mode;

  if (number>=time_adding(alarm_time,3)){
    alarm_on = false;
    light_on = false;
  }
}

void update_color() {
  nr++;
  if (nr > 7) {
    r += 1;
    if (r == 160) { green_led = true;}
    
    if (green_led) {
      g += 1;
      if (g == 150) { blue_led = true;}
    }
    if (blue_led) {
      b += 1;
    }
    if (r >= 252) { r = 252;}
    if (g >= 240) { g = 240;}
    if (b >= 235) { b = 235;
      green_led = false; blue_led = false;
    }
    nr = 0;
    cr = int(0.0028*r*r+0.28*r+1);
    cg = int(0.0028*g*g+0.28*g);
    cb = int(0.0028*b*b+0.28*b);
  }
}

void clear_leds() {
  anim = false;
  int m = 0;
  if (r != 0 || g != 0 || b != 0) {  
    r = 0; g = 0; b = 0; nr = 7;
    cr = 0; cg = 0; cb = 0;
    green_led = false; blue_led = false;
    while (m < 8) {
      pixels.setPixelColor(m, 0, 0, 0);
      pixels.setPixelColor(15-m, 0, 0, 0);
      m++;
    }
    pixels.show();
  }
}

void set_display(int start, int len) {
  if (millis() - last_dot_time > (dot_time - 6)) {
    display.showNumberDecEx(number, 0b00000000, true, len, start);
    if (millis() - last_dot_time > (dot_time - 6) * 2) {
      last_dot_time = millis();
    }
  } else {
    uint8_t data[] = { 0, 0, 0, 0 };
    display.setSegments(data, len, start);
  }
}

int time_subtraction(int time, int minute) {
  int hour1 = int(time / 100);
  int hour2 = int(minute / 60);
  int hour_result = hour1 - hour2;
  if (hour_result < 0) {
    hour_result = 24 + hour_result;
  }

  int minute1 = time % 100;
  int minute2 = minute % 60;
  int minute_result = minute1 - minute2;
  if (minute_result < 0) {
    minute_result = 60 + minute_result % 60;
    hour_result = hour_result - 1;
    if (hour_result < 0) {
      hour_result = 24 + hour_result;
    }
  }
  return hour_result * 100 + minute_result;
}

int time_adding(int time, int minute) {
  int hour1 = int(time / 100);
  int hour2 = int(minute / 60);
  int hour_result = hour1 + hour2;
  if (hour_result > 23) {
    hour_result = hour_result-24;
  }

  int minute1 = time % 100;
  int minute2 = minute % 60;
  int minute_result = minute1 + minute2;
  if (minute_result > 59) {
    minute_result = minute_result % 60;
    hour_result = hour_result + 1;
    if (hour_result > 23) {
    hour_result = hour_result-24;
    }
  }
  return hour_result * 100 + minute_result;
}

void button_up() {
  switch (menu_level) {
    case 0:
      tm.Hour = tm.Hour - 1;
      if (tm.Hour <= 0) {
        tm.Hour = 23;
      }
      RTC.write(tm);
      break;
    case 1:
      tm.Minute = tm.Minute - 1;
      if (tm.Minute <= 0) {
        tm.Minute = 59;
      }
      RTC.write(tm);
      break;
    case 2:
      alarm_time = alarm_time - 100;
      if (alarm_time <= 0) {
        alarm_time = alarm_time + 2300;
      }
      next_alarm_off = false;
      EEPROM.update(ALARM_TIME_HOUR_ADDRESS, int(alarm_time/100));
      break;
    case 3:
      alarm_time = alarm_time - 1;
      if (alarm_time % 100 >= 60) {
        alarm_time = alarm_time + 60;
      }
      next_alarm_off = false;
      EEPROM.update(ALARM_TIME_MINUTE_ADDRESS, alarm_time%100);
      break;
    case 4:
      light_duration = light_duration - 1;
      if (light_duration < 5) {
        light_duration = 5;
      }
      EEPROM.update(LIGHT_DURATION_ADDRESS, light_duration);
      step_time = int(light_duration * 100 / 7.27);
      break;
  }
}

void button_down() {
  switch (menu_level) {
    case 0:
      tm.Hour = tm.Hour + 1;
      if (tm.Hour > 23) {
        tm.Hour = 0;
      }
      RTC.write(tm);
      break;
    case 1:
      tm.Minute = tm.Minute + 1;
      if (tm.Minute > 59) {
        tm.Minute = 0;
      }
      RTC.write(tm);
      break;
    case 2:
      alarm_time = alarm_time + 100;
      if (alarm_time > 2399) {
        alarm_time = alarm_time - 2400;
      }
      next_alarm_off = false;
      EEPROM.update(ALARM_TIME_HOUR_ADDRESS, int(alarm_time/100));
      break;
    case 3:
      alarm_time = alarm_time + 1;
      if (alarm_time % 100 >= 60) {
        alarm_time = alarm_time - 60;
      }
      next_alarm_off = false;
      EEPROM.update(ALARM_TIME_MINUTE_ADDRESS, alarm_time%100);
      break;
    case 4:
      light_duration = light_duration + 1;
      if (light_duration > 60) {
        light_duration = 60;
      }
      EEPROM.update(LIGHT_DURATION_ADDRESS, light_duration);
      step_time = int(light_duration * 100 / 7.27);
      break;
  }
}
