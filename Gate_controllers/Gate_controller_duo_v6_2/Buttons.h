#pragma once
#include "Config.h"


byte button_pin[BUT_NUM];
byte prev_button_stat;
svsTimer button_tm;
bool double_pressed = false;
byte last_pressed_button;
#define DOUBLECLICK_TIMEOUT 500 //ms
byte button_emulator_val = 0;
bool button_emulator_stat = false;
svsTimer button_emulator_t;
int current_button, prev_current_button;
bool prev_button_mode;
bool long_pressed = false;


byte m_read_buttons () {
  byte val = 0;
  byte output_val = 0;
  for (int i = 0; i < BUT_NUM; i++) bitWrite(val, i, digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  if (val) {
    if (last_pressed_button == val && prev_button_stat == 0) double_pressed = true;
    else button_tm.reset();
    last_pressed_button = val;
  }
  if (button_tm.ready(DOUBLECLICK_TIMEOUT)) {
    if (double_pressed) output_val = last_pressed_button * 10;
    else output_val = last_pressed_button;
    double_pressed = false;
    last_pressed_button = 0;
  }
  prev_button_stat = val;
  return output_val;  
}

/*byte button_emulator () {
  String text_val = Serial_read_data();
  if (text_val.indexOf("open") > -1) button_emulator_val = 1;
  else if (text_val.indexOf("close") > -1) button_emulator_val = 2;
  else if (text_val.indexOf("wicket") > -1) button_emulator_val = 4;
  else if (text_val.indexOf("stop") > -1) button_emulator_val = 8;
  if (button_emulator_t.ready(100)) {
    button_emulator_val = 0;
  }
  return button_emulator_val;
}*/

int g_read_buttons () {
  int output_val = false;
  byte val = 0;
  for (int i = 0; i < BUT_NUM; i++) bitWrite(val, i, !digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  if (val && !long_pressed) {
    long_pressed = true;
    button_tm.reset();
  }
  if (button_tm.ready(200) && long_pressed) {
    if (val) output_val = val;
    else output_val = 0;
    long_pressed = false;
    delay(100);
  }
  return output_val;  
}