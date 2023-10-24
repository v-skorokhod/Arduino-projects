class my_timer {
  public:  
  my_timer(){};
  void timer_init(bool);
  void timer_reset();
  bool timer_ready();
  void timer(long);
  bool stat = false;
  long time_left ();
  
  private:
  bool timer_stat = false;
  unsigned long prev_millis;
  bool serial_print_stat = false;
};

void my_timer::timer_init(bool val_1 = true) {
  serial_print_stat = val_1;
}

void my_timer::timer_reset() {
  timer_stat = true;
  prev_millis = millis();
}

bool my_timer::timer_ready() {
  return !timer_stat;
}

long my_timer::time_left() {
  long d_millis = millis() - prev_millis;
  return d_millis;
}

void my_timer::timer(long val) { 
  unsigned long d_millis = millis() - prev_millis;
  if (!stat) {
    timer_reset();
  }
  if (timer_stat && d_millis >= val) {
    timer_stat = false;
  }  
  if (serial_print_stat) Serial.println("d_miliis: " + String(d_millis));
}
