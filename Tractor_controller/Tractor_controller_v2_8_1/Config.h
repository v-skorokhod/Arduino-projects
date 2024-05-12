#include "GyverFilters.h"

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

#define min_RPM_imp_PERIOD 45 // ms

#define START_SCREEN_DELAY 1500 //ms
bool DISLAY_DARK_STYLE;
#define MAIN_ID 0
#define POMP_ID 1
#define SEEDER_ID 2
int MODULE_ID;
const byte MAX_ID = 3;
const int max_outputs = 5;

#define BLACK 0
#define GREEN 896
#define RED 51200
#define BLUE 20
#define WHITE 65535
#define YELLOW 65520

float global_speed = 0;

#define buzzer_pin 7
int power_out_pin [max_outputs] = {4,5,6,7,3};
#define LED_pin A7
#define input_voltage_pin A0
#define input_current_pin A1
int pressure_pin [max_outputs] = {A2, A3, A4, -1, -1}; // FOR POMP MODULE
int RPM_num_sensor [max_outputs] = {0, -1, -1, -1, -1}; // 0 - D2, 1 - D3, 2 - D18 - FOR SEEDER MODULE 
int press_num_map [max_outputs] = {1,2,0,3,4};
#define LM335_PIN A5

float max_pressure = 10.0; // atm
float max_flow = 90.0; // l/min
float max_RPM = 100.0; // RPM
#define max_RPMeter 1.0 // rev per meter
#define min_RPMeter 0.1 // rev per meter
float barrel_volume = 200.0; // l
int global_flow_input_num = 2; // 0..3
int herbicide_flow_input_num = 1; // 0..3
int seeder_RPM_input_num = 0; // 0..3
int speed_RPM_input_num = 3; // 0..3
#define MIN_POWER_RPMETER 20 // % 0..100

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
  float RPM_holes[max_outputs];
};

Coef coef;

float set_float[max_outputs * 2];
byte EEPROM_param[20];

const int INTERRUPT_PERIOD_FILTER_LENGTH = 10;
const int interrupts_num = 4;
volatile unsigned long global_interrupt_impulses[interrupts_num];
volatile unsigned long local_interrupt_impulses[interrupts_num];
volatile unsigned long interrupt_period[interrupts_num];//, interrupt_period_micros[3];
volatile unsigned long buffer_val_interrupt_period[interrupts_num];
volatile unsigned long last_interrupt_period[interrupts_num];//, last_interrupt_period_micros[3];
volatile unsigned long interrupt_period_filter[interrupts_num][INTERRUPT_PERIOD_FILTER_LENGTH];
volatile int filter_pos[interrupts_num];

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
#define MAX_ENGINE_TEMP 95

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
  //for (int i = 1; i < 20; i++) EEPROM.write(posEEPROM++, EEPROM_param[i]);
}

void buzzer(int val = 100){
  tone(buzzer_pin, 1000, val);
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

void calculate_interrupt_period(unsigned long val, byte num) {
  interrupt_period[num] = val - last_interrupt_period[num];
  last_interrupt_period[num] = val;
}

  /*interrupt_period_micros[num] = val - last_interrupt_period_micros[num];
  interrupt_period[num] = interrupt_period_micros[num] / 1000;
  last_interrupt_period_micros[num] = val;
  last_interrupt_period[num] = last_interrupt_period_micros[num] / 1000;*/

void interrupt_read_0() {
  local_interrupt_impulses[0]++;
  global_interrupt_impulses[0]++;
  calculate_interrupt_period(millis(), 0);
}

void interrupt_read_1() {
  local_interrupt_impulses[1]++;
  global_interrupt_impulses[1]++;
  calculate_interrupt_period(millis(), 1);
}

void interrupt_read_2() {
  local_interrupt_impulses[2]++;
  global_interrupt_impulses[2]++;
  calculate_interrupt_period(millis(), 2);
}

void interrupt_read_3() {
  local_interrupt_impulses[3]++;
  global_interrupt_impulses[3]++;
  calculate_interrupt_period(millis(), 3);
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
