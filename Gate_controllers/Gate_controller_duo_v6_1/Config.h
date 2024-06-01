#pragma once

#include <EEPROM.h>
#include "MyTimer.h"
#include "CommunicationPWM.h"

bool debug_serial = true;
#define debug_serial_period 200 // ms
svsTimer debug_serial_tm;
String data_str = "";
String prev_data_str;
byte here_pos = 0;

byte MODULE_ID;
#define BUT_NUM 4

// main_pins:
#define left_relay_pin 13
#define right_relay_pin 12
#define garage_output_pin 9
#define r_current_pin A7
#define r_forward_pin 10
#define r_reverse_pin 11
#define r_end_sw_pin A2
#define l_current_pin A6
#define l_forward_pin 3
#define l_reverse_pin 5
#define l_end_sw_pin A3
#define m_led_pin A0
#define m_IO_DATA_pin 9
int m_button_pin[BUT_NUM] = {6,8,4,2}; // open, close, wicket, stop (менять пины местами для соответсвия пульта)

// garage_pins:
#define power_rl_pin 2
#define reverse_rl_pin 4
#define g_led_pin 8
#define g_IO_DATA_pin 3
byte g_button_pin[BUT_NUM] = {A1,A0,7,6}; // open_g, close_g, open_m, close_m

#include "Buttons.h"

#define open_gate_delay 4000 // ms (задержка открывания створок)
#define button_hold_delay 35000 // ms (время удержания кнопки)
#define wicket_delay 15000 // ms (время открытия калитки кнопки)
//#define garage_button_hold_delay 30000 // ms (время удержания кнопки гаражных ворот)
//#define garage_wicket_delay 24000 // ms (время открытия калитки гаражных ворот)
#define HC_OFFSET 3.0 // A (High Current mode offset) 

byte output_driver_mode, prev_output_driver_mode;

void here () {
  Serial.println("HERE_" + String(here_pos++));
}

String Serial_read_data (byte serial_num = 0) {
  String val = "";
  bool if_available = false;
  switch (serial_num) {
    case 0:
      if (Serial.available() && !if_available) {
        if_available = true;   
        while(Serial.available()) {
          delayMicroseconds(200);
          val += char(Serial.read());     
        }
        if (debug_serial) Serial.println("Ser_0--> " + val + " <---Ser_0");
      }
      break; 
  }    
  return val;
}

void serial_print (String val) {
  if(debug_serial) {
    if (debug_serial_tm.ready(debug_serial_period)) {
      Serial.println(val);
    }
    prev_data_str = val;
  }
}
