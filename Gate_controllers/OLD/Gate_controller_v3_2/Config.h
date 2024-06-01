#include <EEPROM.h>
#include "MyTimer.h"

bool debug_serial = true;
long debug_serial_period = 250; // ms
my_timer debug_serial_t;
String data_str = "";
String prev_data_str;

byte here_pos = 0;

void here () {
  Serial.println("HERE_" + String(here_pos++));
}

String Serial_read_data (byte serial_num = 0) {
  String val = "";
  bool if_available = false;
  switch (serial_num) {
    case 0:
      if (Serial.available() && !if_available) {
        if_available = true;   
        while(Serial.available()) {
          delayMicroseconds(200);
          val += char(Serial.read());     
        }
        if (debug_serial) Serial.println("Ser_0--> " + val + " <---Ser_0");
      }
      break; 
  }    
  return val;
}

void serial_print (String val) {
  if(debug_serial) {
    debug_serial_t.timer(debug_serial_period);
    debug_serial_t.stat = true;
  if (debug_serial_t.ready()) {
      Serial.println(val);
      debug_serial_t.reset();
    }
    prev_data_str = val;
  }
}
