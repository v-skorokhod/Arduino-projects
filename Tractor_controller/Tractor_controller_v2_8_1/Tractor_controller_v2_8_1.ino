//#pragma once
#include "Config.h"
#include "Communication.h"
#include "Main_module.h"
#include "Control_unit.h"

String filenameIDE = "Tractor_controller_v2_8_1(12.05.24)"; // ИЗМЕНЯТЬ ПОСЛЕ ИЗМЕНЕНИЯ НАЗВАНИЯ ФАЙЛА !!!
String page_name;
// ########################################################### SETUP SETUP SETUP ################################################
void setup () {
  Serial.begin(57600);
  int bar_val = 10;
  int bar_step_delay = START_SCREEN_DELAY / 100;
  Serial.println("************************************* ");
  Serial.println("Current version: " + filenameIDE);
  //read_param_EEPROM();
  Serial2.begin(RS485_BAUD); //RS485 init
  pinMode(rs485_direction_pin, OUTPUT);
  RS485_char_delay_microseconds = 10000000 / RS485_BAUD;
  Serial.println("RS485 begin: " + String(RS485_BAUD) + " | char_lenth: " + String(RS485_char_delay_microseconds) + "us");
  //EEPROM.write(0, 2);
  MODULE_ID = EEPROM.read(0);
  MODULE_ID = constrain(MODULE_ID, 0, MAX_ID - 1);
  Serial.print("CONTROL UNIT (ID_" + String(MODULE_ID) + "): ");
  switch (MODULE_ID) {
    case 0: // main
      //EEPROM.write(1000, 0);
      //EEPROM.write(1001, 0);
      Serial.println("MAIN_MODULE");
      Serial3.begin(NEXTION_BAUD);
      GLOBAL_RANGE = global_range_EEPROM_read();
      Serial.println("GLOBAL RANGE (READ FROM EEPROM): " + String(GLOBAL_RANGE,1) + "km");
      buzzer(300);
      delay(100);
      page_name = "page 0";
      Serial3.print(page_name +char(255)+char(255)+char(255));
      send_nextion_txt("Prog_name", filenameIDE);
      Serial.println("NEXTION BEGIN: " + String(NEXTION_BAUD));
      DISLAY_DARK_STYLE = EEPROM.read(1);
      if (DISLAY_DARK_STYLE) Serial.println("DISPLAY DARK MODE");
      else Serial.println("DISPLAY LIGHT MODE");
      RTC.read(tm);
      Serial.println("RTC BEGIN");
      ds_0.begin();
      ds_0.setResolution(TEMPERATURE_PRECISION);
      engine_temp = read_temp(0);
      outside_temp = read_temp(1);
      for (int i = 0; i < 7; i++) {
        switch_mode[i].init(switch_pin[i][0],switch_pin[i][1],potentiometer_pin[i],switch_optional_mode[i],switch_min_manual_val[i]);
      } 
      pinMode(button_A, INPUT_PULLUP);
      pinMode(button_B, INPUT_PULLUP);
      pinMode(button_C, INPUT_PULLUP);
      pinMode(button_D, INPUT_PULLUP);
      pinMode(main_switch, INPUT);
      coef.voltage = 4.17; //4.2 \ 3.95
      coef.ref_voltage = 5.0;
      coef.current_offset = coef.ref_voltage / 2.0;
      coef.current = 0.1;
      coef.wheel_radius = 0.485;
      coef.wheel_IPR = 6;
      attachInterrupt(4, interrupt_read_3, RISING); //порт 19 \ был 2порт 
      while(millis() < START_SCREEN_DELAY) {
        send_nextion_val("p_bar", bar_val++);
        delay(bar_step_delay);
        }
      if (DISLAY_DARK_STYLE) page_name = "page 1";
      else page_name = "page 2";
      Serial3.print(page_name +char(255)+char(255)+char(255));
      send_nextion_val("p_bar", 100);
      delay(100);
      break;
    case 1: // pomp
      Serial.println("POMP_MODULE");
      MOSFET_temp = read_temp(0);
      coef.voltage = 4.17;
      coef.ref_voltage = 5.0; // V
      coef.current_offset = coef.ref_voltage / 2.0;
      coef.current = 0.040; // V/A
      coef.LM335 = 0.01; // V/'C
      coef.pressure[0] = 0.392; // V/atm
      coef.pressure[1] = 0.333; // V/atm
      coef.pressure[2] = 0.333; // V/atm
      coef.pressure_offset[0] = 0.5; // V
      coef.pressure_offset[1] = 0.5; // V
      coef.pressure_offset[2] = 0.5; // V
      coef.flow[0] = 100.0; // imp/L
      coef.flow[1] = 100.0; // imp/L
      coef.flow[2] = 485.0; // imp/L
      power_out_pin[4] = 0;
      
      for (int i = 0; i < max_outputs; i++) {
        output[i].init(power_out_pin[i], press_num_map[i]);
      }  
      pinMode(LED_pin, OUTPUT);
      attachInterrupt(0, interrupt_read_0, RISING); //порт 2  
      attachInterrupt(1, interrupt_read_1, RISING); //порт 3
      attachInterrupt(5, interrupt_read_2, RISING); //порт 18
      break;
    case 2: // seeder
      Serial.println("SEEDER_MODULE");
      MOSFET_temp = read_temp(0);
      coef.voltage = 4.21;
      coef.ref_voltage = 5.0; // V
      coef.current_offset = coef.ref_voltage / 2.0;
      coef.current = 0.040; // V/A
      coef.LM335 = 0.01; // V/'C
      coef.RPM_holes[0] = 13; // было 118
      coef.RPM_holes[2] = 13; // было 215
      for (int i = 0; i < max_outputs; i++) {
        output[i].init(power_out_pin[i], -1, RPM_num_sensor[i]);
      }  
      pinMode(LED_pin, OUTPUT);
      attachInterrupt(0, interrupt_read_0, FALLING); //порт 2  
      attachInterrupt(5, interrupt_read_2, FALLING); //порт 18
      break;
  }  
  //init_func();
  show_terminal_comands();
  Serial.println("********** DEVICE IS READY ********** ");
}
// ############################################################ LOOP LOOP LOOP ##################################################
void loop () {
  read_terminal_command();
  //if(debug_serial) Serial.println("test");
  ready_to_send_update();
  if (MODULE_ID == 0) main_func();
  else control_func();
}
