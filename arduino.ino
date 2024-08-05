
#include <FirebaseArduino.h> // Gui du lieu len Firebase
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h> // LIB use for realtime
#include <NTPClient.h> // LIB use for realtime
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h" 
#define DHTTYPE DHT11
LiquidCrystal_I2C lcd(0x27,16,2);
// SCL -D1, SDA-D2
#define Led_test D0
#define flame_ss D3
#define BT2 D5
#define BT3 D6
#define Pump_Machine D7
#define Fan_pin D8
#define Gas_Sensor A0


//#define gas_value 315
#define dht_dpin 2 // D4

#define FIREBASE_HOST "gas-fire-alarm-default-rtdb.firebaseio.com"     
#define FIREBASE_AUTH "EQbuE2jdBo28gec7FjgYzALNOryuoykMomL4L3xt"  
DHT dht(dht_dpin, DHTTYPE);

#define WIFI_SSID "nhat" // change your WiFi name
#define WIFI_PASSWORD "nhat1234"  //change your Password  WiFi

float temp_c,old_temp = 0 ;
float gas_value, old_gas_value = 0 ;
unsigned long time_check = 0 ;
WiFiClient client;


//Declare
float temp_point = 0 ,gas_point = 0  ;
signed int check_box1=0,check_box2=0,check_box3=0;

String real_time;
signed int sec,minu,hrs;
long time_process = 0;
int display_time=0,process_control_sendata= 0,process_control = 0 ;
boolean flag_turn_on_pump = 0;
boolean flag_turn_on_fan = 0;
boolean flag_send_data = 0,old_flag_pump = 0,old_flag_fan = 0;
boolean flag_follow_condition = 0;
int flag_check_rain_ss = 2,old_flag_rain = 5, flag_check_time = 0 ;
String convert_send;
String data;

int manual_flag_pump = 0 ,manual_flag_fan = 0;

WiFiUDP u;
NTPClient n(u,"3.vn.pool.ntp.org",7*3600); 

void blink_led(byte number);
void display_actual();
void display_setpoint();
int read_gas_value();
void convert_time_h_m(String get_time,int *_hour,int *_minute);
void get_real_time(int _timeout,int *_hour,int *_minute,int *_sec);
void get_firebase();
void control_pump();
void check_conditions_on_pump();
void check_gas_sensor();
void ini_process();
void auto_process();
void manual_process();

void setup() {
  Serial.begin(9600);
  pinMode(Led_test, OUTPUT);
  pinMode(Pump_Machine, OUTPUT);
  pinMode(Fan_pin, OUTPUT);
  pinMode(flame_ss, INPUT);
  pinMode(BT2, INPUT_PULLUP);
  pinMode(BT3, INPUT_PULLUP);

lcd.begin(); 


  lcd.backlight();
  lcd.print("Hello world    ");
  dht.begin();
  if (digitalRead(BT3)== 0)
  {
    lcd.setCursor(0,0);
    lcd.print("Manual Mode");     
    process_control = 5; // go to Manual Mode
    digitalWrite(Led_test,1);
    digitalWrite(Pump_Machine,LOW);
    digitalWrite(Fan_pin,LOW);
    while( digitalRead(BT3) == 0) {delay(100);} 
  }
  else
  {
      // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED)
    {
      lcd.setCursor(0,1);
      lcd.print("Connecting Wifi       ");
      Serial.print(".");
      delay(500);
      process_control++;
      if (process_control == 15)
      {
        break;
      }
    }
  if ( process_control <15)
  {
  process_control = 0;
  lcd.setCursor(0,1);
  lcd.print("Connected           ");
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); 
  Firebase.stream("/GetData");
  n.begin();
  n.update();
  real_time= n.getFormattedTime();
  blink_led(4);
  }
  else
    {
    lcd.setCursor(0,0);
    lcd.print("Can not connect");
    lcd.setCursor(0,1);
    lcd.print("internet !!!        ");
    digitalWrite(Led_test,1);
    delay(2000);
    lcd.clear();
    process_control = 5;   
    }   
  }
}


void loop() 
{
  
  switch (process_control)
  {
    case 0:
      ini_process();
      break;
    case 4:
      auto_process();
      break;
    case 5:
      manual_process();
      break;
  }
}


void blink_led(byte number)
{
  for (int i = 0 ; i<number;i++ )
  {
    digitalWrite(Led_test,1);
    delay(200);
    digitalWrite(Led_test,0);
    delay(200);
  }
}
void display_actual()
{
  lcd.setCursor(0,0);
  lcd.print("Thuc te:"); 
  lcd.print(hrs);
  lcd.print(":");
  lcd.print(minu);
  lcd.print("        ");

  lcd.setCursor(0,1);
  //lcd.print(round(temp_c));
  lcd.print("T=");
  lcd.print((int)temp_c);
  lcd.print("*C-");
  lcd.print("Gas=");
  //lcd.print(round(gas_value));
  lcd.print((int)gas_value);
  lcd.print("      ");
}

