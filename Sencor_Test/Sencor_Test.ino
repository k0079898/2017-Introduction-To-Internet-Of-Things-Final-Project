/*
#include "LDHT.h"
#include <LBattery.h>

#define BAUDRATE 19200
#define DHTPIN 7          // what pin we're connected to
#define DHTTYPE DHT22     // using DHT22 sensor

LDHT dht(DHTPIN, DHTTYPE);

float tempC = 0.0, Humi = 0.0;
char readcharbuffer[20];
int readbuffersize;
char temp_input;

void setup(){

  Serial.begin(9600);
  Serial1.begin(9600);

	dht.begin();
  Serial.begin(BAUDRATE);

	Serial.print(DHTTYPE);
	Serial.println(" test!");
}
void loop(){
  
    Serial.print("Battery level is ");
    Serial.println(LBattery.level());
    Serial.print("Charging: ");
    Serial.println(LBattery.isCharging() ? "yes" : "no");
    delay(5000);



		Serial.println("------------------------------");
		Serial.print("Temperature Celcius = ");
		Serial.print(dht.readTemperature());
		Serial.println("C");

		Serial.print("Temperature Fahrenheit = ");
		Serial.print(dht.readTemperature(false));
		Serial.println("F");

		Serial.print("Humidity = ");
		Serial.print(dht.readHumidity());
		Serial.println("%");

		Serial.print("HeatIndex = ");
		Serial.print(dht.readHeatIndex(tempC, Humi));
		Serial.println("C");

		Serial.print("DewPoint = ");
		Serial.print(dht.readDewPoint(tempC, Humi));
		Serial.println("C");

		Serial.println("Ready to Send");
		Serial.print("AT+DTX=11,\"T");
		Serial.print(dht.readHeatIndex(tempC, Humi));
		Serial.print(dht.readHumidity());
		Serial.println("\"");

//		Serial1.print("AT+DTX=11,\"T");
//		Serial1.print(dht.readHeatIndex(tempC, Humi));
//		Serial1.print(dht.readHumidity());
//		Serial1.println("\"");

		Serial1.println("AT+DTX=11,\"T1234567890\"");
	

  if(Serial1.read()){
    Serial.print(Serial1.read());
    Serial1.print("AT+DRX?");
  }
  
	delay(1000);

	/*
  Serial.println("Ready to Send");
  Serial1.println("AT+DTX=11,\"12345ABCdef\"");
  delay(1000);*/ /*
  readbuffersize = Serial1.available();
  while(readbuffersize){
    temp_input = Serial1.read();
    Serial.print(temp_input);
    readbuffersize--;
 }
 delay(9000);
   readbuffersize = Serial1.available();
  while(readbuffersize){
    temp_input = Serial1.read();
    Serial.print(temp_input);
    readbuffersize--;
 }
 Serial.println("things");
  delay(10000);
}

*/
#include <LBattery.h>
#include <Grove_LED_Bar.h>
#include <LGPS.h>

//LED
Grove_LED_Bar bar(9, 8, 0);
int LED_BAR = 0;
int LED_FLASH = 0;
unsigned long led_timer;
unsigned long led_sampletime_ms = 1000;  //1s
//Dust
int Dust_PIN = 2;
unsigned long duration;
unsigned long dust_timer;
unsigned long dust_sampletime_ms = 30000;  //30s
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
//GPS
unsigned long gps_timer;
unsigned long gps_sampletime_ms = 1000;  //1s
gpsSentenceInfoStruct info;
char buff[256];

static unsigned char getComma(unsigned char num,const char *str)
{
  unsigned char i,j = 0;
  int len=strlen(str);
  for(i = 0;i < len;i ++)
  {
     if(str[i] == ',')
      j++;
     if(j == num)
      return i + 1; 
  }
  return 0; 
}

static double getDoubleNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atof(buf);
  return rev; 
}

static double getIntNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atoi(buf);
  return rev; 
}

