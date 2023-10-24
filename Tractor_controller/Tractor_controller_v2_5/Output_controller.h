const int RPM_FILTER_LENGTH = 10;

class output_controller {
  public:
  bool enable = false;
  
  output_controller(){};
  void main();
  void parameters_read(int, float, int);
  void init(int,int,int);
  String get_status();
  
  private:
  GFilterRA pressure_filter;
  GMedian<10, float> RPM_median_filter; 
  GMedian<10, float> global_speed_median_filter; 
  Servo myservo;
  my_timer pid;
  my_timer RPM_filter;
  int PID_TIMER = 100; // ms
  int RPM_FILTER_TIMER = 20; // ms
  float PRESSURE_PID_VAL = 40.0; // % per s
  float RPM_PID_VAL = 3.0; // % per s
  float START_POWER_RPMeter = 50.0; // % start in auto RPM mode
  float dp_coef = 0.1; //atm
  float df_coef = 0.1; //l/min
  float dR_coef = 0.2; //RPM
  int millis_coef = 500; //ms
  int set_mode, optional_mode;
  float set_power, set_pressure, set_flow, set_RPM, set_SERVO, set_RPMeter;
  float current_power, current_pressure, current_flow, current_RPM, current_SERVO;
  int output_pin;
  float output_power;
  unsigned long prev_millis;
  int press_pin;
  float press_offset, press_coef;
  int RPM_sensor_num;
  float RPM_filter_val[RPM_FILTER_LENGTH];
  int filter_pos;
  

  float pressure_controller(float);
  float flow_controller(float);
  float RPM_controller(float);
  void output_check(float);

  float pressure();
  float flow();
  float RPM(bool);
  
};

void output_controller::init(int pin, int pressure_sensor = -1, int RMP_sensor = -1) {
  output_pin = pin;
  press_pin = pressure_pin[pressure_sensor];
  press_offset = coef.pressure_offset[pressure_sensor];
  press_coef = coef.pressure[pressure_sensor];
  RPM_sensor_num = RMP_sensor;
  pressure_filter.setCoef(0.5); // установка коэффициента фильтрации (0.0... 1.0). Чем меньше, тем плавнее фильтр
  pressure_filter.setStep(10); // установка шага фильтрации (мс). Чем меньше, тем резче фильтр
}

float output_controller::pressure () {
  float output_val = -1.0;
  int analog_val;
  if (press_pin > 0) {
    analog_val = pressure_filter.filteredTime(analogRead(press_pin));
    output_val = (float(analog_val) * coef.ref_voltage / 1024.0 - press_offset) / press_coef; // V
    output_val = constrain(output_val,0.0,99.9);
  }
  return output_val;
}

float output_controller::flow () {
  float output_val = -1.0;
  return output_val;
}

float output_controller::RPM (bool filter_stat = false) {
  static float prev_output_val;
  static float output_val = prev_output_val;
  static float buffer_val;
  if (RPM_sensor_num >= 0) {
    RPM_filter.timer(RPM_FILTER_TIMER);
    if (!RPM_filter.stat) {
      RPM_filter.stat = true;
    }
    if(interrupt_period[RPM_sensor_num] > 2) buffer_val = 60.0 / (float(interrupt_period[RPM_sensor_num]) * coef.RPM_holes[RPM_sensor_num] / 1000.0);  // RPM
    if (millis() - last_interrupt_period[RPM_sensor_num] >= 1000) buffer_val = 0.0;
    buffer_val = RPM_median_filter.filtered(buffer_val);
    if (RPM_filter.timer_ready()) {
      if (filter_stat ) {
        RPM_filter_val[filter_pos++] = buffer_val;
        if (filter_pos >= RPM_FILTER_LENGTH) filter_pos = 0;
      }
      output_val = 0;
      for (int i = 0; i < RPM_FILTER_LENGTH; i++) {
        output_val += RPM_filter_val[i] / RPM_FILTER_LENGTH;
      }
      output_val = constrain(output_val,0,77);
      prev_output_val = output_val;
      //Serial.println("RPM_" + String(output_pin) + ": " + String(local_interrupt_impulses[RPM_sensor_num]) + " | " + String(interrupt_period[RPM_sensor_num]) + " | " + String(buffer_val) + " | " + String(output_val));
      RPM_filter.timer_reset();
    }
  }
  return output_val;
}

void output_controller::parameters_read(int mode, float input_val, int opt_mode) {
  Serial_write_data("Set to pin_" + String(output_pin) + ": " + String(mode) + " | " + String(input_val) + " | " + String(opt_mode));
  set_mode = mode;
  optional_mode = opt_mode;
  switch (set_mode) {
    case -1: // disable
      enable = false;
      break;
    case 0: // manual
      enable = true;
      set_power = input_val;
      set_power = constrain(set_power, 0.0, 100.0); // %
      break;
    case 1: // pressure
      enable = true;
      set_pressure = input_val;
      set_pressure = constrain(set_pressure, 0.0, max_pressure); // atm
      break;
    case 2: // flow
      enable = true;
      set_flow = input_val;
      set_flow = constrain(set_flow, 0.0, max_flow); // l/min
      break;
    case 3: // Rev per meter
      enable = true;
      set_RPMeter = input_val;
      set_RPMeter = constrain(set_RPMeter, 0.0, max_RPMeter); // RPM
      break;
    case 4: // SERVO
      enable = true;
      set_SERVO = input_val;
      set_SERVO = constrain(set_SERVO, 0.0, 100.0); // %
      break;
  }  
}

