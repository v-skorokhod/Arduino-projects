#include "Output_controller.h"
#include "MyLED.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
String data_line[4];
svsTimer LCD_tm;
bool GARAGE_MODE = false;
output_controller left_gate;
output_controller right_gate;
svsTimer gate_tm;
bool start_stat = false;
svsTimer button_hold_tm;
//svsTimer garage_button_hold_tm;
svsTimer wicket_tm;
//svsTimer garage_wicket_tm;
float set_current_val = 0.0; // A

//byte output_garage_driver_mode;
String output_driver_mode_str;

void show_lcd () {
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(0,i); 
    lcd.print(data_line[i]);
  }
  if (LCD_tm.ready(500)) {
    lcd.clear();
    for (int i = 0; i < 4; i++) data_line[i] = "";
  }
}

void startDesktop () {
  lcd.setCursor(0,1); lcd.print(" Skorokhod&Co 2024");
  lcd.setCursor(0,2); lcd.print(filenameIDE); 
  delay(1000);
  lcd.clear();
}

void main_gate_init () {
  Serial.println("Main_gate_module");
  lcd.init(); 
  lcd.backlight();
  startDesktop();
  right_gate.init(r_current_pin,r_forward_pin,r_reverse_pin,r_end_sw_pin,"RIGTH"); // current_pin, forward_pin, reverse_pin, end_switch_pin (пины правой створки)
  left_gate.init(l_current_pin,l_forward_pin,l_reverse_pin,l_end_sw_pin,"LEFT"); // current_pin, forward_pin, reverse_pin, end_switch_pin (пины левой створки)
  pinMode(left_relay_pin, OUTPUT);
  pinMode(right_relay_pin, OUTPUT);
  pinMode(garage_output_pin, OUTPUT);
  pinMode(LED_pin, OUTPUT);
}

void output_driver () {
  
  prev_current_button = current_button;
  //current_button = button_emulator(); // <--раскомментировать чтобы работало с консоли (либо то, либо то) 
  current_button = m_read_buttons(); // <-------раскомментировать чтобы работало с  пульта (либо то, либо то) 
  int IO_button = read_IO_data(); if (IO_button != no_data_val) current_button = IO_button;
  if (current_button) {
    if (current_button == 80) {
      set_current_val = HC_OFFSET;
    } else if (current_button == 8) {
      output_driver_mode = 0;
      send_IO_data(8);
    } else if (current_button < 10) {
      output_driver_mode = current_button;
      right_gate.reset_error();
      left_gate.reset_error();
      set_current_val = 0.0;
      start_stat = false;
      button_hold_tm.reset();
      wicket_tm.reset(); 
      gate_tm.reset(); 
    } else {
      if (current_button == 10) send_IO_data(1);
      else if (current_button == 20) send_IO_data(2);
      else if (current_button == 40) send_IO_data(4);
    }
  } 
  right_gate.set_current(set_current_val);
  left_gate.set_current(set_current_val);

  switch (output_driver_mode) {
    case 0: // stop
      digitalWrite(left_relay_pin, HIGH);
      digitalWrite(right_relay_pin, HIGH);
      right_gate.set(0);
      left_gate.set(0);
      start_stat = false;
      right_gate.reset_error();
      left_gate.reset_error();
      gate_tm.reset();
      button_hold_tm.reset();
      wicket_tm.reset();
      output_driver_mode_str = "* STOPPED"; 
      break;
   case 1: // open by timer
      digitalWrite(left_relay_pin, LOW);
      digitalWrite(right_relay_pin, LOW);

      right_gate.set(100);
      if (gate_tm.ready(open_gate_delay)) start_stat = true; // left gate delay 
      if (start_stat) left_gate.set(100); 
      else left_gate.set(0);

      if (button_hold_tm.ready(button_hold_delay)) {
        output_driver_mode = 0;
      } 

      if (left_gate.error_check() || right_gate.error_check()) {
        set_LED_color(3); 
      } else {
        if (set_current_val) {
          output_driver_mode_str = "* OPEN_HC"; 
          set_LED_color(4); 
        } else {
          output_driver_mode_str = "* OPEN"; 
          set_LED_color(2);
        }
      }
      break;
    case 2: // close by timer
      digitalWrite(left_relay_pin, LOW);
      digitalWrite(right_relay_pin, LOW);

      left_gate.set(-100);
      if (right_gate.position() >= (left_gate.position() + 30.0) || left_gate.position() == 0) {
        if (gate_tm.ready(open_gate_delay)) start_stat = true; //                                right gate delay 
        if (start_stat) right_gate.set(-100); 
        else right_gate.set(0);
      } else {
        right_gate.set(0);
      }
      if (button_hold_tm.ready(button_hold_delay)) {
        output_driver_mode = 0;
      } 

      if (left_gate.error_check() || right_gate.error_check()) {
        set_LED_color(3); 
      } else {
        if (set_current_val) {
          output_driver_mode_str = "* CLOSE_HC"; 
          set_LED_color(4); 
        } else {
          output_driver_mode_str = "* CLOSE"; 
          set_LED_color(2);
        }
      }
      break;
    case 4: // open rigth gate by timer
      digitalWrite(left_relay_pin, HIGH);
      digitalWrite(right_relay_pin, LOW);
      right_gate.set(100);
      left_gate.set(0); 
      if (wicket_tm.ready(wicket_delay)) {
        output_driver_mode = 0;
      } 
      if (left_gate.error_check() || right_gate.error_check()) {
        set_LED_color(3); 
      } else {
        if (set_current_val) {
          output_driver_mode_str = "* WICKET_HC"; 
          set_LED_color(4); 
        } else {
          output_driver_mode_str = "* WICKET"; 
          set_LED_color(2);
        }
      }
      break;
  }  
}

void main_gate_loop () {
  output_driver();
  serial_print(output_driver_mode_str + " >>> " + String(button_hold_tm.time_left()) + " * \r\n" + right_gate.get_feedback() + "\r\n" + left_gate.get_feedback() + "\r\n ********************");
  data_line[0] = output_driver_mode_str + ">>>" + String(button_hold_tm.time_left());
  data_line[1] = "--pos|pwr|cur|sw";
  data_line[2] = "R:"+ right_gate.get_feedback(true) + "<";
  data_line[3] = "L:"+ left_gate.get_feedback(true)  + "<";
  show_lcd();
}