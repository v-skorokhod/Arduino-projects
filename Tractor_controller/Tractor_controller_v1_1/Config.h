#pragma once
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MyTimer.h"
#include <Servo.h>
#include <Time.h>
#include <DS1307RTC.h>
tmElements_t tm;

#define START_SCREEN_DELAY 3000 //ms
bool DISLAY_DARK_STYLE;
#define MAIN_ID 0
#define POMP_ID 1
#define SEEDER_ID 2
int MODULE_ID;
const byte max_ID = 3;
const int max_outputs = 5;


#define buzzer_pin 7
int power_out_pin [max_outputs] = {4,5,6,7,3};
#define LED_pin A7
#define input_voltage_pin A0
#define input_current_pin A1
int pressure_pin [max_outputs] = {A2, A3, A4, -1, -1};
int press_num_map [max_outputs] = {1,2,0,3,4};
#define LM335_PIN A5

float max_pressure = 5.0; // atm
float max_flow = 90.0; // l/min
float max_RPM = 100.0; // RPM
float barrel_volume = 200.0; // l
int global_flow_input_num = 2; // 0..2

// ****************** EEPROM parameters ***************** //
struct Coef {
  float ref_voltage;
  float voltage;
  float current;
  float current_offset; // V
  float speed;
  float wheel_radius;
  float wheel_IPR;
  float LM335;
  float pressure_offset[max_outputs];
  float flow[max_outputs];
  float pressure[max_outputs];
};

Coef coef;

float set_float[max_outputs * 2];
byte EEPROM_param[20];

volatile unsigned long global_interrupt_impulses[3];
volatile unsigned long local_interrupt_impulses[3];
volatile unsigned long interrupt_period[3] = {2000000,2000000,2000000};
volatile unsigned long last_interrupt_period[3];


int ds18b20_pin [2] =  {3, 15};
#define TEMPERATURE_PRECISION 9
OneWire oneWire_0(ds18b20_pin[0]);// вход датчиков 18b20 
DallasTemperature ds_0(&oneWire_0);
OneWire oneWire_1(ds18b20_pin[1]);// вход датчиков 18b20 
DallasTemperature ds_1(&oneWire_1);
my_timer temp;
#define temp_request 5000
float engine_temp, outside_temp, MOSFET_temp;
#define MAX_MOSFET_TEMP 70
#define MAX_ENGINE_TEMP 90

float voltage () {
  float val = analogRead(input_voltage_pin) * coef.ref_voltage * coef.voltage / 1024; // V
  val = constrain(val,0.0,99.9);
  return val;
}

float current () {
  float val = ((float(analogRead(input_current_pin)) * coef.ref_voltage / 1024.0) - coef.current_offset) / coef.current ; // A
  val = constrain(val,0.0,99.9);
  return val;
}

float program_millis () {
  long val = millis() / 100;
  float out_val = float(val) / 10.0;
  return out_val;
}

void read_param_EEPROM () {
  int posEEPROM = 0;
  for (int i = 1; i < 20; i++) EEPROM_param[i] = EEPROM.read(posEEPROM++);
}

void write_param_EEPROM () {
  int posEEPROM = 0;
  for (int i = 1; i < 20; i++) EEPROM.write(posEEPROM++, EEPROM_param[i]);
}

void buzzer(int val = 100){
  tone(buzzer_pin, 1000, val);
}

void send_nextion_txt (String val_name, String cmd) {
  while (Serial3.available())Serial3.read();
  Serial3.print(val_name + ".txt=\"" + cmd + "\""+char(255)+char(255)+char(255));
}

float read_temp (int val = 0) { 
  float output_val;
  if (MODULE_ID == 0)  {
    ds_0.requestTemperatures(); 
    output_val = (ds_0.getTempCByIndex(val)); // 'C    
  } else {
    float analog_val = float(analogRead(LM335_PIN)) * coef.ref_voltage / 1024.0;
    output_val = analog_val / coef.LM335 - 273.15; // 'C  
  }                              
  //val = constrain(val,-99.9,999.9);
  return output_val;
}

void calculate_interrupt_period(unsigned long val, byte pos) {
  interrupt_period[pos] = val - last_interrupt_period[pos];
  last_interrupt_period[pos] = val;
}

void interrupt_read_0() {
  local_interrupt_impulses[0]++;
  global_interrupt_impulses[0]++;
  calculate_interrupt_period(micros(), 0);
}

void interrupt_read_1() {
  local_interrupt_impulses[1]++;
  global_interrupt_impulses[1]++;
  calculate_interrupt_period(micros(), 1);
}

void interrupt_read_2() {
  local_interrupt_impulses[2]++;
  global_interrupt_impulses[2]++;
  calculate_interrupt_period(micros(), 2);
}

String toStr (float input) {
  String val = String(input,1) + ";";
  return val;
}

String to_str (int val) {
  String out_val;
  if (val < 10) out_val = "0" + String(val);
  else out_val = String(val);
  return out_val;
}

String timeStr () {
  String val = to_str(tm.Hour) + ":" + to_str(tm.Minute);
  return val;
}

String dateStr () {
  String val = to_str(tm.Day) + "/" + to_str(tm.Month) + "/" + to_str(tmYearToCalendar(tm.Year));
  return val;
}
