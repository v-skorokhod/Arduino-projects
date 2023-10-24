#include "Output_controller.h"

bool output_enable [max_outputs] = {1,1,1,1,1};
output_controller output[max_outputs];

void control_set_temp () { 
  temp.timer(temp_request);
  if (!temp.stat) {
    temp.stat = true;
  }
  if (temp.timer_ready()) {
    MOSFET_temp  = read_temp();
    //Serial.println("T_FET:" + String(MOSFET_temp));
    temp.timer_reset();
  }
}

float flow (byte num = 0) {
  float val = 60.0 / (float(interrupt_period[num]) / 1000000.0) / coef.flow[num];  // l/min
  if (micros() - last_interrupt_period[num] >= 1000000) val = 0.0;
  val = constrain(val,0.0,999.9);
  return val;
}

float RPM (byte num = 0) {
  float val = 60.0 / (float(interrupt_period[num]) / 1000000.0);  // lRPM
  if (micros() - last_interrupt_period[num] >= 1000000) val = 0.0;
  val = constrain(val,0.0,999.9);
  return val;
}

float volume (byte num = 0) {
  float val = float(local_interrupt_impulses[num]) / coef.flow[num];
  val = constrain(val,0.0,999.9);
  return val;
} 

float flow_per_km () {
  float val;
  return val;
}

String fill_info_array () {
  String val = start_index + String(MODULE_ID) + ":" + inf_index;
  for (int i = 0; i < max_outputs; i++) {
    val += output[i].get_status();
  }
  val += toStr(voltage()) + toStr(current()) + toStr(MOSFET_temp) + toStr(flow(global_flow_input_num)) + toStr(volume(global_flow_input_num));
  val += end_index;
  return val;
}

String fill_parameters_array () {
  String val = start_index + String(MODULE_ID) + ":" + parameters_index;
  val+= String(coef.voltage) + ";" + String(coef.current); // + ";" + String(coef.flow) + ";";
  val += end_index;
  return val;
}

String fill_status_array () {
  String val = start_index + String(MODULE_ID) + ":" + ready_index + end_index;
  return val;
}
/*
void EEPROM_parse (bool val = true) {
  byte pos = 0;
  if (val == true) {
    debug_serial = EEPROM_param[pos++];
    RS485_send_delay = EEPROM_param[pos++];
    coef.voltage = float(EEPROM_param[pos++]) / 100.0;
    coef.current = float(EEPROM_param[pos++]) / 100.0;
    coef.flow = EEPROM_param[pos++] * 20;
    set_pressure = float(EEPROM_param[pos++]) / 10.0;
    set_flow = float(EEPROM_param[pos++]) / 10.0;
  } else {
    EEPROM_param[pos++] = debug_serial;
    EEPROM_param[pos++] = RS485_send_delay;
    EEPROM_param[pos++] = byte(coef.voltage * 100.0);
    EEPROM_param[pos++] = byte(coef.current * 100.0);
    EEPROM_param[pos++] = byte(coef.flow / 20);
    EEPROM_param[pos++] = byte(set_pressure * 10.0);
    EEPROM_param[pos++] = byte(set_flow * 10.0);
  }
}*/

void control_commands_parse () {
  int pos = 0;
  for (int i = 0; i < max_outputs; i++) {
    output[i].parameters_read(int(set_float[pos]),set_float[pos+1]);
    pos = pos + 2;
  }
}

void control_message_parse (String input_data) {
  if (input_data.length() > 2) {
    Serial.println("message_parse: " + input_data);
    int data_ID = check_ID(input_data);
    if (data_ID == MODULE_ID) {
      digitalWrite(LED_pin, HIGH);
      if (input_data.indexOf(get_index) > -1) { // ответ на запрос ****************************************** 
        Serial.println("Get command accepted");
        digitalWrite(13, LOW);
        delay(RS485_send_delay * 2);
        if (input_data.indexOf(inf_index) > -1) {
          send_data_to_RS485(fill_info_array());
        } else if (input_data.indexOf(parameters_index) > -1) {
          send_data_to_RS485(fill_parameters_array());
        } else if (input_data.indexOf(status_index) > -1) {
          send_data_to_RS485(fill_status_array());
        }
        input_data = "";
      } else if (input_data.indexOf(control_index) > -1) { // получение управляющих команд и исполнение ****************************************** 
        int index_length = control_index.length();
        String data = input_data.substring(input_data.indexOf(control_index) + index_length, input_data.indexOf(end_index));
        byte data_length = data.length();
        Serial.println("Control commands accepted: " + data);
        String str_val;
        byte s_pos = 0;
        byte val_length = 0;
        for (int i = 0; i < max_outputs * 2; i++) {      
          str_val = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val.length() + 1; 
          set_float[i] = atof(str_val.c_str());
        }
        control_commands_parse();
        //write_param_EEPROM();
      } else if (input_data.indexOf(reset_volume_index) > -1) {
        local_interrupt_impulses[global_flow_input_num] = 0;
      }
      digitalWrite(LED_pin, LOW);    
    }
  }
}

void control_func () {
  control_set_temp();
  control_message_parse (Serial_read_data(rs485_serial));
  for (int i = 0; i < max_outputs; i++) {
    output[i].main();
  }
}
