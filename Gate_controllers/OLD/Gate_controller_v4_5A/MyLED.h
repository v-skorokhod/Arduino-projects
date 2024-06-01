#define LED_pin A0
#define LED_WORK_PERIOD 1000
#define LED_ERROR_PERIOD 200
svsTimer LED_tm;
bool LED_STAT = false;
long LED_period;

void set_LED_color (int val) {
  //LED_tm.init(1);
  switch (val) {
    case 0: // off
      LED_STAT = false;
      LED_tm.reset();
      break;
    case 1: // on
      LED_STAT = true;
      LED_tm.reset();
      break;
    case 2: // work
      if (LED_tm.ready(LED_period)) LED_STAT = !LED_STAT;
      LED_period = LED_WORK_PERIOD;
      break;
    case 3: // error
      if (LED_tm.ready(LED_period)) LED_STAT = !LED_STAT;
      LED_period = LED_ERROR_PERIOD;
      break;
    case 4: // high current
      if (LED_tm.ready(LED_period)) LED_STAT = !LED_STAT;
      if (LED_STAT) LED_period = LED_WORK_PERIOD;
      else LED_period = LED_ERROR_PERIOD;
      break;  
  }  
  digitalWrite(LED_pin, LED_STAT);
}
