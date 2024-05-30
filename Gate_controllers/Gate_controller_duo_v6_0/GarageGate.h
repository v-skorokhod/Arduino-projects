bool BUTTON_MODE = false;
unsigned int prev_duration;

void garage_gate_init () {
  Serial.println("Garage_gate_module");
  pinMode(reverse_rl_pin, OUTPUT);
  pinMode(power_rl_pin, OUTPUT);
  digitalWrite(reverse_rl_pin, LOW);
  digitalWrite(power_rl_pin, LOW);
}

void open_door () {
  digitalWrite(reverse_rl_pin, LOW);
  delay(100);
  digitalWrite(power_rl_pin, HIGH);
}

void close_door () {
  digitalWrite(reverse_rl_pin, HIGH);
  delay(100);
  digitalWrite(power_rl_pin, HIGH);
}

void garage_gate_loop () {
  
  prev_current_button = current_button;
  //current_button = button_emulator(); // <--раскомментировать чтобы работало с консоли (либо то, либо то) 
  current_button = g_read_buttons(); // <-------раскомментировать чтобы работало с  пульта (либо то, либо то) 
  int IO_button = read_IO_data(); 

  if (current_button) {
    //Serial.println(current_button);
    if (current_button == 1 || current_button == 2) {
      button_hold_tm.reset();
      if (BUTTON_MODE == true) {
        BUTTON_MODE = false;   
        send_IO_data(8);     
        //Serial.println("close");
      } else {
        BUTTON_MODE = true;
        output_driver_mode = current_button;
      }
    } else if (current_button == 4) {
      send_IO_data(1);
    } else if (current_button == 8) {
      send_IO_data(2);
    }
  }
  if (button_hold_tm.ready(button_hold_delay)) BUTTON_MODE = false;

  if (!BUTTON_MODE) {
    digitalWrite(power_rl_pin, LOW);
    delay(100);
    digitalWrite(reverse_rl_pin, LOW);
    delay(100);
    output_driver_mode = 0;
  }
  if (BUTTON_MODE == true) {
    if (output_driver_mode == 1) {        // OPEN
      open_door();
    } else if (output_driver_mode == 2) { // CLOSE
      close_door();
    } 
  } else {
    if (IO_button != 13) Serial.println(IO_button);
    if (IO_button != no_data_val) {
      switch (IO_button) {
        case 0: // stop 
          output_driver_mode = 0;
          BUTTON_MODE = false;
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
    if (output_driver_mode == 1) {        // OPEN
      open_door();
    } else if (output_driver_mode == 2) { // CLOSE
      close_door();
    } 
  }

/*



  if (current_button > 0 && current_button <= 2) {
    button_hold_tm.reset();
    if (BUTTON_MODE == true) {
      BUTTON_MODE = false;
      delay(50);
    } else {
      BUTTON_MODE = true;
      output_driver_mode = current_button;
      //delay(500);
    }
  }
  if (button_hold_tm.ready(button_hold_delay)) BUTTON_MODE = false;
  if (BUTTON_MODE == true) {
    if (output_driver_mode == 1) {        // OPEN
      digitalWrite(open_pin, HIGH);
      digitalWrite(close_pin, LOW);
    } else if (output_driver_mode == 2) { // CLOSE
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, HIGH);
    } else if (output_driver_mode == 4) { // open_main
      send_IO_data(1);
    } else if (output_driver_mode == 8) { // close_main
      send_IO_data(2);
    }
    set_LED_color(2);
  } else {

    duration = pulseIn(IO_DATA_pin, HIGH, PWM_period_us * 2); //timeout 10ms
    if (abs(duration - prev_duration) >= 10) {
      if (duration >= 80 && duration <= 20) {
        digitalWrite(open_pin, LOW);
        digitalWrite(close_pin, LOW);
        Serial.println("command changed, stopped for 200ms");
        delay(200);
      }
    }
    int val = 100 * duration / PWM_period_us; // %
    if (duration == 0) val = 50;
    if (val >= 80 && val < 95) {        // OPEN
      digitalWrite(open_pin, HIGH);
      digitalWrite(close_pin, LOW);
      set_LED_color(2);
    } else if (val <= 20 && val > 5) {  // CLOSE
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, HIGH);
      set_LED_color(2);
    } else {                            // STOP
      digitalWrite(open_pin, LOW);
      digitalWrite(close_pin, LOW);
      set_LED_color(0);;
    }
  }*/

  /*if (prev_button_mode != BUTTON_MODE) {
    digitalWrite(open_pin, LOW);
    digitalWrite(close_pin, LOW);
    Serial.println("button changed, stopped for 200ms");
    delay(200);
  }*/

  prev_button_mode = BUTTON_MODE;
  prev_duration = duration;

}