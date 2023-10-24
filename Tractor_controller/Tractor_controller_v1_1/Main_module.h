#include "Switch_config.h"

Switch_mode switch_mode[7];

long total_range;
float info_array[max_ID][20];
float param_array[max_ID][20];

my_timer send_data;
my_timer timeout[max_ID];
my_timer nextion_update;
#define NEXTION_UPDATE_PERIOD 1000 //ms


unsigned long timeout_millis[max_ID];
bool device_online[max_ID];
#define MAX_TIMEOUT 5000
byte send_pos, send_turn, request_pos, request_turn;

unsigned long debug_serial_millis;

void main_set_temp () { 
  temp.timer(temp_request);
  if (!temp.stat) {
    temp.stat = true;
  }
  if (temp.timer_ready()) {
    engine_temp = read_temp(0);
    outside_temp = read_temp(1);
    temp.timer_reset();
  }
}

float speedT (byte num = 0) {
  //float val = 22.62 * coef.wheel_radius / (float(interrupt_period[num]) / 1000000.0) * coef.wheel_IPR;  // km/h
  float val = 1.0 / (float(interrupt_period[num]) * coef.wheel_IPR / 1000000.0) * 22.62 * coef.wheel_radius;  // km/h
  if (micros() - last_interrupt_period[num] >= 1000000) val = 0.0;
  //Serial.println(val,5);
  val = constrain(val,0.0,99.9);
  return val;
}

long range (byte num = 0) {
  float val = float(global_interrupt_impulses[num]) * 6.28 * coef.wheel_radius  / coef.wheel_IPR; // m
  return val;
}

String inf_request(int val_ID) {
  String out_val = start_index + String(val_ID) + ":" + get_index + inf_index + end_index;
  //if (val_ID == 2) out_val = "ID1:ctrl/0.0;" + toStr(random(50)) + "1.0;" + toStr(random(50)) + "2.0;" + toStr(random(50)) + "3.0;" + toStr(random(50)) + "5.0;6.0;/re";
  return out_val; 
}

String param_request(int val_ID) {
  String out_val = start_index + String(val_ID) + ":" + get_index + parameters_index + end_index;
  return out_val; 
}

String device_stat (int val_ID) {
  String val;
  if (val_ID == 1) val = "POMP_UNIT: ";
  else if (val_ID == 2) val = "SEEDER_UNIT: ";
  else val = "UNKNOWN_UNIT:";
  if (device_online[val_ID]) val += "ONLINE";
  else val += "OFFLINE";
  return val;
}

String device_param (int val_ID) {
  String val;
  byte pos = max_outputs * 2;
  if (device_online[val_ID]) {
    val = String(info_array[val_ID][pos],1) + "V*" + String(info_array[val_ID][pos+1],1) + "A | T_FET:" + String(info_array[val_ID][pos+2],0) + "'C";
    if (val_ID == POMP_ID) val += "\r\nF:" + String(info_array[val_ID][pos+3],1) + "L/min | V:" + String(info_array[val_ID][pos+4],1) + "L";
  } else {
    val = "-";
  }
  return val;
}

void data_parse (int val_ID) {
  int pos = 1; 
  for (int i = 0; i < 7; i++) {
    //Serial.println("HERE: " + toStr(switch_config[i][0]) + toStr(val_ID) + toStr(switch_config[i][1]) + toStr(i));
    if (switch_config[i][0] == val_ID) {
      for (int j = 0; j < 7; j++) {
        if(switch_config[j][1] == i) {
          switch_mode[j].feedback_val = info_array[val_ID][pos];
          //Serial.println("feedback_val_" + String(j) + " -> " + String(switch_mode[i].feedback_val)); -----> ??????????????????
        }
      }
    }
    pos += 2;
  }
}

void message_parse (String input_data) { 
  if (input_data.length() > 2) Serial_write_data("message_parse: " + input_data + " -> " + String(input_data.length()));
  int input_data_ID = check_ID(input_data);
  if (input_data_ID > 0 && input_data.indexOf(start_index) > -1 && input_data.indexOf(end_index) > -1) {
    if (input_data.indexOf(inf_index) > -1) {
      byte index_length = inf_index.length();
      String data = input_data.substring(input_data.indexOf(inf_index) + index_length, input_data.indexOf(end_index));
      byte data_length = data.length();
      String str_val;
      byte val_length = 0;
      int data_cells = max_outputs * 2 + 5;
      for (int i = 0; i < data_cells; i++) {      
        str_val = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val.length() + 1; 
        info_array[input_data_ID][i] = atof(str_val.c_str());
        Serial_write_data(String(info_array[input_data_ID][i]));
      }
    }  else if (input_data.indexOf(parameters_index) > -1) {
      byte index_length = parameters_index.length();
      String data = input_data.substring(input_data.indexOf(parameters_index) + index_length, input_data.indexOf(end_index));
      byte data_length = data.length();
      String str_val;
      byte val_length = 0;
      for (int i = 0; i < 20; i++) {      
        str_val = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val.length() + 1; 
        param_array[input_data_ID][i] = atof(str_val.c_str());
        Serial_write_data(String(param_array[input_data_ID][i]));
      }
    }
    if (input_data.indexOf(start_index) > -1) {
      device_online[input_data_ID] = true;
      timeout[input_data_ID].timer_reset();
    }
      data_parse(input_data_ID);
      //write_param_EEPROM();
  }     
}

