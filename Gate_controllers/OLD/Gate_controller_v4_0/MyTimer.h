class my_timer {
  public:  
  my_timer(){};
  void init(bool);
  void reset();
  bool ready();
  void timer(long);
  long time_left ();
  
  private:
  bool timer_stat = true;
  unsigned long prev_millis;
  bool serial_print_stat = false;
  bool ready_stat = false;
};

void my_timer::init(bool val_1 = true) {
  serial_print_stat = val_1;
}

void my_timer::reset() {
  prev_millis = millis();
}

bool my_timer::ready() {
  bool out_val = ready_stat;
  ready_stat = false;
  return out_val;
}

long my_timer::time_left() {
  long d_millis = millis() - prev_millis;
  return d_millis;
}

void my_timer::timer(long val) { 
  long d_millis = millis() - prev_millis;
  if (d_millis >= val) {
    ready_stat = true;
    reset();
  }  
  if (serial_print_stat) Serial.println("d_miliis: " + String(d_millis));
}
