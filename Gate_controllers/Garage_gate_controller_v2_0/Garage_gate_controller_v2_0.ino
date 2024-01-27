#define LED_pin A5
#define input_pin 3
#define PWM_freq 500 // Hz
#define open_pin 4
#define close_pin 5
unsigned long duration;
#include "MyTimer.h";
int current_button, prev_current_button;
int button_pin[2] = {A0,A1}; // 
bool BUTTON_MODE = false;
svsTimer button_hold_tm;
long button_hold_delay = 30000; // ms (время удержания кнопки)
int output_driver_mode, prev_output_driver_mode;
long PWM_period_us;

void setup() {
  pinMode(input_pin, INPUT);
  pinMode(open_pin, OUTPUT);
  pinMode(close_pin, OUTPUT);
  pinMode(LED_pin, OUTPUT);
  digitalWrite(open_pin, LOW);
  digitalWrite(close_pin, LOW);
  for (int i = 0; i < 2; i++) pinMode(button_pin[i], INPUT);
  Serial.begin(57600);
  Serial.println();
  Serial.println("###########################");
  Serial.println("Programm_init");
  Serial.println("Garage_gate_control._v2_0");
  Serial.println("###########################");
  delay(100);
  PWM_period_us = 1000000 / PWM_freq;
  Serial.println("PWM_period_us: " + String(PWM_period_us) + " us");
}
 
void loop() {
  prev_current_button = current_button; 
  current_button = read_buttons();
  Serial.println(current_button);
  if (current_button) {
    button_hold_tm.reset();
    if (BUTTON_MODE == true) {
      BUTTON_MODE = false;
      delay(500);
    } else {
      BUTTON_MODE = true;
      output_driver_mode = current_button;
      delay(500);
    }
  }
  if (button_hold_tm.ready(button_hold_delay)) BUTTON_MODE = false;
  if (BUTTON_MODE == true) {
    if (output_driver_mode == 1) {        // OPEN
      digitalWrite(open_pin, HIGH);
      digitalWrite(close_pin, LOW);
      digitalWrite(LED_pin, HIGH);
    } else if (output_driver_mode == 2) { // CLOSE
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, HIGH);
      digitalWrite(LED_pin, HIGH);
    }
  } else {
    duration = pulseIn(input_pin, HIGH, PWM_period_us * 2); //timeout 10ms
    int val = 100 * duration / PWM_period_us; // %
    if (duration == 0) val = 50;
    if (val >= 80 && val < 95) {        // OPEN
      digitalWrite(open_pin, HIGH);
      digitalWrite(close_pin, LOW);
      digitalWrite(LED_pin, HIGH);
    } else if (val <= 20 && val > 5) {  // CLOSE
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, HIGH);
      digitalWrite(LED_pin, HIGH);
    } else {                            // STOP
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, LOW);
      digitalWrite(LED_pin, LOW);
    }
  }
}

byte read_buttons () {
  byte val = 0;
  for (int i = 0; i < 2; i++) bitWrite(val, i, !digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  return val;  
}
