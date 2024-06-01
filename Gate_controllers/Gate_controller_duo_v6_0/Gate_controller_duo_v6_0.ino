String filenameIDE = "Gate_contr._duo_v6_0"; // ИЗМЕНЯТЬ ПОСЛЕ ИЗМЕНЕНИЯ НАЗВАНИЯ ФАЙЛА !!! ####################################################################
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
  //EEPROM.write(5,1);
  MODULE_ID = EEPROM.read(5);
  if (MODULE_ID == 0) {
    for (int i = 0; i < BUT_NUM; i++) button_pin[i] = m_button_pin[i];
    IO_DATA_pin = m_IO_DATA_pin;
    LED_pin = m_led_pin;
    main_gate_init();
  } else if (MODULE_ID == 1) {
    for (int i = 0; i < BUT_NUM; i++) button_pin[i] = g_button_pin[i];
    IO_DATA_pin = g_IO_DATA_pin;
    LED_pin = g_led_pin;
    garage_gate_init();
  }
  for (int i = 0; i < BUT_NUM; i++) pinMode(button_pin[i], INPUT);
  pinMode(IO_DATA_pin, INPUT);
  PWM_period_us = 1000000 / PWM_freq;
  Serial.println("PWM_period_us: " + String(PWM_period_us) + " us");
  Serial.println("###########################");
}

void loop() {
  refresh_IO_pin();
  if (MODULE_ID == 0) main_gate_loop();
  else if (MODULE_ID == 1) garage_gate_loop();
}
