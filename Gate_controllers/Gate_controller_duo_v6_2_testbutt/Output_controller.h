

class output_controller {
  public:
  bool enable = false;
  
  output_controller(){};
  void set(int);
  void init(int, int, int, int, String);  
  float position();
  void reset_error();
  bool error_check();
  bool finished();
  String get_feedback(bool);
  void set_current(float);
  
  private:
  struct Coef {
    float ref_voltage = 5.0; // V
    float voltage;
    float current = 0.1; // V/A
    float current_offset = 2.5; // V
  };
  Coef coef;
  String name;
  int current_pin[2];
  byte forward_pin;
  byte reverse_pin;
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
  float current_filter[20];
  byte filter_pos = 0;
  bool finished_stat = false;
  
  bool soft_start = true;
  bool soft_brake = true;
  #define error_timeout 30000 // ms
  #define check_timeout 500 // ms  
  #define open_time 25000 // ms (время открытия от концевика до концевика)
  #define end_switch_timeout 5000 // ms (задержка до обнуления состояния концевика если он не активен)
  #define power_step_millis 100 // ms
  #define start_power_step 10.0 // % per 100ms (power_step_millis) (шаг ускорения % за 100мс)
  #define stop_power_step 2.0 // % per 100ms (power_step_millis) (шаг торможения % за 100мс)
  #define min_power 20.0 // % (минимальная мощность с которой начинает расчет положения)

  #define set_overcurrent_val 3.0 // A (защита по току)
  #define max_overcurrent_val 6.0 // A (макс. ток защиты)
  float overcurrent_val;  
  #define OCP_reverse_power 250 // 0..255 OVERCURRENT PROTECTION reverse power (мощность отъезжания назад при сработке защиты по току)
  #define OCP_reverse_timer 1500 // ms OVERCURRENT PROTECTION reverse timer
  #define OCP_deadzone 10.0 // % OVERCURRENT PROTECTION deadzone NO REVERSE
  #define OCP_DELAY 2000 //ms (задержка включения защиты по току после включения)

  #define position_deadzone 70.0 // % for end switches func 
  #define EEPROM_save_step 5.0 // %
  bool set_changes = false;
  int prev_direction_val;

  unsigned long prev_position_millis;
  svsTimer error;
  svsTimer check;
  svsTimer power_change_step;
  svsTimer end_switch_t;
  svsTimer OCP_delay_tm;

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

void output_controller::init(int val_1, int val_3, int val_4, int val_5, String val_6) {
  current_pin[0] = val_1;
  forward_pin = val_3;
  reverse_pin = val_4;
  end_switch_pin = val_5;
  name = val_6;
  pinMode(forward_pin, OUTPUT);
  pinMode(reverse_pin, OUTPUT);
  pinMode(current_pin[0], INPUT);
  pinMode(current_pin[1], INPUT);
  pinMode(end_switch_pin, INPUT);
  //tone(forward_pin, 4000, 1000);
  EEPROM_cel = forward_pin * 10;
  byte val = EEPROM.read(EEPROM_cel);
  position_time = val * open_time / 200; // ms
}

float output_controller::output_pid(float target, bool mode = false) {
  float pos_val = position();
  float pid_step = start_power_step;
  if (end_switch_stat()) {
    if (reverse_stat && position() >= position_deadzone) {
      target = 0.0;
      pid_step = stop_power_step;
      if (end_switch_stat() == 2) {
        output_power = 0.0;
        finished_stat = true;
      }
    } else if (!reverse_stat && position() <= (100 - position_deadzone)) {
      target = 0.0;
      pid_step = stop_power_step;
      if (end_switch_stat() == 2) {
        output_power = 0.0;
        finished_stat = true;
      }
    }
  } 
  if (mode) {
    if (power_change_step.ready(power_step_millis)) {
      float d_target = target - output_power; d_target = abs(d_target);
      if (d_target < pid_step) {
        output_power = target;
      } else if (target > output_power) {
        output_power += pid_step;
      } else if (target < output_power) {
        output_power -= pid_step;
      }
      output_power = constrain(output_power, 0.0, 100.0); //%
    } 
  } else {
    output_power = target; // %
  }
  output_check(output_power);
}

String output_controller::get_feedback (bool LCD_mode = false) {
  String val = "";
  if (!LCD_mode) val = name + " >> pos: " + String(position()) + "% - pwr: " + String(output_power) +"% - curr: " + String(read_current()) + "A - set_OC: " + String(overcurrent_val) + "A - end_sw: " + String(end_switch_stat());
  else val = String(int(position())) + "|" + String(int(output_power)) + "%|" + String(read_current(),1) + "A|" + String(end_switch_stat());
  return val;
}

void output_controller::set(int direction_val) {
  if (prev_direction_val != direction_val) {
    set_changes = true;
    OCP_delay_tm.reset();
  }
  if (OCP_delay_tm.ready(OCP_DELAY)) set_changes = false;

  if (direction_val != 0 && !error_stat && !finished()) {
    if (direction_val > 0) reverse_stat = true;
    else if (direction_val < 0) reverse_stat = false;
    prev_start_stat = start_stat;
    start_stat = true;  
    output_pid(100.0, soft_start);    
  } else {
    output_pid(0.0);
  }
  prev_direction_val = direction_val;
}

float output_controller::read_current() {
  float val = ((analogRead(current_pin[0]) * coef.ref_voltage / 1023.0) - coef.current_offset) / coef.current; // A
  current_filter[filter_pos++] = val;
  if (filter_pos >= 20) filter_pos = 0;
  float out_val = 0;
  for (int i = 0; i < 20; i++) {
    out_val += current_filter[i] / 20;
  }
  out_val = constrain(out_val,0.0,99.9);
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
  if (read_current() > overcurrent_val && !error_stat && !set_changes) {
    error_stat = true;
    Serial.println(name + " >> OVERCURRENT PROTECTION!!! >> " + String(read_current()) + "A");
    if (position() >= OCP_deadzone) {
      reverse_stat = !reverse_stat;
      if (reverse_stat) {
        analogWrite(forward_pin, 0);
        analogWrite(reverse_pin, OCP_reverse_power);
      } else {
        analogWrite(forward_pin, OCP_reverse_power);
        analogWrite(reverse_pin, 0);
      }
      delay(OCP_reverse_timer);
    }
    return true;
  } else {
    return false;
  }
}

void output_controller::reset_error() {
  error_stat = false;
  finished_stat = false;
}

bool output_controller::error_check() {
  return error_stat;
}

bool output_controller::finished() {
  return finished_stat;
}

int output_controller::end_switch_stat(){
  bool end_switch_stat;
  end_switch_stat = !digitalRead(end_switch_pin);
  if (end_switch_stat) {
    if (end_switch_stat != prev_end_switch_stat) end_switch_val++;
    end_switch_t.reset();
  }
  if (end_switch_t.ready(end_switch_timeout)) end_switch_val = 0;
  end_switch_val = constrain(end_switch_val, 0, 2);
  prev_end_switch_stat = end_switch_stat;
  return end_switch_val;
}

void output_controller::output_check(int input_val) {
  input_val = map(input_val, 0, 100, 0, 255); // % -> pwm (0..255)
  input_val = constrain(input_val, 0, 255); // %
  byte output_val = byte(input_val);
  if (overcurrent_protection() == false && !finished()) {
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