String fill_control_array (int val_ID) {
  String val = start_index + String(val_ID) + ":" + control_index;
  String cell_val[max_outputs];
  for (int i = 0; i < max_outputs; i++) cell_val[i] = "-1;0.0;";
  for (int i = 0; i < 7; i++) {
    if (switch_config[i][0] == val_ID) {
      cell_val[switch_config[i][1]] = switch_mode[i].get_status();
    }
  }
  for (int i = 0; i < max_outputs; i++) val += cell_val[i];
  val += end_index;
  return val;
}

String fill_reset_volume (int val_ID) {
  String val = start_index + String(val_ID) + ":" + reset_volume_index + end_index;
  return val;
}

void send_control () {
  send_data.timer(REQUEST_PERIOD);
  if (!send_data.stat) {
    send_data.stat = true;
  }
  if (send_data.timer_ready()) {
    switch (send_pos) {
      case 0: // 
        send_data_to_RS485(fill_control_array(1));
        //delay(RS485_send_delay * 2);
        //send_data_to_RS485(inf_request(1));
        break;
      case 1: // 
        send_data_to_RS485(fill_control_array(2));
        //delay(RS485_send_delay * 2);
        //send_data_to_RS485(inf_request(2));
        break;
      case 2: // 
        send_data_to_RS485(inf_request(1));
        delay(RS485_send_delay);
        break;
      case 3: // 
        send_data_to_RS485(inf_request(2));
        delay(RS485_send_delay);
        break;
      }
    send_pos++;
    if (send_pos >= 4) send_pos = 0;
    send_data.timer_reset();
  }
}

void fill_nextion () {
  nextion_update.timer(NEXTION_UPDATE_PERIOD);
  if (!nextion_update.stat) {
    nextion_update.stat = true;
  }
  if (nextion_update.timer_ready()) {
    send_nextion_txt("Left_bar", timeStr() + " | " + String(program_millis(),1) + "s");
    String rb_val = "";
    if (outside_temp > -100.0) rb_val += String(outside_temp,1) + "'C ";
    else rb_val += "--.-'C ";
    if (voltage() > 2.0) rb_val += "| " + String(voltage(),1) + "V";
    else rb_val += "| --.-V";
    send_nextion_txt("Right_bar", rb_val);
    String range_val;
    if (range() < 1000) range_val = String(range()) + "m";
    else range_val = String(float(range()) / 1000.0, 2) + "km";
    String engine_t_val;
    if (engine_temp > -100.0) engine_t_val += String(engine_temp,1) + "'C ";
    else engine_t_val += "---'C ";
    send_nextion_txt("Speed", "S:" + String(speedT(),1) + "km/h | R:" + range_val + "\r\nT:" + engine_t_val);
    send_nextion_txt("ID1_stat", device_stat(1));
    send_nextion_txt("ID2_stat", device_stat(2));
    send_nextion_txt("ID1_param", device_param(1));
    send_nextion_txt("ID2_param", device_param(2));
    for (int i = 0; i < 7; i++) {
      send_nextion_txt("sw_" + String(i+1), switch_name[i] + "\r\n" + switch_mode[i].mode_str());
      if (i < 5) send_nextion_txt("swD_" + String(i+1), String(switch_mode[i].feedback_parameter()));
    }
    nextion_update.timer_reset();
  }
  for (int i = 0; i < 7; i++) {
    if (switch_mode[i].get_changes()) {
      send_nextion_txt("sw_" + String(i+1), switch_name[i] + "\r\n" + switch_mode[i].mode_str());
      if (i < 5) send_nextion_txt("swD_" + String(i+1), String(switch_mode[i].feedback_parameter()));
    }
  }
}

void init_func () {
  
}

void main_func () {
  if (read_buttons() == 1) {
    DISLAY_DARK_STYLE = !DISLAY_DARK_STYLE;
    String page_name;
    if (DISLAY_DARK_STYLE) page_name = "page 1";
    else page_name = "page 2";
    Serial3.print(page_name +char(255)+char(255)+char(255));
    EEPROM.write(1, byte(DISLAY_DARK_STYLE));
    delay(1000);
  } else if (read_buttons() == 8) {
    send_data_to_RS485(fill_reset_volume(POMP_ID));
  }
  RTC.read(tm);
  fill_nextion();
  send_control();
  main_set_temp();
  message_parse(Serial_read_data(rs485_serial));
  for (int i = 0; i < 7; i++) {
    switch_mode[i].get_status();
  }
  for (int i = 0; i < max_ID; i++) {
    timeout[i].timer(MAX_TIMEOUT);
    if (!timeout[i].stat) {
      timeout[i].stat = true;
    }
    if (timeout[i].timer_ready()) {
      device_online[i] = false;
    }
  }
}
