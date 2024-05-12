#include "Switch_config.h"

#define NEXTION_UPDATE_PERIOD 10 //ms
#define REQUEST_TIMEOUT 100
#define CHECK_ONLINE_TIME 10000 //ms ожидание устройств в сети после старта программы
#define ONLINE_MIN_COUNT 10
#define MAX_TIMEOUT 2000
#define SPEED_FILTER_TIMER 50 // ms

GMedian<5, float> speed_median_filter;  

struct Devices {
  bool online;
  bool available;
  int online_count;
  float voltage; // V
  float current;
  float MOSFET_temp;
  float flow;
  float volume;
  float seeder_rev;
  unsigned long timeout_millis;
  my_timer timeout;
};

Devices device[MAX_ID];

Switch_mode switch_mode[7];
long total_range;
my_timer send_data;
my_timer nextion_update;
my_timer speed_filter;
byte send_pos, send_port_turn, request_pos, request_turn, nextion_send_pos;
byte nextion_send_turn = 0;
bool message_sent = false;
String GLOBAL_RS485_MESSAGE;
float prev_global_range;
float GLOBAL_RANGE;

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
  static float buffer_val;
  static float output_val;
  speed_filter.timer(SPEED_FILTER_TIMER);
  if (!speed_filter.stat) {
    speed_filter.stat = true;
  }
  if (speed_filter.timer_ready()) {
    if (interrupt_period[num] > 20) buffer_val = 1.0 / (float(interrupt_period[num]) * coef.wheel_IPR / 1000.0) * 22.62 * coef.wheel_radius;  // km/h
    if (millis() - last_interrupt_period[num] >= 1000) buffer_val= 0.0;
    output_val = constrain(buffer_val,0.0,19.9);
    output_val = speed_median_filter.filtered(output_val);
    speed_filter.timer_reset();
  }
  //send_nextion_txt("ID1_stat", String(buffer_val) + " | " + String(output_val));
  return output_val;
}

long range (byte num = 0) {
  float output_val = float(local_interrupt_impulses[num]) * 6.28 * coef.wheel_radius  / coef.wheel_IPR; // m
  return output_val;
}

String device_stat (int val_ID) {
  String val;
  if (val_ID == 1) val = "POMP_UNIT: ";
  else if (val_ID == 2) val = "SEEDER_UNIT: ";
  else val = "UNKNOWN_UNIT:";
  if (device[val_ID].available) {
    if (device[val_ID].online) val += "ONLINE";
    else val += "OFFLINE";
  } else {
    val += "UNAVAILABLE";
  }
  return val;
}

String device_param (int val_ID) {
  String val;
  byte pos = max_outputs * 2;
  if (device[val_ID].online) {
    val = String(device[val_ID].voltage,1) + "V*" + String(device[val_ID].current,1) + "A|T:" + String(device[val_ID].MOSFET_temp,0) + "'C";
    if (val_ID == POMP_ID) val += "\r\nF:" + String(device[val_ID].flow,1) + "L/min|V:" + String(device[val_ID].volume,1) + "L";
    else if (val_ID == SEEDER_ID) val += "\r\nRev:" + String(device[val_ID].seeder_rev,1) + "";
  } else {
    val = "-";
  }
  return val;
}

