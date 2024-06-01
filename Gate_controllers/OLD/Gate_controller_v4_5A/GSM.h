char phone_no[]="+375298711942"; // номер на который высылается сообщение инициализации модуля
bool statusGSM = false;
String smsDataLine = "No info...";
bool globalValSMS = true;


bool getAnswerGSM () {
  String val = "";
  bool valBool = false;
  delay(500);
  if (Serial2.available()) {    
    while(Serial2.available()) {
      delayMicroseconds(200);
      val += char(Serial2.read());     
    }
  }
  Serial.println("~~~~~");
  if (val.indexOf("OK") > -1) {
    valBool = true;
    Serial.println("OK");
  } else {
    Serial.println("**");
  }
  return valBool;  
}

void sendSMS (String val, String phone_val) {
  if (statusGSM) {    
    Serial.println("Sending data to " + phone_val + "... ");                                            
    Serial2.print("AT+CMGS=\"");
    Serial2.print(phone_val); 
    Serial2.write(0x22);
    Serial2.write(0x0D);  // hex equivalent of Carraige return    
    Serial2.write(0x0A);  // hex equivalent of newline
    getAnswerGSM();
    Serial2.print(val); 
    delay(500);
    Serial2.println (char(26));//the ASCII code of the ctrl+z is 26   
    getAnswerGSM();
    Serial.println("done!"); 
    delay(500);
  } else {
    Serial.println("GSM ERROR!"); 
    delay(1000);  
  }
}

bool initGSM () {
  bool val = false;
  String valStr = "";
  Serial.println("GSM initialization... ");
  Serial2.begin(57600);
  //Serial2.println("AT+IPR=9600");// задавать скорость соединения
  for (int i = 0; i < 3; i++) {
  Serial2.println("AT");
  delay(2000);
  if (Serial2.available()) {
    delay(500);
    while(Serial2.available()) {
      valStr += char(Serial2.read()); 
      delayMicroseconds(10); 
    }
  }  
  Serial.println(valStr);
  }
  //statusGSM = true;
  if (statusGSM){
    Serial.println("done!");
    Serial2.println("AT+CMGF=1"); 
    getAnswerGSM();
    Serial2.println("AT+CNMI=2,2,0,0,0"); 
    getAnswerGSM();  
    Serial2.println("AT+CMGD=2,1"); 
    getAnswerGSM();
  } else {
    Serial.println("failed!");  
  }
  val = statusGSM;
  smsDataLine = "* Module initialized! *";
  sendSMS(smsDataLine, phone_no);
  return val;
}

void readDataSMS () {
  String inputData = "";
  String phoneNumber = "";
  bool numberConfirmed = false;
  String myNumber[10];
  myNumber[0] = "+375339011877";
  myNumber[1] = "+375298078699";
  myNumber[2] = "+375292237536";
  String infoSMS = "none";
  
  String phoneIndex = "+375";
  String fieldIndex = "Pole";
  String globalInfoIndex = "Info";
  String endIndex = "axx";
  byte indexLength = fieldIndex.length();
  String data = "";
  byte dataLength = 0;
  if (Serial2.available()) {
    while(Serial2.available()) {
      inputData += char(Serial2.read()); 
      delayMicroseconds(1); 
    }
    Serial.println(inputData);
    Serial.println("*****************");
  }
  while(Serial.available()) {
    delay(1); 
    Serial2.print(char(Serial.read()));   
  }
  
  if (inputData.indexOf("+CMT") > -1) {      
    phoneNumber = inputData.substring(inputData.indexOf(phoneIndex), inputData.indexOf(phoneIndex) + 13);
    bool numberConfirmed = false;
    for (int i = 0; i < 10; i++) if (!phoneNumber.compareTo(myNumber[i])) numberConfirmed = true;
    delay(2000);
    if (inputData.indexOf(fieldIndex) > -1 && inputData.indexOf(endIndex) > -1) {
      data = inputData.substring(inputData.indexOf(fieldIndex) + indexLength, inputData.indexOf(endIndex));
      dataLength = data.length();
      if (dataLength == 12) {    
        
      } else if (dataLength == 1) {
        
      } 
    } /*else if (inputData.indexOf(globalInfoIndex) > -1 && inputData.indexOf(endIndex) > -1) {
      if (pompStat) infoSMS = globalInfo() + "POMP ON * " + checkPomp() + " *"; else infoSMS = globalInfo() + "POMP OFF *";
    } else if (inputData.indexOf("Stop") > -1 && inputData.indexOf(endIndex) > -1) {
      for (int i = 0; i < maxFields; i++) field[i].GSM_stop();
      infoSMS = "POLE STOP";
    } else if (inputData.indexOf("Start") > -1 && inputData.indexOf(endIndex) > -1) {
      for (int i = 0; i < maxFields; i++) field[i].GSM_start();
      infoSMS = "POLE START";      
      pomp_global_error = false;
      globalErrorPompMillis = millis();
    } else if (inputData.indexOf("ON") > -1 && inputData.indexOf(endIndex) > -1) {
      infoSMS = "Security ON"; 
      security_stat = true;  
    } else if (inputData.indexOf("OFF") > -1 && inputData.indexOf(endIndex) > -1) {
      infoSMS = "Security OFF";  
      security_stat = false;
    } */

    if (!infoSMS.equals("none")) sendSMS (infoSMS, phoneNumber);
    Serial2.println("AT+CMGD=1"); 
    getAnswerGSM();  
  }
}