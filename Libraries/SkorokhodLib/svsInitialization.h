#pragma once

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
String data_line[4];

void SerialInit (String name, long speed = 57600) {
  Serial.begin(speed);
  delay(10);
  Serial.println();
  Serial.println("#################################################");
  Serial.println(name);
  Serial.println("#################################################");
}

void LCDInit (String name, int val = 0) {
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0,1);
  if (val == 0) lcd.print(" aOrion.aero 2023");
  else if (val == 1) lcd.print("Skorokhod&Co 2023");
  lcd.setCursor(0,2); lcd.print(name); 
  lcd.clear();
}