void parseGPGGA(const char* GPGGAstr)
{
  /* Refer to http://www.gpsinformation.org/dale/nmea.htm#GGA
   * Sample data: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
   * Where:
   *  GGA          Global Positioning System Fix Data
   *  123519       Fix taken at 12:35:19 UTC
   *  4807.038,N   Latitude 48 deg 07.038' N
   *  01131.000,E  Longitude 11 deg 31.000' E
   *  1            Fix quality: 0 = invalid
   *                            1 = GPS fix (SPS)
   *                            2 = DGPS fix
   *                            3 = PPS fix
   *                            4 = Real Time Kinematic
   *                            5 = Float RTK
   *                            6 = estimated (dead reckoning) (2.3 feature)
   *                            7 = Manual input mode
   *                            8 = Simulation mode
   *  08           Number of satellites being tracked
   *  0.9          Horizontal dilution of position
   *  545.4,M      Altitude, Meters, above mean sea level
   *  46.9,M       Height of geoid (mean sea level) above WGS84
   *                   ellipsoid
   *  (empty field) time in seconds since last DGPS update
   *  (empty field) DGPS station ID number
   *  *47          the checksum data, always begins with *
   */
  double latitude;
  double longitude;
  int tmp, hour, minute, second, num ;
  if(GPGGAstr[0] == '$')
  {
    tmp = getComma(1, GPGGAstr);
    hour     = (GPGGAstr[tmp + 0] - '0') * 10 + (GPGGAstr[tmp + 1] - '0');
    minute   = (GPGGAstr[tmp + 2] - '0') * 10 + (GPGGAstr[tmp + 3] - '0');
    second    = (GPGGAstr[tmp + 4] - '0') * 10 + (GPGGAstr[tmp + 5] - '0');
    
    sprintf(buff, "UTC timer %2d-%2d-%2d", hour, minute, second);
    Serial.println(buff);
    
    tmp = getComma(2, GPGGAstr);
    latitude = getDoubleNumber(&GPGGAstr[tmp]);
    tmp = getComma(4, GPGGAstr);
    longitude = getDoubleNumber(&GPGGAstr[tmp]);
    sprintf(buff, "latitude = %10.4f, longitude = %10.4f", latitude, longitude);
    Serial.println(buff); 
    
    tmp = getComma(7, GPGGAstr);
    num = getIntNumber(&GPGGAstr[tmp]);    
    sprintf(buff, "satellites number = %d", num);
    Serial.println(buff); 
  }
  else
  {
    Serial.println("Not get data"); 
  }
}

void setup(){
  Serial.begin(9600);
  bar.begin();
  led_timer = millis();
  pinMode(Dust_PIN,INPUT);
  dust_timer = millis();
  gps_timer = millis(); 
  LGPS.powerOn();
}

void loop(){
  //Serial.print("Battery level is ");
  //Serial.println(LBattery.level());
  //Serial.print("Charging: ");
  //Serial.println(LBattery.isCharging() ? "yes" : "no");
  if(LBattery.isCharging() == 1 && ((millis()-led_timer) >= led_sampletime_ms))
  {
      LED_FLASH = 0; 
      if(LED_BAR < 10)  bar.setLevel(LED_BAR++);
      else{
        LED_BAR = 0;
        bar.setLevel(10);
      }
      led_timer = millis();
  }else if(LBattery.isCharging() == 0 && ((millis()-led_timer) >= led_sampletime_ms))
  {
      if(LBattery.level() == 0)
      {
        LED_BAR = 1;
        if(LED_FLASH == 0)
        {
          LED_FLASH = 1;
          bar.setLevel(0);
        }else
        {
          LED_FLASH = 0; 
          bar.setLevel(LED_BAR);
        }
      }else if(LBattery.level() == 33)
      {
        LED_BAR = 3;
        if(LED_FLASH == 0)
        {
          LED_FLASH = 1;
          bar.setLevel(0);
        }else
        {
          LED_FLASH = 0; 
          bar.setLevel(LED_BAR);
        }
      }else if(LBattery.level() == 66)
      {
        LED_BAR = 7;
        LED_FLASH = 0; 
        bar.setLevel(LED_BAR);
      }else if(LBattery.level() == 100)
      {
        LED_BAR = 10;
        LED_FLASH = 0; 
        bar.setLevel(LED_BAR);
      }
      led_timer = millis();
  }
  duration = pulseIn(Dust_PIN, LOW);
  lowpulseoccupancy = lowpulseoccupancy + duration;
  if ((millis()-dust_timer) >= dust_sampletime_ms)
  {
    ratio = lowpulseoccupancy/(dust_sampletime_ms*10.0);  // Integer percentage 0=&gt;100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    //Serial.print("concentration = ");
    //Serial.print(concentration);
    //Serial.println(" pcs/0.01cf");
    //Serial.println("\n");
    lowpulseoccupancy = 0;
    dust_timer = millis();
  }
  LGPS.getData(&info);
  Serial.println((char*)info.GPGGA); 
  if ((millis()-gps_timer) >= gps_sampletime_ms)
  {
    parseGPGGA((const char*)info.GPGGA);
    gps_timer = millis();
  }
}
