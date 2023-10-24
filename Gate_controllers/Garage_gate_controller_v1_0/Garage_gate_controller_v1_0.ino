#define LED_pin A5
#define input_pin 3
#define PWM_freq 500 // Hz
#define power_relay_pin 4
#define dir_output_pin 5
unsigned long duration;
#include "MyTimer.h";

byte prev_button_stat;
my_timer button_t;
bool double_pressed = false;
byte last_pressed_button;
int current_button, prev_current_button;
int button_pin[2] = {A0,A1}; // 
bool BUTTON_MODE = false;
my_timer button_hold_tm;
long button_hold_delay = 35000; // ms (время удержания кнопки)
int output_driver_mode, prev_output_driver_mode;

long PWM_period_us;

my_timer test_tm;

void setup() {
  pinMode(input_pin, INPUT);
  pinMode(power_relay_pin, OUTPUT);
  pinMode(dir_output_pin, OUTPUT);
  pinMode(LED_pin, OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(button_pin[i], INPUT);
  Serial.begin(57600);
  Serial.println();
  Serial.println("###########################");
  Serial.println("Programm_init");
  Serial.println("Garage_gate_controller_v1_1");
  Serial.println("###########################");
  delay(100);
  PWM_period_us = 1000000 / PWM_freq;
  Serial.println("PWM_period_us: " + String(PWM_period_us) + " us");
}
 
void loop() {
  prev_current_button = current_button;
  button_hold_tm.timer(button_hold_delay);  
  current_button = read_buttons();
  Serial.println(current_button);
  if (current_button) {
    if (BUTTON_MODE == true) {
      BUTTON_MODE = false;
      delay(500);
    } else {
      BUTTON_MODE = true;
      output_driver_mode = current_button;
      delay(500);
    }
  }
  if (button_hold_tm.ready()) BUTTON_MODE = false;

  if (BUTTON_MODE == true) {
    if (output_driver_mode == 1) {
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, LOW);
      digitalWrite(LED_pin, HIGH);
    } else if (output_driver_mode == 2) {
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, HIGH);
      digitalWrite(LED_pin, HIGH);
    }
  } else {
    duration = pulseIn(input_pin, HIGH, PWM_period_us * 2); //timeout 10ms
    int val = 100 * duration / PWM_period_us; // %
    if (duration == 0) val = 50;
    if (val >= 80 && val < 100) { // OPEN
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, LOW);
      digitalWrite(LED_pin, HIGH);
    } else if (val <= 20 && > 0) { // CLOSE
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, HIGH);
      digitalWrite(LED_pin, HIGH);
    } else { // STOP
      digitalWrite(power_relay_pin, HIGH);
      digitalWrite(dir_output_pin, HIGH);
      digitalWrite(LED_pin, LOW);
    }
  }
}

byte read_buttons () {
  byte val = 0;
  for (int i = 0; i < 2; i++) bitWrite(val, i, !digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  return val;  
}
