class output_controller {
  public:
  bool enable = false;
  
  output_controller(){};
  void main();
  void parameters_read(int, float);
  void init(int,int);
  String get_status();
  float pressure();
  
  private:
  Servo myservo;
  my_timer pid;
  float dp_coef = 0.1; //atm
  float df_coef = 0.1; //l/min
  int millis_coef = 500; //ms
  int set_mode;
  float set_power, set_pressure, set_flow, set_RPM, set_SERVO;
  float current_power, current_pressure, current_flow, current_RPM, current_SERVO;
  int output_pin;
  float output_power;
  unsigned long prev_millis;
  int press_pin;
  float press_offset, press_coef;

  float pressure_controller(float);
  int flow_controller(float);
  void output_check(float);
  
};

void output_controller::init(int pin, int pressure_sensor = -1) {
  output_pin = pin;
  press_pin = pressure_pin[pressure_sensor];
  press_offset = coef.pressure_offset[pressure_sensor];
  press_coef = coef.pressure[pressure_sensor];
}

float output_controller::pressure () {
  float val = -1.0;
  if (press_pin > 0) {
    val = (float(analogRead(press_pin)) * coef.ref_voltage / 1024.0 - press_offset) / press_coef; // V
    val = constrain(val,0.0,99.9);
    //Serial.println("PIN_" + String(press_pin) + ": " + String(analogRead(press_pin)) + " -> " + String(val) + "atm");
  }
  return val;
}

void output_controller::parameters_read(int mode, float input_val) {
  Serial.println("Set to pin_" + String(output_pin) + ": " + String(mode) + " | " + String(input_val));
  set_mode = int(mode);
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
    case 3: // RPM
      enable = true;
      set_RPM = input_val;
      set_RPM = constrain(set_RPM, 0.0, max_RPM); // RPM
      break;
    case 4: // SERVO
      enable = true;
      set_SERVO = input_val;
      set_SERVO = constrain(set_SERVO, 0.0, 100.0); // %
      break;
  }  
}

String output_controller::get_status () {
  String val = "";
  switch (set_mode) {
    case -1: // disable
      val = "-1;0.0;";
      break;
    case 0: // manual
      val = String(set_mode) + ";" + toStr(current_power);
      break;
    case 1: // pressure
      val = String(set_mode) + ";" + toStr(current_pressure);
      break;
    case 2: // flow
      val = String(set_mode) + ";" + toStr(current_flow);
      break;
    case 3: // RPM
      val = String(set_mode) + ";" + toStr(current_RPM);
      break;
    case 4: // SERVO
      val = String(set_mode) + ";" + toStr(current_SERVO);
      break;
  }
  return val;
}

void output_controller::main() {
  current_pressure = pressure();
  current_flow = -1;//val_flow;
  switch (set_mode) {
    case -1: // disable
      output_power = 0;
      break;
    case 0: // manual
      output_power = set_power;
      current_power = set_power;
      break;
    case 1: // pressure
      output_power += pressure_controller(current_pressure);
      break;
    case 2: // flow
      output_power += flow_controller(current_flow);
      break;
    case 3: // RPM
      enable = true;
      break;
    case 4: // servo
      output_power = set_SERVO;
      current_SERVO = set_SERVO;
      break;
  }
  output_check(output_power);
}

float output_controller::pressure_controller(float current_pressure) {
  float d_pressure = current_pressure - set_pressure;
  float out_val = 0;
  if (d_pressure > dp_coef) out_val = -0.2;
  else if (d_pressure < dp_coef) out_val = 0.2;
  return out_val;
}

int output_controller::flow_controller(float current_flow) {
  float d_flow = current_flow - set_flow;
  int out_val = 0;
  if (d_flow > df_coef) out_val = -1;
  else if (d_flow < df_coef) out_val = 1;
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
