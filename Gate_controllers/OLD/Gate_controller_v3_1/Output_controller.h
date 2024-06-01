#include "Config.h"

class output_controller {
  public:
  bool enable = false;
  
  output_controller(){};
  void set(int);
  void init(int, int, int, int, int, String);  
  float position();
  void reset_error();
  bool finished();
  String get_feedback();
  void set_current(float);
  
  private:
  struct Coef {
    float ref_voltage = 5.0; // V
    float voltage;
    float current = 0.117; // V/A
    float current_offset = 0.0; // V
  };
  Coef coef;
  String name;
  int current_pin[2];
  int forward_pin;
  int reverse_pin;
  int end_switch_pin;
  byte EEPROM_cel;
  bool reverse_stat = false;
  bool error_stat = false;
  bool start_stat = false;
  bool prev_start_stat;
  float output_power; // %
  long position_time;
  long prev_position_time;
  int end_switch_val = 0;
  bool prev_end_switch_stat = false;
  float current_filter[10];
  byte filter_pos = 0;
  
  bool soft_start = true;
  bool soft_brake = true;
  int error_timeout = 30000; // ms
  int check_timeout = 500; // ms  
  long open_time = 25000; // ms (время открытия от концевика до концевика)
  int end_switch_timeout = 5000; // ms (задержка до обнуления состояния концевика если он не активен)
  int power_step_millis = 100; // ms
  float start_power_step = 10; // % per 100ms (power_step_millis) (шаг ускорения % за 100мс)
  float stop_power_step = 2; // % per 100ms (power_step_millis) (шаг торможения % за 100мс)
  float min_power = 20; // % (минимальная мощность с которой начинает расчет положения)
  float set_overcurrent_val = 30.0; // A (защита по току)
  float max_overcurrent_val = 60.0; // A (макс. ток защиты)
  float overcurrent_val;  
  byte OCP_reverse_power = 170; // 0..255 OVERCURRENT PROTECTION reverse power (мощность отъезжания назад при сработке защиты по току)
  int OCP_reverse_timer = 2000; // ms OVERCURRENT PROTECTION reverse timer
  float OCP_deadzone = 10.0; // % OVERCURRENT PROTECTION deadzone NO REVERSE
  float position_deadzone = 30.0; // % for end switches func 
  float EEPROM_save_step = 5.0; // %

  unsigned long prev_position_millis;
  my_timer error;
  my_timer check;
  my_timer power_change_step;
  my_timer end_switch_t;

  float read_current(); 
  void reverse(bool);
  void output_check(int);
  bool overcurrent_protection();
  int end_switch_stat();
  float output_pid(float, bool);
};

void output_controller::set_current(float val = 0.0) {
  overcurrent_val = set_overcurrent_val + val;
  overcurrent_val = constrain(overcurrent_val, 0.0, max_overcurrent_val);
}

void output_controller::init(int val_1, int val_2, int val_3, int val_4, int val_5, String val_6) {
  current_pin[0] = val_1;
  current_pin[1] = val_2;
  forward_pin = val_3;
  reverse_pin = val_4;
  end_switch_pin = val_5;
  name = val_6;
  pinMode(forward_pin, OUTPUT);
  pinMode(reverse_pin, OUTPUT);
  pinMode(end_switch_pin, INPUT);
  EEPROM_cel = forward_pin * 10;
  byte val = EEPROM.read(EEPROM_cel);
  position_time = val * open_time / 200; // ms
}

float output_controller::output_pid(float target, bool mode = false) {
  power_change_step.timer(power_step_millis);
  float pos_val = position();
  float pid_step = start_power_step;
  if (end_switch_stat()) {
    if (reverse_stat && position() >= position_deadzone) {
      target = 0.0;
      pid_step = stop_power_step;
      if (end_switch_stat() == 2) output_power = 0.0;
    } else if (!reverse_stat && position() <= (100 - position_deadzone)) {
      target = 0.0;
      pid_step = stop_power_step;
      if (end_switch_stat() == 2) output_power = 0.0;
    }
  }
  if(!power_change_step.stat) power_change_step.stat = true;
  if (mode) {
    if (power_change_step.ready()) {
      float d_target = target - output_power; d_target = abs(d_target);
      if (d_target < pid_step) {
        output_power = target;
      } else if (target > output_power) {
        output_power += pid_step;
      } else if (target < output_power) {
        output_power -= pid_step;
      }
      output_power = constrain(output_power, 0.0, 100.0); //%
      power_change_step.reset();
    } 
  } else {
    output_power = target; // %
  }
  output_check(output_power);
}

