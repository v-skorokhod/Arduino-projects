#define LED_pin A0
#define LED_WORK_PERIOD 1000
#define LED_ERROR_PERIOD 200
my_timer LED_tm;
bool LED_STAT = false;
long LED_period;

void set_LED_color (int val) {
  //LED_tm.init(1);
  LED_tm.timer(LED_period);
  switch (val) {
    case 0: // stop
      LED_STAT = false;
      LED_tm.reset();
      break;
    case 1: // open by timer
      LED_STAT = true;
      LED_tm.reset();
      break;
    case 2: // close by timer
      if (LED_tm.ready()) LED_STAT = !LED_STAT;
      LED_period = LED_WORK_PERIOD;
      break;
    case 3: // open rigth gate by timer
      if (LED_tm.ready()) LED_STAT = !LED_STAT;
      LED_period = LED_ERROR_PERIOD;
      break;
    case 4: // open rigth gate by timer
      if (LED_tm.ready()) LED_STAT = !LED_STAT;
      if (LED_STAT) LED_period = LED_WORK_PERIOD;
      else LED_period = LED_ERROR_PERIOD;
      break;  
  }  
  digitalWrite(LED_pin, LED_STAT);
}
