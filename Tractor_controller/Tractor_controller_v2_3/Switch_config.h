int switch_pin [7][2] = {
    {4, 5}, // K1
    {6, A7}, // K2
    {9, 10}, // K3
    {11, 12}, // K4 11 12
    {24, 23}, // K5 22 23
    {19, 0}, // K6
    {18, 0}, // K7
  };
int switch_optional_mode [7] = {
  1,
  1,
  1,
  3,
  4,
  -1,
  -1,
};
int potentiometer_pin [7] = {
  A2,
  A3,
  A4,
  A5,
  A6,
  -1,
  -1,
};
int switch_config [7][2] = { //  module_ID 1..2, output 0..5
    {1, 2}, // K1 
    {1, 0}, // K2
    {1, 1}, // K3
    {2, 0}, // K4
    {2, 4}, // K5
    {1, 3}, // K6
    {2, 1}, // K7
};
String switch_name [7] = {
  "POMP_L",
  "POMP_R",
  "HERBICIDE",
  "SEEDER",
  "VENTIL.",
  "LAMP",
  "Reserved",
};

int switch_min_manual_val [7] = {
  10,
  10,
  10,
  30,
  0,
  0,
  0,
};

#define button_A 50
#define button_B 51
#define button_C 52
#define button_D 53
#define main_switch 13

bool main_switch_stat () {
  bool val = digitalRead(main_switch);
  return val;
}

class Switch_mode {
  public:
  bool enable = false;
  
  Switch_mode(){};
  void init(int,int,int,int,int);
  String get_status();
  String mode_str();
  float feedback_val;
  String feedback_parameter();
  bool get_changes();
  void set_feedback(int, int, float);
  unsigned int color();
  
  private:
  String Mode_str = "";
  int input_pin_1, input_pin_2, analog_pin, optional_mode;
  int analog_val;
  float set_parameter;
  int set_mode;
  int min_manual_val;
  bool pin_1_stat, pin_2_stat, changes_val;
  bool prev_pin_1_stat, prev_pin_2_stat, prev_main_stat;
  int prev_analog_val;
  struct Feedback {
    int power;
    int mode;
    float optional_val;
  };
  Feedback feedback;
};

void Switch_mode::init (int pin_1, int pin_2, int pin_3, int mode, int min_man_val = 0) {
  input_pin_1 = pin_1;
  input_pin_2 = pin_2;
  analog_pin = pin_3;
  optional_mode = mode;
  min_manual_val = min_man_val;
  pinMode(input_pin_1, INPUT_PULLUP);
  pinMode(input_pin_2, INPUT_PULLUP);
  pinMode(analog_pin, INPUT);
}

String Switch_mode::mode_str () {
  String output_val = Mode_str;
  return output_val;
}


unsigned int Switch_mode::color () {
  unsigned int output_val;
  if ((main_switch_stat() && set_parameter) || (input_pin_2 == 0 && set_parameter)) {
    output_val = RED;
  } else if (set_parameter && set_mode) {
    output_val = GREEN;
  } else if (set_parameter && !set_mode) {
    output_val = BLUE;
  } else {
    if (DISLAY_DARK_STYLE) output_val = BLACK;
    else output_val = YELLOW;
    if (!input_pin_2) output_val = BLACK;
  }
  return output_val;
}

void Switch_mode::set_feedback (int input_mode, int input_power, float input_opt_val) {
  feedback.mode = input_mode;
  feedback.power = input_power;
  feedback.optional_val = input_opt_val;
}

String Switch_mode::feedback_parameter () {
  String output_val = "";
  switch (optional_mode) {
    case -1: // disable
      //output_power = 0;
      break;
    case 0: // manual
      break;
    case 1: // pressure
      if (feedback.optional_val > 0.0) output_val += String(feedback.optional_val,1) + "atm\r\n    ";
      break;
    case 2: // flow
      //val = String(feedback_val,1) + "\r\nl/min";
      break;
    case 3: // RPM
      if (feedback.optional_val > 0.0) output_val += String(feedback.optional_val,0) + "RPM\r\n    ";
      break;
    case 4: // SERVO
      //val = String(int(feedback_val)) + "%";
      break;
  }
  
  if(main_switch_stat()) output_val += String(feedback.power) + "%";
  else output_val = "---";
  return output_val;
}