void display_setpoint()
{
  lcd.setCursor(0,0);
  lcd.print("Cai Dat:            ");  
  lcd.setCursor(0,1);
  lcd.print("T=");
  lcd.print((int)temp_point);
  lcd.print("*C-");
  lcd.print("Gas=");
  lcd.print((int)gas_point);
  lcd.print("      ");  
}
void display_mode()
{
  if (check_box1 == 1)
  {
  lcd.setCursor(0,0);
  lcd.print("Che do tu dong   ");    
  lcd.setCursor(0,1); 
  lcd.print("                        "); 
  digitalWrite(Led_test,HIGH);    
  }
  else
  {
  lcd.setCursor(0,0);
  lcd.print(" Che do dieu         ");    
  lcd.setCursor(0,1); 
  lcd.print("khien bang tay       ");
  digitalWrite(Led_test,LOW);         
    }

}

void convert_time_h_m(String get_time,int *_hour,int *_minute)
{
  byte moc[3];byte count_moc=0;
  String chuoi1, chuoi2;
  get_time.remove(0,1); 
  get_time.remove((get_time.length()-1),1); 
    for (int i = 0 ; i<get_time.length();i++)
  {
    if (get_time.charAt(i) == ':')
    {
      moc[count_moc] = i;
      count_moc++;
    }  
  }
  chuoi1 = get_time; 
  chuoi2 = get_time; 
  chuoi1.remove(moc[0]); 
  chuoi2.remove(0,moc[0]+1);
  *_hour = chuoi1.toInt();
  *_minute = chuoi2.toInt();
  //Serial.print(*_hour );Serial.print("-" );Serial.println(*_minute );    
}
void get_real_time(int _timeout,int *_hour,int *_minute,int *_sec)
{
   if (millis()- time_process > _timeout)
  {
  time_process = millis();
  n.update();
  real_time= n.getFormattedTime();  // Get time from Internet
  byte moc[3];byte count_moc=0;
  String chuoi1, chuoi2,chuoi3;

  //Data is string, type: 12:20:35 => Convert to int
  for (int i = 0 ; i< real_time.length();i++)
  {
    if (real_time.charAt(i) == ':')
    {
      moc[count_moc] = i;
      count_moc++;
    }  
  }
  chuoi1 = real_time;
  chuoi2 = real_time;
  chuoi3 = real_time;
  chuoi1.remove(moc[0]); 
  chuoi2.remove(0,moc[0]+1);
  chuoi3.remove(0,moc[1]+1);
  *_hour = chuoi1.toInt();
  *_minute = chuoi2.toInt();
  *_sec = chuoi3.toInt();
  }
}
void get_firebase() // Lấy dữ liệu từ Firebase
{ 
  if (Firebase.available()) // Nếu fire có sự thay đổi dữ liệu    
  {
    FirebaseObject event = Firebase.readEvent();
    String eventType = event.getString("type");
    eventType.toLowerCase();
    if (eventType == "put") 
      {
        String path = event.getString("path");
        data = event.getString("data");
        if (path == "/pump")
        {
          if (data == "0")
          {
          flag_turn_on_pump = 0 ;
          }
          if (data == "1")
          {
           flag_turn_on_pump = 1;
          }
        }
        if (path == "/fan")
        {
          if (data == "0")
          {
          flag_turn_on_fan = 0 ;
          }
          if (data == "1")
          {
           flag_turn_on_fan = 1;
          }
        }        
    if (path == "/SetHumi")
        {
          data.remove(0,1); 
          data.remove((data.length()-1),1);
          gas_point =  data.toFloat();      
        }
    if (path == "/SetTemp")
        {
          data.remove(0,1); 
          data.remove((data.length()-1),1);
      temp_point =  data.toFloat();         
        }
    if (path == "/CheckBox1")
        {
      data.remove(0,1); 
      data.remove((data.length()-1),1); 
      check_box1 = data.toInt();      
        }
    if (path == "/CheckBox2")
        {
      data.remove(0,1); 
      data.remove((data.length()-1),1); 
      check_box2 = data.toInt();      
        }
    if (path == "/CheckBox3")
        {
      data.remove(0,1); 
      data.remove((data.length()-1),1); 
      check_box3 = data.toInt();      
        }     
      }
  }
}
void control_pump()
{
  if (old_flag_pump == flag_turn_on_pump)
  {}
  else
  {
     
    old_flag_pump = flag_turn_on_pump;
    convert_send = String(old_flag_pump);
    Firebase.setString("/PumpStatus", convert_send);
    delay(50);
    Firebase.setString("/PumpStatus", convert_send);
    delay(50);
    Firebase.setString("/PumpStatus", convert_send);
    if (old_flag_pump == 1 )
    {
      digitalWrite(Pump_Machine,HIGH); 
      Firebase.setString("/GetData/pump", "1");         
    }
      
    if (old_flag_pump == 0)
    {
      Firebase.setString("/GetData/pump", "0"); 
      digitalWrite(Pump_Machine,LOW);
    }   
  }
  
}
void control_fan()
{
  if (old_flag_fan == flag_turn_on_fan)
  {}
  else
  {
    Serial.println("Doi trang thai ");
  //  delay(5000);
    old_flag_fan = flag_turn_on_fan;
    convert_send = String(old_flag_fan);
    Serial.println("Bat Quat");
    Firebase.setString("/FanStatus", convert_send);
    delay(50);
    Firebase.setString("/FanStatus", convert_send);
    delay(50);
    Firebase.setString("/FanStatus", convert_send);
    if (old_flag_fan == 1 )
    {
      digitalWrite(Fan_pin,HIGH);  
      Firebase.setString("/GetData/fan","1") ; 
    }
      
    if (old_flag_fan == 0)
    {
      digitalWrite(Fan_pin,LOW);
      Firebase.setString("/GetData/fan","0");  
    }   
  }
    
  }
