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
    temp.timer_reset();
  }
}

float flow (byte num = 0) {
  float output_val = 60.0 / (float(interrupt_period[num]) / 1000.0) / coef.flow[num];  // l/min
  if (millis() - last_interrupt_period[num] >= 1000) output_val = 0.0;
  output_val = constrain(output_val,0.0,999.9);
  return output_val;
}

float RPM (byte num = 0) {
  float output_val = 60.0 / (float(interrupt_period[num]) / 1000.0);  // RPM
  if (millis() - last_interrupt_period[num] >= 1000) output_val = 0.0;
  output_val = constrain(output_val,0.0,999.9);
  return output_val;
}

float volume (byte num = 0) {
  float output_val = float(local_interrupt_impulses[num]) / coef.flow[num];
  output_val = constrain(output_val,0.0,999.9);
  return output_val;
} 

float seeder_revolutions () {
  float output_val = float(local_interrupt_impulses[seeder_RPM_input_num]) / coef.RPM_holes[seeder_RPM_input_num];
  output_val = constrain(output_val,0.0,99999.9);
  return output_val;
}

String fill_info_array () {
  String output_val = start_index + String(MODULE_ID) + ":" + inf_index;
  for (int i = 0; i < max_outputs; i++) {
    output_val += output[i].get_status();
  }
  output_val += toStr(voltage()) + toStr(current()) + toStr(MOSFET_temp) + toStr(flow(global_flow_input_num)) + toStr(volume(global_flow_input_num));
  output_val += end_index;
  return output_val;
}

String fill_port_info (int port_num) {
  String output_val = start_index + String(MODULE_ID) + ":" + String(port_num) + port_index + inf_index;
  output_val += output[port_num].get_status();
  //output_val += toStr(voltage()) + toStr(current()) + toStr(MOSFET_temp) + toStr(flow(global_flow_input_num)) + toStr(volume(global_flow_input_num));
  output_val += end_index;
  return output_val;
}

String fill_parameters_array () {
  String output_val = start_index + String(MODULE_ID) + ":" + parameters_index;
  output_val+= String(coef.voltage) + ";" + String(coef.current); // + ";" + String(coef.flow) + ";";
  output_val += end_index;
  return output_val;
}

String fill_status_array () {
  String output_val = start_index + String(MODULE_ID) + ":" + status_index; 
  output_val += toStr(voltage()) + toStr(current()) + toStr(MOSFET_temp);
  if (MODULE_ID == POMP_ID) output_val += toStr(flow(global_flow_input_num)) + toStr(volume(global_flow_input_num));
  else if (MODULE_ID == SEEDER_ID) output_val += toStr(seeder_revolutions()) + "-1.0;";
  else output_val += "-1.0;-1.0;";
  output_val += end_index;
  return output_val;
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

void control_message_parse (String input_data) {    
    int data_ID = check_ID(input_data);
    if (data_ID == MODULE_ID) {
      Serial_write_data("message_parse: " + input_data);
      digitalWrite(LED_pin, HIGH);
      if (input_data.indexOf(get_index) > -1) { // ответ на запрос ****************************************** 
        Serial_write_data("Get command accepted");
        digitalWrite(13, LOW);
        if (input_data.indexOf(inf_index) > -1) {
          send_data_to_RS485("ERROR!");
        } else if (input_data.indexOf(parameters_index) > -1) {
          send_data_to_RS485(fill_parameters_array());
        } else if (input_data.indexOf(status_index) > -1) {
          send_data_to_RS485(fill_status_array());
        }
        input_data = "";
      } else if (input_data.indexOf(control_index) > -1) { // получение управляющих команд и исполнение ****************************************** 
        int port = check_port(input_data);
        int index_length = control_index.length();
        String data = input_data.substring(input_data.indexOf(control_index) + index_length, input_data.indexOf(end_index));
        byte data_length = data.length();
        Serial_write_data("Control commands accepted: " + data);
        String str_val[4];
        byte s_pos = 0;
        byte val_length = 0;
        str_val[0] = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val[0].length() + 1;
        str_val[1] = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val[1].length() + 1;
        str_val[2] = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val[2].length() + 1;
        str_val[3] = data.substring(val_length, data.indexOf(";", val_length)); val_length += str_val[3].length() + 1;
        global_speed = atof(str_val[0].c_str());
        output[port].parameters_read(atoi(str_val[1].c_str()),atof(str_val[2].c_str()),atoi(str_val[3].c_str()));
        send_data_to_RS485(fill_port_info(port));
        //write_param_EEPROM();
      } else if (input_data.indexOf(reset_volume_index) > -1) {
        local_interrupt_impulses[global_flow_input_num] = 0;
      } else if (input_data.indexOf(reset_rev_index) > -1) {
        local_interrupt_impulses[seeder_RPM_input_num] = 0;
      }
      digitalWrite(LED_pin, LOW);    
    }
    //read_termnial_command(input_data);
}

void control_func () {  
  control_message_parse (Serial_read_data(rs485_serial));
  for (int i = 0; i < max_outputs; i++) {
    output[i].main();
  }
  //Serial.println(fill_control_command(2,1));
  //local_interrupt_impulses[seeder_RPM_input_num]++;
  //static long prev_imp; // = local_interrupt_impulses[seeder_RPM_input_num];
  //if (local_interrupt_impulses[seeder_RPM_input_num] != prev_imp) Serial.println("IMP_2: " + String(local_interrupt_impulses[seeder_RPM_input_num]));
  //prev_imp = local_interrupt_impulses[seeder_RPM_input_num];
}