String Switch_mode::get_status () {
  analog_val = analogRead(analog_pin);
  int a_val = analog_val;
  analog_val = map(analog_val, 0, 1023, 100, 0); // %
  if (analog_val < 2) analog_val = 0;
  pin_1_stat = !digitalRead(input_pin_1);
  bool main_switch_enable = true;
  if(input_pin_2) {
    pin_2_stat = !digitalRead(input_pin_2);
  } else {
    pin_2_stat = false;
    pin_1_stat = !pin_1_stat;
    analog_val = 100;
    main_switch_enable = false;
  }
  int d_analog_val = prev_analog_val - a_val;
  if (prev_pin_1_stat != pin_1_stat) changes_val = true;
  else if (prev_pin_2_stat != pin_2_stat) changes_val = true;
  else if (abs(d_analog_val) > 5) changes_val = true;
  else if (main_switch_stat() != prev_main_stat) changes_val = true;
  if (pin_1_stat) {
    set_mode = 0;
    set_parameter = 0;
    Mode_str = "Offed";
  } else if (pin_2_stat) {
    set_mode = optional_mode;
    switch (set_mode) {
      case -1: // disable
        enable = false;
        set_parameter = -1.0;
        Mode_str = "Disabled";
        break;
      case 1: // pressure
        enable = true;
        set_parameter = float(map(analog_val, 0, 100, 0, max_pressure * 10))/10.0; // atm
        set_parameter = constrain(set_parameter, 0, max_pressure); //atm
        Mode_str = "Press(" + String(set_parameter, 1) + ")";
        break;
      case 2: // flow
        enable = true;
        set_parameter = map(analog_val, 0.0, 100.0, 0.0, max_flow); // l/min
        set_parameter = constrain(set_parameter, 0.0, max_flow); // l/min
        Mode_str = "Flow(" + String(set_parameter, 1) + ")";
        break;
      case 3: // RPM
        enable = true;
        set_parameter = map(analog_val, 0.0, 100.0, 0.0, max_RPMeter * 100); // RPmeter
        set_parameter = constrain(set_parameter, 0.0, max_RPMeter * 100); // RPmeter
        set_parameter /= 100.0;
        Mode_str = "o/m(" + String(set_parameter,2) + ")";
        break;
      case 4: // SERVO
        set_parameter = analog_val;
        set_parameter = constrain(set_parameter, 0, 100); // %
        Mode_str = "SERVO(" + String(set_parameter,0) + ")";
        break;
    }
  } else {
    set_mode = 0;
    set_parameter = analog_val;
    set_parameter = map(set_parameter, 0, 100, min_manual_val, 100);
    set_parameter = constrain(set_parameter, 0, 100); // %
    if(input_pin_2) Mode_str = "Man(" + String(set_parameter, 0) + ")";  
    else Mode_str = "ON";  
  }
  String output_val = String(set_mode) + ";" + String(set_parameter,1) + ";" + String(optional_mode) + ";";
  if (!main_switch_stat() && main_switch_enable) output_val = "0;0.0;" + String(optional_mode) + ";";
  prev_pin_1_stat = pin_1_stat;
  prev_pin_2_stat = pin_2_stat;
  prev_analog_val = a_val;
  prev_main_stat = main_switch_stat();
  return output_val;
}

bool Switch_mode::get_changes () {
  bool out_val = changes_val;
  changes_val = false;
  return out_val;
}

byte read_buttons(){
  int but_stat = 0;
  bitWrite(but_stat, 0, !digitalRead(button_A));  // 1    000001
  bitWrite(but_stat, 1, !digitalRead(button_B));  // 2    000010
  bitWrite(but_stat, 2, !digitalRead(button_C));  // 4    000100
  bitWrite(but_stat, 3, !digitalRead(button_D));  // 8    000100
  delay(1);
  //if (but_stat && but_stat < 15) Serial.println(but_stat);
  return but_stat; // 
}
