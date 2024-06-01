#define LED_pin 8
#define input_pin 3
#define PWM_freq 500 // Hz
#define power_relay_pin 2
#define dir_output_pin 4
unsigned long duration;
#include "MyTimer.h"

byte prev_button_stat;
my_timer button_t;
bool double_pressed = false;
byte last_pressed_button;
int current_button, prev_current_button;
int button_pin[2] = {A0,A1}; // 
bool BUTTON_MODE = false;
my_timer button_hold_tm;
my_timer LED_tm;
long button_hold_delay = 35000; // ms (время удержания кнопки)
int output_driver_mode, prev_output_driver_mode;

long PWM_period_us;

my_timer test_tm;
my_timer button_tm;
bool long_pressed = false;

int read_buttons () {
  int output_val = false;
  button_tm.timer(200);
  byte val = 0;
  for (int i = 0; i < 2; i++) bitWrite(val, i, !digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  if (val && !long_pressed) {
    long_pressed = true;
    button_tm.reset();
  }
  if (button_tm.ready() && long_pressed) {
    if (val) output_val = val;
    else output_val = 0;
    long_pressed = false;
    delay(100);
  }
  return output_val;  
}

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
  Serial.println("Garage_gate_controller_v1_4");
  Serial.println("###########################");
  delay(100);
  PWM_period_us = 1000000 / PWM_freq;
  Serial.println("PWM_period_us: " + String(PWM_period_us) + " us");
}

void LED(bool val) {
  static bool LED_stat;
  if (LED_tm.ready()) LED_stat = !LED_stat;
  if (val) digitalWrite(LED_pin, LED_stat);
  else digitalWrite(LED_pin, LOW);
}

bool prev_button_mode;
 
void loop() {
  LED_tm.timer(250);  
  prev_current_button = current_button;
  button_hold_tm.timer(button_hold_delay);  
  current_button = read_buttons();
  if (current_button) Serial.println(current_button);
  if (current_button) {
    button_hold_tm.reset();
    if (BUTTON_MODE == true) {
      BUTTON_MODE = false;
      //delay(500);
    } else {
      BUTTON_MODE = true;
      output_driver_mode = current_button;
      //delay(500);
    }
  }
  if (button_hold_tm.ready()) BUTTON_MODE = false;

  if (BUTTON_MODE == true) {
    if (output_driver_mode == 1) {
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, LOW);
    } else if (output_driver_mode == 2) {
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, HIGH);
    }
    LED(HIGH);
  } else {
    duration = pulseIn(input_pin, HIGH, PWM_period_us * 2); //timeout 10ms
    int val = 100 * duration / PWM_period_us; // %
    if (duration == 0) val = 50;
    if (val >= 80 && val < 95) { // OPEN
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, LOW);
      LED(HIGH);
    } else if (val <= 20 && val > 5) { // CLOSE
      digitalWrite(power_relay_pin, LOW);
      digitalWrite(dir_output_pin, HIGH);
      LED(HIGH);
    } else { // STOP
      digitalWrite(power_relay_pin, HIGH);
      digitalWrite(dir_output_pin, HIGH);
      LED(LOW);
    }
  }
  if (prev_button_mode != BUTTON_MODE) {
    Serial.println("CHANGES!");
    delay(1000);
  }
  prev_button_mode = BUTTON_MODE;
}