void message_parse (String input_data) { 
  GLOBAL_RS485_MESSAGE = input_data;
  int input_data_ID = check_ID(input_data);
  int input_port = check_port(input_data);
  if (input_data_ID > 0 && input_data.indexOf(start_index) > -1 && input_data.indexOf(end_index) > -1) {
    Serial_write_data("Message_parse: " + input_data + " -> " + String(input_data.length()));
    if (input_data.indexOf(inf_index) > -1) {
      byte index_length = inf_index.length();
      String data = input_data.substring(input_data.indexOf(inf_index) + index_length, input_data.indexOf(end_index));
      byte data_length = data.length();
      byte val_length = 0;
      const byte MAX_CELLS = 3;
      String str_val[MAX_CELLS];
      for (int i = 0; i < MAX_CELLS; i++) {
        str_val[i] = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val[i].length() + 1;
      }
      for (int i = 0; i < 7; i++) {   
        if (switch_config[i][0] == input_data_ID && switch_config[i][1] == input_port) {
          switch_mode[i].set_feedback(atoi(str_val[0].c_str()),atoi(str_val[1].c_str()),atof(str_val[2].c_str()));
        }    
      }
    } else if (input_data.indexOf(status_index) > -1) {
      byte index_length = status_index.length();
      String data = input_data.substring(input_data.indexOf(status_index) + index_length, input_data.indexOf(end_index));
      byte data_length = data.length();
      byte val_length = 0;
      const byte MAX_CELLS = 5;
      String str_val[MAX_CELLS];
      for (int i = 0; i < MAX_CELLS; i++) {
        str_val[i] = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val[i].length() + 1;
      }
      device[input_data_ID].voltage = atof(str_val[0].c_str());
      device[input_data_ID].current = atof(str_val[1].c_str());
      device[input_data_ID].MOSFET_temp = atof(str_val[2].c_str());
      if (input_data_ID == POMP_ID) {
        device[input_data_ID].flow = atof(str_val[3].c_str());
        device[input_data_ID].volume = atof(str_val[4].c_str());
      } else if (input_data_ID == SEEDER_ID) {
        device[input_data_ID].seeder_rev = atof(str_val[3].c_str());
      }
      
    } else if (input_data.indexOf(parameters_index) > -1) {
      byte index_length = parameters_index.length();
      String data = input_data.substring(input_data.indexOf(parameters_index) + index_length, input_data.indexOf(end_index));
      byte data_length = data.length();
      byte val_length = 0;
      const byte MAX_CELLS = 20;
      String str_val[MAX_CELLS];
      for (int i = 0; i < MAX_CELLS; i++) {
        str_val[i] = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val[i].length() + 1;
      }
    }
    if (input_data.indexOf(start_index) > -1) {
      device[input_data_ID].online = true;
      device[input_data_ID].timeout.timer_reset();
    }
      //write_param_EEPROM();
  }     
}

String fill_control_command (int val_ID, int port_num) {
  String val = start_index + String(val_ID) + ":" + String(port_num) + port_index + control_index + toStr(speedT(speed_RPM_input_num));
  String command_val = "-1;0.0;-1;";
  for (int i = 0; i < 7; i++) {
    if (switch_config[i][0] == val_ID && switch_config[i][1] == port_num) {
      command_val = switch_mode[i].get_status();
    }
  }
  val += command_val + end_index;
  return val;
}

//ID2:0_port/ctrl/5.0;0;50.0;3;/re -- сеялка
//ID2:1_port/ctrl/0.0;0;0.0;0;/re



String inf_request (int val_ID, int port_num) {
  String output_val = start_index + String(val_ID) + ":" + String(port_num) + port_index + get_index + inf_index + end_index;
  return output_val; 
}

String param_request (int val_ID) {
  String output_val = start_index + String(val_ID) + ":" + get_index + parameters_index + end_index;
  return output_val; 
}

String status_request (int val_ID) {
  String output_val = start_index + String(val_ID) + ":" + get_index + status_index + end_index;
  return output_val; 
}

String reset_volume_request (int val_ID) {
  String output_val = start_index + String(val_ID) + ":" + reset_volume_index + end_index;
  return output_val;
}

String reset_seeder_rev_request (int val_ID) {
  String output_val = start_index + String(val_ID) + ":" + reset_rev_index + end_index;
  return output_val;
}

bool SEND_DATA (String message_val) {
  bool out_stat = false;
  int out_ID = check_ID(message_val);
  int out_port = check_port(message_val);
  int in_ID = check_ID(GLOBAL_RS485_MESSAGE);
  int in_port = check_port(GLOBAL_RS485_MESSAGE);
  if (ready_to_send_update() && !message_sent) {
    send_data_to_RS485(message_val);
    Serial_write_data(message_val);
    message_sent = true;
  }
  if (out_ID == in_ID && out_port == in_port && message_val.indexOf(end_index) > -1) {
    out_stat = true;
    Serial_write_data(message_val + " -> ANSWER ACCEPTED");
  }
  return out_stat;
}

