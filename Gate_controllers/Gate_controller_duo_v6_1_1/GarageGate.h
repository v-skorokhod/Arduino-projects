bool BUTTON_MODE = false;
unsigned int prev_driver_mode;
bool second_click_val = false;
svsTimer dbl_click_timeout;

void open_door () {
  digitalWrite(reverse_rl_pin, HIGH);
  delay(100);
  digitalWrite(power_rl_pin, HIGH);
  set_LED_color(2);
}

void close_door () {
  digitalWrite(reverse_rl_pin, LOW);
  delay(100);
  digitalWrite(power_rl_pin, HIGH);
  set_LED_color(2);
}

void stop_door () {
  digitalWrite(power_rl_pin, LOW);
  delay(100);
  digitalWrite(reverse_rl_pin, LOW);
  delay(100);
  set_LED_color(0);
}

void garage_gate_init () {
  Serial.println("Garage_gate_module");
  pinMode(reverse_rl_pin, OUTPUT);
  pinMode(power_rl_pin, OUTPUT);
  stop_door();
}

void garage_gate_loop () {
  prev_current_button = current_button;
  current_button = g_read_buttons();
  int IO_button = read_IO_data(); 

  if (current_button) {
    if (current_button == 1 || current_button == 2) {
      button_hold_tm.reset();
      if (output_driver_mode) {
        output_driver_mode = 0;  
        //send_IO_data(8);     
      } else {
        output_driver_mode = current_button;
      }
    } else if (current_button == 4) {
      if (!second_click_val) {
        send_IO_data(1);
        second_click_val = true;
        dbl_click_timeout.reset();
      } else {
        send_IO_data(8);
        second_click_val = false;
      }
      
    } else if (current_button == 8) {
      if (!second_click_val) {
        send_IO_data(2);
        second_click_val = true;
        dbl_click_timeout.reset();
      } else {
        send_IO_data(8);
        second_click_val = false;
      }
    }
  }

  if (dbl_click_timeout.ready(button_hold_delay) && second_click_val) second_click_val = false;

  if (button_hold_tm.ready(button_hold_delay)) output_driver_mode = 0;

  if (IO_button != no_data_val) {
    button_hold_tm.reset();
    Serial.println(IO_button);
    switch (IO_button) {
      case 8: // stop 
        output_driver_mode = 0;
        break;
      case 1: // open 
        output_driver_mode = 1;
        break;
      case 2: // close
        output_driver_mode = 2;
        break;
      case 4: // wicket 
        output_driver_mode = 2;
        break; 
    } 
  }

  if (output_driver_mode == 1) open_door();
  else if (output_driver_mode == 2) close_door();
  else if (output_driver_mode == 0) stop_door();

  if (prev_driver_mode != output_driver_mode && prev_driver_mode) {
    Serial.println("button changed, stopped for 200ms");
    stop_door();
  }

  prev_driver_mode = output_driver_mode;
}