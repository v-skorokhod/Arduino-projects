String filenameIDE = "Gate_contr._v5_0"; // ИЗМЕНЯТЬ ПОСЛЕ ИЗМЕНЕНИЯ НАЗВАНИЯ ФАЙЛА !!! ####################################################################
const bool TEST = false;

//#include "GSM.h"
#include "Output_controller.h"
#include "MyLED.h"
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
int button_pin[4] = {6,8,4,2}; // open, close, wicket, stop (менять пины местами для соответсвия пульта)

bool GARAGE_MODE = false;
output_controller left_gate;
output_controller right_gate;
int current_button, prev_current_button;
svsTimer gate_tm;
#define open_gate_delay 4000 // ms (задержка открывания створок)
#define button_hold_delay 35000 // ms (время удержания кнопки)
#define wicket_delay 15000 // ms (время открытия калитки кнопки)

#define garage_button_hold_delay 30000 // ms (время удержания кнопки гаражных ворот)
#define garage_wicket_delay 24000 // ms (время открытия калитки гаражных ворот)
bool start_stat = false;
svsTimer button_hold_tm;
svsTimer garage_button_hold_tm;
svsTimer wicket_tm;
svsTimer garage_wicket_tm;
float set_current_val = 0.0; // A
#define HC_OFFSET 3.0 // A (High Current mode offset) 
byte prev_button_stat;
svsTimer button_tm;
bool double_pressed = false;
byte last_pressed_button;
#define DOUBLECLICK_TIMEOUT 500 //ms
byte button_emulator_val = 0;
bool button_emulator_stat = false;
svsTimer button_emulator_t;
int output_driver_mode, prev_output_driver_mode;
int output_garage_driver_mode;
String output_driver_mode_str;

void startDesktop () {
  lcd.setCursor(0,1); lcd.print(" Skorokhod&Co 2024");
  lcd.setCursor(0,2); lcd.print(filenameIDE); 
  delay(1000);
  lcd.clear();
}
svsTimer LCD_tm;

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

void setup() {
  Serial.begin(57600);
  Serial.println("###########################");
  Serial.println("Programm_init");
  Serial.println(filenameIDE);
  Serial.println("###########################");
  delay(100);
  lcd.init(); 
  lcd.backlight();
  startDesktop();
  right_gate.init(r_current_pin,r_forward_pin,r_reverse_pin,r_end_sw_pin,"RIGTH"); // current_pin, forward_pin, reverse_pin, end_switch_pin (пины правой створки)
  left_gate.init(l_current_pin,l_forward_pin,l_reverse_pin,l_end_sw_pin,"LEFT"); // current_pin, forward_pin, reverse_pin, end_switch_pin (пины левой створки)
  for (int i = 0; i < 4; i++) pinMode(button_pin[i], INPUT);
  pinMode(left_relay_pin, OUTPUT);
  pinMode(right_relay_pin, OUTPUT);
  pinMode(garage_output_pin, OUTPUT);
  pinMode(LED_pin, OUTPUT);
  //test test test
  digitalWrite(left_relay_pin, LOW);
  delay(1000);
  digitalWrite(left_relay_pin, HIGH);
  //initGSM();
  //digitalWrite(right_relay_pin, LOW);
}

void loop() {
  here_pos = 0;
  prev_current_button = current_button;
  //current_button = button_emulator(); // <---------------------раскомментировать чтобы работало с консоли (либо то, либо то) 
  current_button = read_buttons(); // <---------------------раскомментировать чтобы работало с пульта (либо то, либо то) 
  output_driver();
  serial_print(output_driver_mode_str + " >>> " + String(button_hold_tm.time_left()) + " * \r\n" + right_gate.get_feedback() + "\r\n" + left_gate.get_feedback() + "\r\n ********************");
  data_line[0] = output_driver_mode_str + ">>>" + String(button_hold_tm.time_left());
  data_line[1] = "--pos|pwr|cur|sw";
  data_line[2] = "R:"+ right_gate.get_feedback(true) + "<";
  data_line[3] = "L:"+ left_gate.get_feedback(true)  + "<";
  show_lcd();
}

void output_driver () {
  if (current_button) {
    if (current_button == 80) {
      set_current_val = HC_OFFSET;
    } else if (current_button == 8) {
      output_driver_mode = 0;
      output_garage_driver_mode = 0;
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
      output_garage_driver_mode = current_button / 10;
      garage_button_hold_tm.reset();
      garage_wicket_tm.reset();
    }
  } 
  right_gate.set_current(set_current_val);
  left_gate.set_current(set_current_val);

  switch (output_garage_driver_mode) {
    case 0: // stop
      analogWrite(garage_output_pin, 128);
      garage_button_hold_tm.reset();
      garage_wicket_tm.reset();
      break;
    case 1: // open by timer
      analogWrite(garage_output_pin, 230);
      if (garage_button_hold_tm.ready(garage_button_hold_delay)) {
        output_garage_driver_mode = 0;
      } 
      break;
    case 2: // close by timer
      analogWrite(garage_output_pin, 25);
      if (garage_button_hold_tm.ready(garage_button_hold_delay)) {
        output_garage_driver_mode = 0;
      } 
      break;
    case 4: // open wicket by timer
      analogWrite(garage_output_pin, 230);
      if (garage_wicket_tm.ready(garage_wicket_delay)) {
        output_garage_driver_mode = 0;
      } 
      break;
  } 

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
      //if (left_gate.finished()) left_gate.set(0);
      //if (right_gate.finished()) right_gate.set(0);

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

byte read_buttons () {
  byte val = 0;
  byte output_val = 0;
  for (int i = 0; i < 4; i++) bitWrite(val, i, digitalRead(button_pin[i])); //1 - 2 - 4 - 8
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

byte button_emulator () {
  String text_val = Serial_read_data();
  if (text_val.indexOf("open") > -1) button_emulator_val = 1;
  else if (text_val.indexOf("close") > -1) button_emulator_val = 2;
  else if (text_val.indexOf("wicket") > -1) button_emulator_val = 4;
  else if (text_val.indexOf("stop") > -1) button_emulator_val = 8;
  if (button_emulator_t.ready(100)) {
    button_emulator_val = 0;
  }
  return button_emulator_val;
}
