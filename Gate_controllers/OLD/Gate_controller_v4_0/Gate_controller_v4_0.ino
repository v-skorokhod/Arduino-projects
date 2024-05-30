const bool TEST = false;

//#include "GSM.h";
#include "Output_controller.h"
#include "MyLED.h";
#define left_relay_pin 13
#define right_relay_pin 12
#define garage_output_pin 9
bool GARAGE_MODE = false;
output_controller left_gate;
output_controller right_gate;
int button_pin[4] = {6,5,4,2}; // open, close, wicket, stop (менять пины местами для соответсвия пульта)
int current_button, prev_current_button;
my_timer gate_tm;
#define open_gate_delay 2000 // ms (задержка открывания створок)
#define button_hold_delay 35000 // ms (время удержания кнопки)
#define wicket_delay 15000 // ms (время открытия калитки кнопки)
bool start_stat = false;
my_timer button_hold_tm;
my_timer wicket_tm;
float set_current_val = 0.0; // A
#define HC_OFFSET 3.0 // A (High Current mode offset) 
byte prev_button_stat;
my_timer button_tm;
bool double_pressed = false;
byte last_pressed_button;
#define DOUBLECLICK_TIMEOUT 300//ms

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
  Serial.println("Gate_controller_v4_0");
  Serial.println("###########################");
  delay(100);
  if (TEST) {
    //right_gate.init(A1,13,11,21, "TEST_R"); // пины для теста на отладочной плате, НЕ ТРОГАТЬ!!!!
    //left_gate.init(A7,10,12,21,"TEST_L"); // current_pin, output_pin, reverse_pin, end_switch_pin (пины левой створки)
    button_pin[0] = 50;
    button_pin[1] = 51;
    button_pin[2] = 52;
    button_pin[3] = 53;
    for (int i = 0; i < 4; i++) pinMode(button_pin[i], INPUT_PULLUP);
  } else {
    right_gate.init(A5,10,11,A2,"RIGTH"); // current_pin_1, current_pin_2, forward_pin, reverse_pin, end_switch_pin (пины правой створки)
    left_gate.init(A4,3,8,A3,"LEFT"); // current_pin_1, current_pin_2, forward_pin, reverse_pin, end_switch_pin (пины левой створки)
    for (int i = 0; i < 4; i++) pinMode(button_pin[i], INPUT);
  }
  pinMode(left_relay_pin, OUTPUT);
  pinMode(right_relay_pin, OUTPUT);
  pinMode(garage_output_pin, OUTPUT);
  pinMode(LED_pin, OUTPUT);
  //initGSM();
  //digitalWrite(left_relay_pin, LOW);
  //digitalWrite(right_relay_pin, LOW);
}

void loop() {
  here_pos = 0;
  prev_current_button = current_button;
  //current_button = button_emulator(); // <---------------------раскомментировать чтобы работало с консоли (либо то, либо то) 
  current_button = read_buttons(); // <---------------------раскомментировать чтобы работало с пульта (либо то, либо то) 
  output_driver();
  serial_print(output_driver_mode_str + " >>> " + String(button_hold_tm.time_left()) + " * \r\n" + right_gate.get_feedback() + "\r\n" + left_gate.get_feedback() + "\r\n ********************");
}