String output_controller::get_feedback () {
  String val = name + " >> position: " + String(position()) + "% - output_power: " + String(output_power) +"% - current: " + String(read_current()) + "A - set_overcurrent: " + String(overcurrent_val) + "A - end_switch: " + String(end_switch_stat());
  return val;
  
}

void output_controller::set(int direction_val) {
  if (direction_val != 0 && error_stat == false) {
    if (direction_val > 0) reverse_stat = true;
    else if (direction_val < 0) reverse_stat = false;
    prev_start_stat = start_stat;
    start_stat = true;  
    output_pid(100.0, soft_start);    
  } else {
    output_pid(0.0);
  }
}

float output_controller::read_current() {
  float val_0 = ((analogRead(current_pin[0]) * coef.ref_voltage / 1023.0) - coef.current_offset) / coef.current; // A
  float val_1 = ((analogRead(current_pin[1]) * coef.ref_voltage / 1023.0) - coef.current_offset) / coef.current; // A
  float val;
  if (val_0 > val_1) val = val_0;
  else val = val_1;
  current_filter[filter_pos++] = val;
  if (filter_pos >= 10) filter_pos = 0;
  float out_val = 0;
  for (int i = 0; i < 10; i++) {
    out_val += current_filter[i] / 10;
  }
  out_val = constrain(out_val,0.0,99.9);
  //out_val = 0.9;
  return out_val;
}

float output_controller::position() {
  float val = float(position_time) / float(open_time) * 100.0; // %
  if (end_switch_stat() && val < position_deadzone && !reverse_stat) {
    position_time = 0;
    if (prev_position_time != position_time) EEPROM.write(EEPROM_cel, byte(val * 2));
    prev_position_time = position_time;
  }
  long d_position_millis = millis() - prev_position_millis;
  if (d_position_millis >= 100 && output_power >= min_power) {
    if (reverse_stat) {
      position_time += d_position_millis * int(output_power) / 100;
    } else {
      position_time -= d_position_millis * int(output_power) / 100;
    }
    prev_position_millis = millis();
  } else if (output_power < min_power) {
    prev_position_millis = millis();
  }
  position_time = constrain(position_time, 0, open_time);
  if (prev_position_time != position_time) EEPROM.write(EEPROM_cel, byte(val * 2));
  val = float(position_time) / float(open_time) * 100.0; // %
  val = constrain(val, 0.0, 100.0); 
  prev_position_time = position_time;
  return val;
}

bool output_controller::overcurrent_protection() {
  if (read_current() > overcurrent_val && !error_stat) {
    error_stat = true;
    output_check(0);
    Serial.println(name + " >> OVERCURRENT PROTECTION!!!");
    if (position() >= OCP_deadzone) {
      //reverse(!reverse_stat);
      //analogWrite(output_pin, OCP_reverse_power);
      //delay(OCP_reverse_timer);
    }
    return true;
  } else {
    return false;
  }
}

void output_controller::reset_error() {
  error_stat = false;
}

bool output_controller::finished() {
  //error_stat = false;
}

int output_controller::end_switch_stat(){
  end_switch_t.timer(end_switch_timeout);
  bool end_switch_stat;
  end_switch_stat = !digitalRead(end_switch_pin);
  if (end_switch_stat) {
    if (end_switch_stat != prev_end_switch_stat) end_switch_val++;
    end_switch_t.reset();
  }
  if (end_switch_t.ready()) end_switch_val = 0;
  end_switch_val = constrain(end_switch_val, 0, 2);
  prev_end_switch_stat = end_switch_stat;
  return end_switch_val;
}

void output_controller::output_check(int input_val) {
  input_val = map(input_val, 0.0, 100.0, 0.0, 255.0); // % -> pwm (0..255)
  input_val = constrain(input_val, 0.0, 255.0); // %
  byte output_val = byte(input_val);
  if (overcurrent_protection() == false) {
    if (reverse_stat) {
      analogWrite(forward_pin, 0);
      analogWrite(reverse_pin, output_val);
    } else {
      analogWrite(forward_pin, output_val);
      analogWrite(reverse_pin, 0);
    }
  } else {
    analogWrite(forward_pin, 0);
    analogWrite(reverse_pin, 0);
  }
}