void send_control () {
  send_data.timer(REQUEST_TIMEOUT);
  if (!send_data.stat) {
    send_data.stat = true;
  }
    switch (send_pos) {
      case 0: // 
        if (device[1].available) {
            if (SEND_DATA(fill_control_command(1,send_port_turn)) || send_data.timer_ready()) {
            send_port_turn++;
            send_data.timer_reset();
            message_sent = false;
          }
        } else {
          send_pos++;
          send_port_turn = 0;
        }
        break;
      case 1: // 
        if (device[2].available) {
          if (SEND_DATA(fill_control_command(2,send_port_turn)) || send_data.timer_ready()) {
            send_port_turn++;
            send_data.timer_reset();
            message_sent = false;
          }
        } else {
          send_pos++;
          send_port_turn = 0;
        }
        break;
      case 2: // 
        if (device[1].available) {
          if (SEND_DATA(status_request(1)) || send_data.timer_ready()) {
            send_pos++;
            send_port_turn = 0;
            send_data.timer_reset();
            message_sent = false;
          }
        } else {
          send_pos++;
          send_port_turn = 0;
        }  
        break;
      case 3: // 
        if (device[2].available) {
          if (SEND_DATA(status_request(2)) || send_data.timer_ready()) {
            send_pos++;
            send_port_turn = 0;
            send_data.timer_reset();
            message_sent = false;
          }
        } else {
          send_pos++;
          send_port_turn = 0;
        }  
        break;
      }
      
    if (send_port_turn >= max_outputs) {
      send_port_turn = 0;
      send_pos++;
    }
    if (send_pos >= 4) {
      send_port_turn = 0;
      send_pos = 0;
    }
   
}

void fill_nextion () {
  nextion_update.timer(NEXTION_UPDATE_PERIOD);
  if (!nextion_update.stat) {
    nextion_update.stat = true;
  }
  String rb_val, range_val, engine_t_val;
  int i = nextion_send_pos;
  if (nextion_update.timer_ready()) {
    switch (nextion_send_turn) {
      case 0:
        send_nextion_txt("Left_bar", timeStr() + " | " + String(program_millis(),1) + "s");
        rb_val += "Y: " + String(GLOBAL_RANGE,1) + "km | ";
        if (outside_temp > -100.0) rb_val += String(outside_temp,1) + "'C ";
        else rb_val += "--.-'C ";
        if (voltage() > 2.0) rb_val += "| " + String(voltage(),1) + "V";
        else rb_val += "| --.-V";
        send_nextion_txt("Right_bar", rb_val);
        //Serial.println("HERE");
        nextion_send_turn++;
        break;
      case 1:
        if (range(speed_RPM_input_num) < 1000) range_val = String(range(speed_RPM_input_num)) + "m";
        else range_val = String(float(range(speed_RPM_input_num)) / 1000.0, 2) + "km";
        if (engine_temp > -100.0) engine_t_val += String(engine_temp,1) + "'C ";
        else engine_t_val += "---'C ";
        if (engine_temp > MAX_ENGINE_TEMP) {
          send_nextion_text_color("Speed", RED);
        } else {
          if (DISLAY_DARK_STYLE) send_nextion_text_color("Speed", WHITE);
          else send_nextion_text_color("Speed", BLACK);
        }
        send_nextion_txt("Speed", "S:" + String(speedT(speed_RPM_input_num),1) + "km/h | R:" + range_val + " | T:" + engine_t_val);
        nextion_send_turn++;
        break;
      case 2: 
        send_nextion_txt("ID1_stat", device_stat(1));
        send_nextion_txt("ID2_stat", device_stat(2));
        nextion_send_turn++;
        break;
      case 3:
        send_nextion_txt("ID1_param", device_param(1));
        send_nextion_txt("ID2_param", device_param(2));
        nextion_send_turn++;
        break;
      case 4: 
        int i = nextion_send_pos;
        send_nextion_bg_color("sw_" + String(i+1), switch_mode[i].color());
        if (i < 5) {
          send_nextion_txt("sw_" + String(i+1), switch_name[i] + "\r\n" + switch_mode[i].mode_str());
          send_nextion_txt("swD_" + String(i+1), String(switch_mode[i].feedback_parameter()));
        } else {
          send_nextion_txt("sw_" + String(i+1), switch_name[i] + " " + switch_mode[i].mode_str());
        }
        nextion_send_pos++;
        if (nextion_send_pos > 7) {
          nextion_send_pos = 0;
          nextion_send_turn++;
        }
        break;
    }
    if (nextion_send_turn > 4) {
      nextion_send_turn = 0;
      nextion_send_pos = 0;
    }
    nextion_update.timer_reset();
  }
  for (int i = 0; i < 7; i++) {
    if (switch_mode[i].get_changes()) {
      send_nextion_bg_color("sw_" + String(i+1), switch_mode[i].color());
      if (i < 5) {
          send_nextion_txt("sw_" + String(i+1), switch_name[i] + "\r\n" + switch_mode[i].mode_str());
          send_nextion_txt("swD_" + String(i+1), String(switch_mode[i].feedback_parameter()));
          //send_nextion_txt("swD_" + String(i+1), "1.0atm\r\n99%");
        } else {
          send_nextion_txt("sw_" + String(i+1), switch_name[i] + " " + switch_mode[i].mode_str());
        }
    }
  }
}