void output_driver () {
  gate_tm.timer(open_gate_delay);
  button_hold_tm.timer(button_hold_delay);
  wicket_tm.timer(wicket_delay);
  
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
      GARAGE_MODE = true;
    } else {
      output_driver_mode = current_button;
      set_current_val = 0.0;
    }
    start_stat = false;
    button_hold_tm.reset();
  } 
  right_gate.set_current(set_current_val);
  left_gate.set_current(set_current_val);
  
  
  //else set_LED_color(0);

  switch (output_driver_mode) {
    case 0: // stop
      digitalWrite(left_relay_pin, HIGH);
      digitalWrite(right_relay_pin, HIGH);
      right_gate.set(0);
      left_gate.set(0);
      analogWrite(garage_output_pin, 128);
      start_stat = false;
      right_gate.reset_error();
      left_gate.reset_error();
      gate_tm.reset();
      button_hold_tm.reset();
      wicket_tm.reset();
      output_driver_mode_str = "* STOPPED"; 
      if (GARAGE_MODE == true) set_LED_color(1);
      else set_LED_color(0);
      break;
   case 1: // open by timer
      if (GARAGE_MODE == false) {
        digitalWrite(left_relay_pin, LOW);
        digitalWrite(right_relay_pin, LOW);
      } else {
        analogWrite(garage_output_pin, 230);
      }
      right_gate.set(100);
      if (gate_tm.ready()) start_stat = true; // left gate delay 
      if (start_stat) left_gate.set(100); 
      else left_gate.set(0);
      if (button_hold_tm.ready()) {
        output_driver_mode = 0;
        GARAGE_MODE = false;
      } 
      if (set_current_val) output_driver_mode_str = "* OPEN_HC"; 
      else output_driver_mode_str = "* OPEN"; 
      if (left_gate.error_check() || right_gate.error_check()) set_LED_color(3); 
      else set_LED_color(2);
      break;
    case 2: // close by timer
      if (GARAGE_MODE == false) {
        digitalWrite(left_relay_pin, LOW);
        digitalWrite(right_relay_pin, LOW);
      } else {
        analogWrite(garage_output_pin, 25);
      }
      if (right_gate.position() >= (left_gate.position() + 30.0) || left_gate.position() == 0) {
        left_gate.set(-100);
        if (gate_tm.ready()) start_stat = true; // right gate delay 
        if (start_stat) right_gate.set(-100); 
        else right_gate.set(0);
      } else {
        left_gate.set(-100);
        right_gate.set(0);
      }
      if (button_hold_tm.ready() || (left_gate.finished() && right_gate.finished())) {
        output_driver_mode = 0;
        GARAGE_MODE = false;
      } 
      if (set_current_val) output_driver_mode_str = "* CLOSE_HC"; 
      else output_driver_mode_str = "* CLOSE";
      if (left_gate.error_check() || right_gate.error_check()) set_LED_color(3); 
      else set_LED_color(2); 
      break;
    case 4: // open rigth gate by timer
      if (GARAGE_MODE == false) {
        digitalWrite(right_relay_pin, LOW);
      } else {
        analogWrite(garage_output_pin, 230);
      }
      right_gate.set(100);
      if (wicket_tm.ready()) {
        output_driver_mode = 0;
        GARAGE_MODE = false;
      } 
      if (set_current_val) output_driver_mode_str = "* WICKET_HC";  
      else output_driver_mode_str = "* WICKET";
      if (left_gate.error_check() || right_gate.error_check()) set_LED_color(3); 
      else set_LED_color(2);
      break;
    case 8: // stop everything
      output_driver_mode = 0;
      output_driver_mode_str = "* STOPPING";
      GARAGE_MODE = false;
      break;
  }  
}

byte read_buttons () {
  byte val = 0;
  byte output_val = 0;
  button_tm.timer(DOUBLECLICK_TIMEOUT);
  for (int i = 0; i < 4; i++) bitWrite(val, i, digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  if (val) {
    if (last_pressed_button == val && prev_button_stat == 0) double_pressed = true;
    else button_tm.reset();
    last_pressed_button = val;
  }
  if (button_tm.ready()) {
    if (double_pressed) output_val = last_pressed_button * 10;
    else output_val = last_pressed_button;
    double_pressed = false;
    last_pressed_button = 0;
  }
  prev_button_stat = val;
  return output_val;  
}

byte button_emulator () {
  button_emulator_t.timer(100);
  String text_val = Serial_read_data();
  if (text_val.indexOf("open") > -1) button_emulator_val = 1;
  else if (text_val.indexOf("close") > -1) button_emulator_val = 2;
  else if (text_val.indexOf("wicket") > -1) button_emulator_val = 4;
  else if (text_val.indexOf("stop") > -1) button_emulator_val = 8;
  if (button_emulator_t.ready()) {
    button_emulator_val = 0;
  }
  return button_emulator_val;
}
