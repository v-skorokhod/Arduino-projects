#define LED_pin 8
#define input_pin 3
#define PWM_freq 500 // Hz
#define open_pin 4
#define close_pin 2
unsigned long duration;
#include "MyTimer.h";
int current_button, prev_current_button;
int button_pin[2] = {A0,A1}; // 
bool BUTTON_MODE = false;
svsTimer button_hold_tm;
long button_hold_delay = 30000; // ms (время удержания кнопки)
int output_driver_mode, prev_output_driver_mode;
long PWM_period_us;
byte prev_button_stat;
bool prev_button_mode;

svsTimer button_tm;
svsTimer LED_tm;
bool long_pressed = false;

int read_buttons () {
  int output_val = false;
  byte val = 0;
  for (int i = 0; i < 2; i++) bitWrite(val, i, !digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  if (val && !long_pressed) {
    long_pressed = true;
    button_tm.reset();
  }
  if (button_tm.ready(200) && long_pressed) {
    if (val) output_val = val;
    else output_val = 0;
    long_pressed = false;
    delay(100);
  }
  return output_val;  
}

void LED(bool val) {
  static bool LED_stat;
  if (LED_tm.ready(250)) LED_stat = !LED_stat;
  if (val) digitalWrite(LED_pin, LED_stat);
  else digitalWrite(LED_pin, LOW);
}

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
  if (current_button) {
    Serial.println(current_button);
    button_hold_tm.reset();
    if (BUTTON_MODE == true) {
      BUTTON_MODE = false;
      delay(100);
    } else {
      BUTTON_MODE = true;
      output_driver_mode = current_button;
      //delay(500);
    }
  }
  if (button_hold_tm.ready(button_hold_delay)) BUTTON_MODE = false;
  if (BUTTON_MODE == true) {
    if (output_driver_mode == 1) {        // OPEN
      digitalWrite(open_pin, HIGH);
      digitalWrite(close_pin, LOW);
      //digitalWrite(LED_pin, HIGH);
    } else if (output_driver_mode == 2) { // CLOSE
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, HIGH);
      //digitalWrite(LED_pin, HIGH);
    }
    LED(HIGH);
  } else {
    duration = pulseIn(input_pin, HIGH, PWM_period_us * 2); //timeout 10ms
    int val = 100 * duration / PWM_period_us; // %
    if (duration == 0) val = 50;
    if (val >= 80 && val < 95) {        // OPEN
      digitalWrite(open_pin, HIGH);
      digitalWrite(close_pin, LOW);
      LED(HIGH);
    } else if (val <= 20 && val > 5) {  // CLOSE
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, HIGH);
      LED(HIGH);
    } else {                            // STOP
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, LOW);
     LED(LOW);
    }
  }

  if (prev_button_mode != BUTTON_MODE) {
    Serial.println("CHANGES!");
    delay(1000);
  }
  prev_button_mode = BUTTON_MODE;
}
