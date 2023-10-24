const bool TEST = false;

#include "Output_controller.h"
output_controller left_gate;
output_controller right_gate;
int button_pin[4] = {4,5,6,7}; // open, close, wicket, reserved (менять пины местами для соответсвия пульта)
int current_button, prev_current_button;
my_timer gate;
long open_gate_delay = 2000; // ms (задержка открывания створок)
long button_hold_delay = 35000; // ms (время удержания кнопки)
long wicket_delay = 15000; // ms (время открытия калитки кнопки)
bool start_stat = false;
my_timer button_hold_t;
my_timer wicket_t;
float set_current_val = 0.0; // A
#define HC_OFFSET 3.0 // A (High Current mode offset) 
byte prev_button_stat;
my_timer button_t;
bool double_pressed = false;
byte last_pressed_button;

byte button_emulator_val = 0;
bool button_emulator_stat = false;
my_timer button_emulator_t;
int output_driver_mode, prev_output_driver_mode;
String output_driver_mode_str;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("###########################");
  Serial.println("Programm_init");
  Serial.println("Gate_controller_v2_0");
  Serial.println("###########################");
  delay(100);
  if (TEST) {
    right_gate.init(A1,13,11,21, "TEST_R"); // пины для теста на отладочной плате, НЕ ТРОГАТЬ!!!!
    left_gate.init(A7,10,12,21,"TEST_L"); // current_pin, output_pin, reverse_pin, end_switch_pin (пины левой створки)
    button_pin[0] = 50;
    button_pin[1] = 51;
    button_pin[2] = 52;
    button_pin[3] = 53;
    for (int i = 0; i < 4; i++) pinMode(button_pin[i], INPUT_PULLUP);
  } else {
    right_gate.init(A6,9,11,A2,"RIGTH"); // current_pin, output_pin, reverse_pin, end_switch_pin (пины правой створки)
    left_gate.init(A7,10,12,A3,"LEFT"); // current_pin, output_pin, reverse_pin, end_switch_pin (пины левой створки)
    for (int i = 0; i < 4; i++) pinMode(button_pin[i], INPUT);
  }
  
}

void loop() {
  here_pos = 0;
  prev_current_button = current_button;
  //current_button = button_emulator(); // <---------------------раскомментировать чтобы работало с консоли (либо то, либо то) 
  current_button = read_buttons(); // <---------------------раскомментировать чтобы работало с пульта (либо то, либо то) 
  output_driver();
  serial_print(output_driver_mode_str + " >>> " + String(button_hold_t.time_left()) + " * \r\n" + right_gate.get_feedback() + "\r\n" + left_gate.get_feedback() + "\r\n **************************************");
}

void output_driver () {
  gate.timer(open_gate_delay);
  button_hold_t.timer(button_hold_delay);
  wicket_t.timer(wicket_delay);
  
  if (current_button) {
    if (current_button == 10) {
      output_driver_mode = 1;
      set_current_val = HC_OFFSET;
      right_gate.reset_error();
      left_gate.reset_error();
    } else if (current_button == 20) {
      output_driver_mode = 2;
      set_current_val = HC_OFFSET;
      right_gate.reset_error();
      left_gate.reset_error();
    } else if (current_button == 40) {
      output_driver_mode = 4;
      set_current_val = HC_OFFSET;
      right_gate.reset_error();
      left_gate.reset_error();
    } else if (current_button == 80) {
      output_driver_mode = 8;
    } else {
      output_driver_mode = current_button;
      set_current_val = 0.0;
    }
    button_hold_t.stat = false;
    gate.stat = false;
    start_stat = false;
  } 
  if (current_button && !gate.stat) {
    gate.stat = true;
    gate.reset();
  }
  if (current_button && !button_hold_t.stat) {
    button_hold_t.stat = true;
    button_hold_t.reset();
    wicket_t.stat = true;
    wicket_t.reset();
  }
  right_gate.set_current(set_current_val);
  left_gate.set_current(set_current_val);
  
  switch (output_driver_mode) {
    case 0: // stop
      right_gate.set(0);
      left_gate.set(0);
      start_stat = false;
      gate.stat = false;
      right_gate.reset_error();
      left_gate.reset_error();
      output_driver_mode_str = "* STOPPED"; 
      button_hold_t.stat = false;
      //set_current_val = 0.0;
      break;
    case 1: // open by timer
      right_gate.set(100);
      if (gate.ready()) start_stat = true; // left gate delay 
      if (start_stat) left_gate.set(100); 
      else left_gate.set(0);
      if (button_hold_t.ready()) {
        output_driver_mode = 0;
        button_hold_t.stat = false;
      } 
      if (set_current_val) output_driver_mode_str = "* OPEN_HC"; 
      else output_driver_mode_str = "* OPEN"; 
      break;
    case 2: // close by timer
      if (right_gate.position() >= left_gate.position()) {
        left_gate.set(-100);
        if (gate.ready()) start_stat = true; // right gate delay 
        if (start_stat) right_gate.set(-100); 
        else right_gate.set(0);
      } else {
        left_gate.set(-100);
      }
      if (button_hold_t.ready()) {
        output_driver_mode = 0;
        button_hold_t.stat = false;
      } 
      if (set_current_val) output_driver_mode_str = "* CLOSE_HC"; 
      else output_driver_mode_str = "* CLOSE"; 
      break;
    case 4: // open rigth gate by timer
      right_gate.set(100);
      if (wicket_t.ready()) {
        output_driver_mode = 0;
        wicket_t.stat = false;
      } 
      if (set_current_val) output_driver_mode_str = "* WICKET_HC";  
      else output_driver_mode_str = "* WICKET"; 
      break;
    case 8: // stop everything
      output_driver_mode = 0;
      output_driver_mode_str = "* STOPPING"; 
      break;
  }  
}

byte read_buttons () {
  byte val = 0;
  byte output_val = 0;
  button_t.timer(500);
  if (TEST) for (int i = 0; i < 4; i++) bitWrite(val, i, !digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  else for (int i = 0; i < 4; i++) bitWrite(val, i, digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  if (val) last_pressed_button = val;
  if (button_t.stat == true && val) double_pressed = true;
  if (prev_button_stat > val && !button_t.stat) {
    button_t.stat = true;
    button_t.reset();
  }
  //Serial.println(button_t.time_left());
  if (button_t.ready()) {
    button_t.stat = false;
    if (double_pressed) output_val = last_pressed_button * 10;
    else output_val = last_pressed_button;
    double_pressed = 0;
    last_pressed_button = 0;
  }
  prev_button_stat = val;
  //Serial.println("out: " + String(output_val)); delay(100);
  return output_val;  
}

byte button_emulator () {
  button_emulator_t.timer(100);
  String text_val = Serial_read_data();
  if (text_val.indexOf("open") > -1) button_emulator_val = 1;
  else if (text_val.indexOf("close") > -1) button_emulator_val = 2;
  else if (text_val.indexOf("wicket") > -1) button_emulator_val = 4;
  else if (text_val.indexOf("stop") > -1) button_emulator_val = 8;
  if (button_emulator_val > 0 && !button_emulator_t.stat) {
    button_emulator_t.stat = true;
    button_emulator_t.reset();
  }
  if (button_emulator_t.ready()) {
    button_emulator_val = 0;
    button_emulator_t.stat = false;
  }
  return button_emulator_val;
}
