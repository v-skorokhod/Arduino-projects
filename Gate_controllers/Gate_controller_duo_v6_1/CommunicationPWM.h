byte IO_DATA_pin;
#define PWM_freq 490 // Hz
long PWM_period_us;
#define STOP_PWM 60 // %
#define OPEN_PWM 80 // %
#define CLOSE_PWM 20 // %
#define WICKET_PWM 40 // %
#define PWM_DURATION 500 // ms
#define d_PWM 3 //%
#define no_data_val 13

svsTimer pwm_dur_tm;
bool IO_send_mode = false;
long duration;

int read_IO_data () {
  long val;
  if (!IO_send_mode) {
    duration = pulseIn(IO_DATA_pin, HIGH, PWM_period_us * 2); 
    val = 100 * duration / PWM_period_us; // %
    if (val > 5) Serial.println(val);
    if (val > STOP_PWM - d_PWM && val < STOP_PWM + d_PWM) return 8;
    else if (val > OPEN_PWM - d_PWM && val < OPEN_PWM + d_PWM) return 1;
    else if (val > CLOSE_PWM - d_PWM && val < CLOSE_PWM + d_PWM) return 2;
    else if (val > WICKET_PWM - d_PWM && val < WICKET_PWM + d_PWM) return 4;
    else return no_data_val;
  } else {
    return no_data_val;
  }
}

void send_IO_data (int val) {
  IO_send_mode = true;
  pinMode(IO_DATA_pin, OUTPUT);
  int output_val;
  switch (val) {
    case 8: // stop 
      output_val = 255 * STOP_PWM / 100;
      analogWrite(IO_DATA_pin, output_val);
      break;
    case 1: // open 
      output_val = 255 * OPEN_PWM / 100;
      analogWrite(IO_DATA_pin, output_val);
      break;
    case 2: // close
      output_val = 255 * CLOSE_PWM / 100;
      analogWrite(IO_DATA_pin, output_val);
      break;
    case 4: // wicket 
      output_val = 255 * WICKET_PWM / 100;
      analogWrite(IO_DATA_pin, output_val);
      break; 
  } 
}

void refresh_IO_pin () {
  if (!IO_send_mode) {
    pwm_dur_tm.reset();
  } else {
    if (pwm_dur_tm.ready(PWM_DURATION)) {
      IO_send_mode = false;
      analogWrite(IO_DATA_pin, 0);
      pinMode(IO_DATA_pin, INPUT);
    }
  }
}