void check_conditions_on_pump()
{
  if (temp_c >temp_point && digitalRead(flame_ss) == 0 )
  {
    //Firebase.setString("/GetData/pump", "1");  
    flag_turn_on_pump = 1;
    flag_follow_condition = 1;      
  } 
  else
  {
    flag_turn_on_pump = 0;
    flag_follow_condition = 0;
    //Firebase.setString("/GetData/pump", "0");           
  }
}
void check_conditions_on_fan()
{
  if (read_gas_value() > gas_point )
  {
    //Firebase.setString("/GetData/fan","1");  
    flag_turn_on_fan = 1;
  } 
  else
  {
    flag_turn_on_fan = 0;
    //Firebase.setString("/GetData/fan", "0");           
  }
}
void check_gas_sensor()
{
  Serial.print("Gas point");Serial.println(gas_point);
  if(read_gas_value() >  gas_point) // Rain On
  {
    flag_check_rain_ss = 1;
    flag_turn_on_pump =0;
  // Serial.print("Troi Mua");
  }
  else
  {
    flag_check_rain_ss =0;
  }
  if (flag_check_rain_ss != old_flag_rain)
  {
    old_flag_rain = flag_check_rain_ss;
    convert_send = String(old_flag_rain);
    Firebase.setString("/RainSensor1", convert_send);     
  }
}

