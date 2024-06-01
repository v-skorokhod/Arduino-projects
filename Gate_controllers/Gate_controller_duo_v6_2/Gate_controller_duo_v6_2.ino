String filenameIDE = "Gate_contr._duo_v6_2"; // ИЗМЕНЯТЬ ПОСЛЕ ИЗМЕНЕНИЯ НАЗВАНИЯ ФАЙЛА !!! ####################################################################
const bool TEST = false;

#include "Config.h"
#include "MainGate.h"
#include "GarageGate.h"

void setup() {
  Serial.begin(57600);
  Serial.println("###########################");
  Serial.println("Programm_init");
  Serial.println(filenameIDE);
  delay(100);
  EEPROM.write(5,3);
  MODULE_ID = EEPROM.read(5);
  Serial.println("MODULE_ID: " + String(MODULE_ID));
  if (MODULE_ID == 0) {
    IO_DATA_pin = m_IO_DATA_pin;
    LED_pin = m_led_pin;
    main_gate_init();
  } else if (MODULE_ID == 1) {
    IO_DATA_pin = g_IO_DATA_pin;
    LED_pin = g_led_pin;
    garage_gate_init();
  } else { /// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST
    for (int i = 0; i < BUT_NUM; i++) m_buttons[i].init(m_button_pin[i], INPUT, HIGH);
  }
  pinMode(IO_DATA_pin, INPUT);
  PWM_period_us = 1000000 / PWM_freq;
  Serial.println("PWM_period_us: " + String(PWM_period_us) + " us");
  Serial.println("###########################");
}

void loop() {
  refresh_IO_pin();
  if (MODULE_ID == 0) main_gate_loop();
  else if (MODULE_ID == 1) garage_gate_loop();
  else {/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST/// TEST
    for (int i = 0; i < BUT_NUM; i++)  m_buttons[i].tick();
    for (int i = 0; i < BUT_NUM; i++) {
   
    if (m_buttons[i].press()) Serial.println("btn_" + String(i) + " press");
    if (m_buttons[i].hasClicks(1)) {
      Serial.println("btn_" + String(i) + " has 1 clicks");
      output_driver_mode = i;
      if (i == 0) send_IO_data(8);
      right_gate.reset_error();
      left_gate.reset_error();
      set_current_val = 0.0;
      start_stat = false;
      button_hold_tm.reset();
      wicket_tm.reset(); 
      gate_tm.reset(); 
    } else if (m_buttons[i].hasClicks(2)) {
      Serial.println("btn_" + String(i) + " has 2 clicks");
      if (i == 0) set_current_val = HC_OFFSET;
      else send_IO_data(i);
    }
  }
  }
}
