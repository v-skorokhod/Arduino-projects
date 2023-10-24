#pragma once

class svsButtons {
  public:  
  svsButtons(int *array, byte length){
    length = length / sizeof(int);
    MAX_BUTTONS = length;
    button_pin = new int[length];
    for (int i = 0; i < length; i++) {
      pinMode(array[i], INPUT_PULLUP);
      button_pin[i] = array[i];
    }
  };
  int pressed();
  void init(int *array, byte);
  
  private:
  int MAX_BUTTONS = 1;
  int* button_pin; 
};


int svsButtons::pressed() {
  int output_val;
  for (int i = 0; i < MAX_BUTTONS; i++) bitWrite(output_val, i, !digitalRead(button_pin[i]));
  return output_val;
}

/*byte read_buttons () {
  byte val = 0;
  byte output_val = 0;
  button_tm.timer(DOUBLECLICK_TIMEOUT);
  for (int i = 0; i < 4; i++) bitWrite(val, i, digitalRead(button_pin[i])); //1 - 2 - 4 - 8
  if (val) {
    if (last_pressed_button == val && prev_button_stat == 0) double_pressed = true;
    else button_tm.reset();
    last_pressed_button = val;
  }
  if (button_tm.ready()) {
    if (double_pressed) output_val = last_pressed_button * 10;
    else output_val = last_pressed_button;
    double_pressed = false;
    last_pressed_button = 0;
  }
  prev_button_stat = val;
  return output_val;  
}*/