String output_controller::get_status () {
  String val = "-1;0.0;-1";
  if (set_mode >= 0) val = String(set_mode) + ";" + toStr(current_power);
  switch (optional_mode) {
    case -1: // disable
      val += "-1;";
      break;
    case 0: // manual
      val += "-1;";
      break;
    case 1: // pressure
      val += String(pressure(),1);
      break;
    case 2: // flow
      val += "-1;";
      break;
    case 3: // Rev per meter
      val += String(RPM(),1);
      break;
    case 4: // servo
      val += "-1;";
      break;
  return val;
  }
}

void output_controller::main() { 
  float current_RPM = RPM(true);
  switch (set_mode) {
    case -1: // disable
      output_power = 0;
      break;
    case 0: // manual
      output_power = set_power;
      break;
    case 1: // pressure
      output_power += pressure_controller(pressure());
      break;
    case 2: // flow
      output_power += flow_controller(flow());
      break;
    case 3: // Rev per meter
      //global_speed = float(random(50)) / 10.0 + 2.0;
      //global_speed = 5.0;
      //global_speed = global_speed_median_filter.filtered(global_speed);
      static float prev_global_speed;
      if (prev_global_speed == 0.0 && global_speed >= 0.0) output_power = START_POWER_RPMeter;
      prev_global_speed = global_speed;
      //Serial.println(set_RPMeter);
      if (global_speed > 0.0) {
        set_RPM = set_RPMeter * global_speed * 16.7; 
        output_power += RPM_controller(current_RPM);
        output_power = constrain(output_power, MIN_POWER_RPMETER, 100);
      } else {
        set_RPM = 0.0;
        output_power = 0.0;
      }
      //Serial.println("global_speed: " + String(global_speed) + " | set_RPMeter: " + String(set_RPMeter) + " | set_RPM: " + String(set_RPM));
      
      break;
    case 4: // servo
      output_power = set_SERVO;
      current_SERVO = set_SERVO;
      break;
  }
  output_power = constrain(output_power, 0, 100);
  current_power = output_power;
  output_check(output_power);
}

float output_controller::pressure_controller(float input_pressure) {
  pid.timer(PID_TIMER);
  if (!pid.stat) {
    pid.stat = true;
  }
  float d_pressure = input_pressure - set_pressure;
  float out_val = 0.0;
  if (pid.timer_ready()) {
    out_val = (PRESSURE_PID_VAL / 1000.0 * PID_TIMER) * d_pressure; 
    out_val = constrain(out_val, -10, 2); //%
    //Serial.println("PID: " + String(out_val) + " d_pressure: " + String(d_pressure));
    if (abs(d_pressure) < dp_coef) out_val = 0;
    pid.timer_reset();
  }
  return -out_val;
}

float output_controller::flow_controller(float current_flow) {
  pid.timer(PID_TIMER);
  if (!pid.stat) {
    pid.stat = true;
  }
  //float d_flow = current_flow - set_flow;
  int out_val = 0;
  /*
  if (pid.timer_ready()) {
    float val = PID_VAL * 1000.0 / PID_TIMER; 
    if (d_flow > df_coef) out_val = -val;
    else if (d_flow < df_coef) out_val = val;
  }*/
  return -out_val;
}

float output_controller::RPM_controller(float input_RPM) {
  pid.timer(PID_TIMER);
  if (!pid.stat) {
    pid.stat = true;
  }
  float d_RPM = input_RPM - set_RPM;
  float out_val = 0.0;
  if (pid.timer_ready()) {
    out_val = (RPM_PID_VAL / 1000.0 * PID_TIMER) * d_RPM; // / max_RPM; 
    out_val = constrain(out_val, -10, 10); //%
    //Serial.println("PID: " + String(out_val) + " d_RPM: " + String(d_RPM));
    if (abs(d_RPM) < dR_coef) out_val = 0;
    pid.timer_reset();
  }
  return -out_val;
}

void output_controller::output_check(float input_val) {
  if (enable) {
    if (set_mode == 4) { // OUTPUT -> SERVO
      if(!myservo.attached()) {
        myservo.attach(output_pin);  // Указываем пин если этого не было сделано раньше
        myservo.writeMicroseconds(1000);
        delay(500);
      }
        input_val = map(input_val, 0, 100, 1000, 2000); //
        int output_val = constrain(input_val, 1000, 2000); // pwm ms
        myservo.writeMicroseconds(output_val);
    } else {
      if(myservo.attached()) {
        myservo.writeMicroseconds(1000);
        delay(100);
        myservo.detach();
      }
      input_val = map(input_val, 0, 100, 0, 255); // % -> pwm
      input_val = constrain(input_val, 0, 255); // pwm
      byte output_val = byte(input_val);
      analogWrite(output_pin, output_val);
    }
  } else {
    if(myservo.attached()) {
      myservo.detach();
    }
    analogWrite(output_pin, 0);
  }
}
