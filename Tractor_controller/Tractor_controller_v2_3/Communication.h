bool debug_serial = false;

#define RS485_BAUD 115200
long RS485_char_delay_microseconds = 1;
#define MAX_RS485_DELAY 50

String system_index = "system/";
String ready_index = "ready";
String start_index = "ID";
String end_index = "/re";
String no_resend_index = "/end";
String send_period_index = "send_period";
String RS485_delay_index = "send_delay";
String debug_serial_index = "debug_serial";
String reset_index = "reset";
String reset_volume_index = "reset_volume";
String reset_rev_index = "reset_rev";
String inf_index = "inf/";
String parameters_index = "param/";
String status_index = "status/";
String get_index = "get/";
String control_index = "ctrl/";
String command_index = "command/";
String offline_index = "offline";
String port_index = "_port/";

int RS485_send_delay = 10;
int send_period;
#define REQUEST_PERIOD 100
#define rs485_serial 2
#define rs485_direction_pin 8

unsigned long last_millis, send_millis, request_millis, ready_to_send_millis;

bool ready_to_send = false;

void Serial_write_data (String val, byte serial_num = 0) {
  switch (serial_num) {
    case 0:
      if(debug_serial) Serial.println(val);
      break;
    case 1:
      Serial1.print(val);
      break;
    case 2:
      Serial2.print(val);
      break;
    case 3:
      Serial3.print(val);
      break;
  }  
}

bool ready_to_send_update () {
  if (millis() - ready_to_send_millis >= RS485_send_delay && ready_to_send) {
    ready_to_send = false;
    digitalWrite(rs485_direction_pin, LOW);
  }
  return !ready_to_send;
}

void send_data_to_RS485 (String val) {
  int length_val = val.length();
  float float_val = float(length_val * RS485_char_delay_microseconds) * 1.5;  ////// ddksfds
  RS485_send_delay = int(float_val / 1000.0);
  RS485_send_delay = constrain(RS485_send_delay, 1, MAX_RS485_DELAY);
  ready_to_send = true;
  Serial_write_data("*******************************************************************");
  Serial_write_data("request to RS485 send: " + val + " | delay: " + String(RS485_send_delay) + "ms");
  digitalWrite(rs485_direction_pin, HIGH);
  delay(1);
  Serial_write_data(val, rs485_serial);
  ready_to_send_millis = millis();
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
    case 1:
      if (Serial1.available() && !if_available) {
        if_available = true; 
        while(Serial1.available()) {
          delayMicroseconds(200);
          val += char(Serial1.read());     
        }
        if (debug_serial && val.length() > 2) Serial.println("Ser_1---> " + val + " <---Ser_1");
      }
      break;
    case 2:
      if (Serial2.available() && !if_available) {  
        if_available = true;
        while(Serial2.available()) {
          delayMicroseconds(200);
          val += char(Serial2.read());     
        }
        if (debug_serial && val.length() > 2) Serial.println("Ser_2---> " + val + " <---Ser_2");
      }
      break;
    case 3:
      if (Serial3.available() && !if_available) { 
        if_available = true; 
        while(Serial3.available()) {
          delayMicroseconds(200);
          val += char(Serial3.read());     
        }
        if (debug_serial && val.length() > 2) Serial.println("Ser_3---> " + val + " <---Ser_3");
      }
      break;
  }   
  
  return val;
}

bool check_data (String val_str, int val_num) {
  int val_length = 0;
  int val_pos = 0;
  int check_val = 0;
  while(val_str.indexOf(";",val_pos) > - 1) {
    val_pos = val_str.indexOf(";",val_pos);
    val_pos++;
    check_val++; 
  }
  if (check_val == val_num) return true;
  else return false;
}

int check_ID (String val) {
  int out_val = -1;
  if (val.indexOf(start_index) > -1) { 
    byte index_length = start_index.length(); 
    String val_data = val.substring(val.indexOf(start_index) + index_length, val.indexOf(":"));
    out_val = atoi(val_data.c_str());
  }
  return out_val;
}

int check_port (String val) {
  int out_val = -1;
  if (val.indexOf(port_index) > -1) { 
    byte index_length = start_index.length(); 
    String val_data = val.substring(val.indexOf(":") + 1, val.indexOf(port_index));
    out_val = atoi(val_data.c_str());
  }
  return out_val;
}

void read_terminal_command () {  
  String input_command = Serial_read_data();
// вкл/выкл данных напрямую с сериал порта *******************************************************
  if (input_command.indexOf(debug_serial_index) > -1) { 
    debug_serial = !debug_serial;
    if (debug_serial) Serial.println("***** Debug serial inputs ON *****"); 
    else Serial.println("***** Debug serial inputs OFF *****"); 
    write_param_EEPROM ();
  }
// изменение задержки отправки данных по RS485 *******************************************************
  if (input_command.indexOf(RS485_delay_index) > -1) { 
    byte index_length = RS485_delay_index.length(); 
    String data = input_command.substring(input_command.indexOf(RS485_delay_index) + index_length, input_command.indexOf(no_resend_index));
    RS485_send_delay = atoi(data.c_str());
    RS485_send_delay = constrain(RS485_send_delay, 0, REQUEST_PERIOD / 2);
    Serial.println("***** Send data delay changed: "  + String(RS485_send_delay) + "ms *****"); 
    write_param_EEPROM ();
  }
// изменение периода отправки данных *************************************************************
  if (input_command.indexOf(send_period_index) > -1 && input_command.indexOf(no_resend_index) > -1) { 
    byte index_length = send_period_index.length(); 
    String data = input_command.substring(input_command.indexOf(send_period_index) + index_length, input_command.indexOf(no_resend_index));
    send_period = atoi(data.c_str());
    Serial.println("***** Send data period changed: "  + String(send_period) + "ms *****"); 
    write_param_EEPROM ();
  }
  

}

void send_nextion_txt (String val_name, String cmd) {
  while (Serial3.available())Serial3.read();
  Serial3.print(val_name + ".txt=\"" + cmd + "\""+char(255)+char(255)+char(255));
}

void send_nextion_bg_color (String val_name, unsigned int color = 0) {
  while (Serial3.available())Serial3.read();
  Serial3.print(val_name + ".bco=" + String(color) +char(255)+char(255)+char(255));
}

void send_nextion_text_color (String val_name, unsigned int color = 0) {
  while (Serial3.available())Serial3.read();
  Serial3.print(val_name + ".pco=" + String(color) +char(255)+char(255)+char(255));
}

void send_nextion_val (String val_name, int val) {
  while (Serial3.available())Serial3.read();
  Serial3.print(val_name + ".val=" + String(val) +char(255)+char(255)+char(255));
}