void check_devices_online () {
  for (int i = 1; i < MAX_ID; i++) {
    if (millis() < CHECK_ONLINE_TIME) {
      device[i].available = true;
      if (device[i].online) device[i].online_count++;
    } else {
      if (device[i].online_count >= ONLINE_MIN_COUNT) device[i].available = true;
      else device[i].available = false;
    }
  }
}

float global_range_EEPROM_read () {
  float output_val;
  byte a = EEPROM.read(1000);
  byte b = EEPROM.read(1001);
  output_val = float(a) / 10.0 + float(b) * 25.0;
  return output_val;
}


void global_range_EEPROM_write (float input_val) {
  int b = int(input_val / 25.0);
  float val = input_val - (b * 25.0);
  int a = int(val * 10);
  EEPROM.write(1000, a);
  EEPROM.write(1001, b);
}

void update_global_range () {
  float val = range(speed_RPM_input_num) / 1000.0;
  float EEPROM_range;
  float d_range = val - prev_global_range;
  if (d_range >= 0.1) {
    EEPROM_range = global_range_EEPROM_read();
    GLOBAL_RANGE = EEPROM_range + d_range;
    global_range_EEPROM_write(GLOBAL_RANGE);
    prev_global_range = val;
  }
}

void button_func () {
  byte current_button = read_buttons();
  String page_name;
  switch (current_button) {
    case 1:
      DISLAY_DARK_STYLE = !DISLAY_DARK_STYLE;
      if (DISLAY_DARK_STYLE) page_name = "page 1";
      else page_name = "page 2";
      Serial3.print(page_name +char(255)+char(255)+char(255));
      EEPROM.write(1, byte(DISLAY_DARK_STYLE));
      delay(1000);
      break;
    case 2:
      send_data_to_RS485(reset_seeder_rev_request(SEEDER_ID));
      break;
    case 4: 
      send_data_to_RS485(reset_volume_request(POMP_ID));
      break;
    case 8:
      local_interrupt_impulses[0] = 0;
      break;
  }
}

void main_func () {
  check_devices_online();
  update_global_range();
  button_func();
  RTC.read(tm);
  fill_nextion();
  send_control();
  main_set_temp();
  message_parse(Serial_read_data(rs485_serial));
  for (int i = 0; i < 7; i++) {
    switch_mode[i].get_status();
  }
  for (int i = 0; i < MAX_ID; i++) {
    device[i].timeout.timer(MAX_TIMEOUT);
    if (!device[i].timeout.stat) {
      device[i].timeout.stat = true;
    }
    if (device[i].timeout.timer_ready()) {
      device[i].online = false;
    }
  }
  //device[2].online = true;
}
