//new timer 2.0

class svsTimer {
  public:  
  svsTimer (){};
  void reset();
  bool ready(long);
  long time_left();
  
  private:
  bool timer_stat = true;
  unsigned long prev_millis;
  bool ready_stat = false;
};

void svsTimer ::reset() {
  prev_millis = millis();
}

bool svsTimer ::ready(long val) {
    long d_millis = millis() - prev_millis;
    if (d_millis >= val) {
    ready_stat = true;
    reset();
  }  
  bool out_val = ready_stat;
  ready_stat = false;
  return out_val;
}

long svsTimer ::time_left() {
  long d_millis = millis() - prev_millis;
  return d_millis;
}