void ini_process() //0
{
  FirebaseObject fbo = Firebase.get("/GetData");
  lcd.setCursor(0,0);
  lcd.print("Waitting              "); 
  lcd.setCursor(0,1);
  lcd.print("Ini Process ....      "); 
  
  data = fbo.getString("CheckBox1");
  data.remove(0,1); 
  data.remove((data.length()-1),1); 
  check_box1 = data.toInt();
//  Serial.println(check_box1);
  delay(200);
  data = fbo.getString("CheckBox2");
  data.remove(0,1); 
  data.remove((data.length()-1),1); 
  check_box2 = data.toInt();
//  Serial.println(check_box2);
  delay(200);
  data = fbo.getString("CheckBox3");
  data.remove(0,1); 
  data.remove((data.length()-1),1); 
  check_box3 = data.toInt();
//  Serial.println(check_box3);
  
  delay(200);
  data = fbo.getString("SetHumi");
  data.remove(0,1); 
  data.remove((data.length()-1),1);
  gas_point =  data.toFloat();
 
  delay(200);
  data = fbo.getString("SetTemp");
  data.remove(0,1); 
  data.remove((data.length()-1),1);
  temp_point =  data.toFloat(); 
  delay(200);
 
  data = fbo.getString("pump");
  if (data == "0")
    {
    flag_turn_on_pump = 0 ;
    }
    if (data == "1")
    {
    flag_turn_on_pump = 1;
    }
    delay(200);  
  
  process_control = 4;
  
}
void auto_process() // 4
{
    get_real_time(10000,&hrs, &minu,&sec);
    if (millis() - time_check >8000)
    {
      time_check = millis(); 
      temp_c = dht.readTemperature();
      if (temp_c > 100 || temp_c < -5)
      {
        temp_c = old_temp;
      }
      gas_value = read_gas_value();
      
      if((temp_c<100) && (temp_c > -5) )
      {
        if (   (abs(temp_c-old_temp) > 0.5)    ||     (abs( gas_value - old_gas_value) > 3)    )
        {
        flag_send_data = 1;
        old_temp = temp_c;
        old_gas_value = gas_value;
        }
        if (flag_send_data == 1 )
        {
         process_control_sendata++;
        switch(process_control_sendata)
         {
        case 1:
        process_control_sendata =1;     
        break;
        case 2:
        Firebase.setFloat("/ActualHumi1", gas_value);
        break;
        case 3: 
        process_control_sendata =3;  
         break;  
        case 4: 
        Firebase.setFloat("/ActualTemp1", temp_c);               
        break;                     
         }
        if(process_control_sendata == 4)
        {
        process_control_sendata=0;  
        flag_send_data = 0;       
        }  
        }   
      }
    }    
  if (check_box1 == 0 ) // Manual mode
  {  
      
  }
  else       // auto mode
  {
    //Serial.print("Auto mode");  
    check_conditions_on_pump(); 
    check_conditions_on_fan();
  }   
    get_firebase();
    control_pump();
    control_fan();

    
  // Hiển thị LCD ////
    display_time++;
    if (display_time<60) display_actual();
    if (display_time>=60 && display_time < 120 )
    {
      display_setpoint();
    }
    if (display_time >= 120)
    {
      display_mode();
      if (display_time>=180)display_time=0;  
    }
  /////////////////////
  
  
  if (digitalRead(BT2) == 0&& digitalRead(BT3) == 1)
  {
    check_box1 = 0;
    Firebase.setString("/CheckBox4", "0");
    if (flag_turn_on_pump == 0   )
    {
      digitalWrite(Pump_Machine,HIGH);
      flag_turn_on_pump = 1 ;
      manual_flag_pump = 1;
      Firebase.setString("/GetData/pump", "1"); 
    }
    else
    {
      digitalWrite(Pump_Machine,LOW);
      flag_turn_on_pump = 0 ;
      manual_flag_pump = 0;  
      Firebase.setString("/GetData/pump", "0");     
    }
    while( digitalRead(BT2) == 0) {delay(100);}  
  }
  
  if (digitalRead(BT3) == 0 && digitalRead(BT2) == 1)
  { 
    Firebase.setString("/CheckBox4", "0");
    check_box1 = 0;
    if (flag_turn_on_fan == 0   )
    {
      digitalWrite(Fan_pin,HIGH);
      flag_turn_on_fan = 1 ;
      manual_flag_fan = 1;
      Firebase.setString("/GetData/fan", "1"); 
    }
    else
    {
      digitalWrite(Fan_pin,LOW);
      flag_turn_on_fan = 0 ;
      manual_flag_fan = 0;  
      Firebase.setString("/GetData/fan", "0");     
    }
    while( digitalRead(BT3) == 0) {delay(100);}   
  }

  if (digitalRead(BT2) == 0 && digitalRead(BT3) == 0 )
  { 
    Firebase.setString("/CheckBox4", "1");
    check_box1 = 1;
    while( digitalRead(BT3) == 0) {delay(100);}   
  }
  
}

void manual_process()
{
  lcd.setCursor(0,0);
  lcd.print("Manual Mode"); 
  
    if (millis() - time_check >2000)
  {
    time_check = millis(); 
    temp_c = dht.readTemperature();
    gas_value = read_gas_value();
    lcd.setCursor(0,1);
    lcd.print("T:");
    lcd.print((int)temp_c);
    lcd.print("*C-");
    lcd.print("Gas:");
    lcd.print((int)gas_value);
    lcd.print("    ");
    digitalWrite(Led_test,flag_turn_on_pump);
    flag_turn_on_pump = !flag_turn_on_pump;
   
  }
  if (digitalRead(BT2) == 0)
  { 
    while( digitalRead(BT2) == 0) {delay(100);} 
    if (manual_flag_pump == 0   )
    {
      digitalWrite(Pump_Machine,HIGH);
      manual_flag_pump = 1;
    }
    else
    {
      digitalWrite(Pump_Machine,LOW);
      manual_flag_pump = 0;      
    }
 
  }
  if (digitalRead(BT3) == 0 )
  {
    while( digitalRead(BT3) == 0) {delay(100);} 
    if (manual_flag_fan == 0   )
    {
      digitalWrite(Fan_pin,HIGH);
      manual_flag_fan = 1;
    }
    else
    {
      digitalWrite(Fan_pin,LOW);
      manual_flag_fan = 0;      
    }         
  }   
}
int read_gas_value()
{
  //Serial.print("Gas Sensor: ");Serial.println(analogRead(Gas_Sensor));
  int kq;
  kq = analogRead(Gas_Sensor);
  return kq;
}
