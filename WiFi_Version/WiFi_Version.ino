//#include <b64.h>
#include <HttpClient.h>
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <LDateTime.h>
#include <LBattery.h>
#include <Grove_LED_Bar.h>
#include <LGPS.h>

#define WIFI_AP "Mark's iPhone"
#define WIFI_PASSWORD "66597797"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.
#define per 50
#define per1 3
#define DEVICEID "Dbt2Ucnn" // Input your deviceId
#define DEVICEKEY "3c06pPZCytVf0tHe" // Input your deviceKey
#define SITE_URL "api.mediatek.com"

LWiFiClient c;
char port[4]={0};
char connection_info[21]={0};
char ip[21]={0};             
int portnum;
String tcpdata = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
String upload_led;
String upload_data;
String upload_gps;
String upload_dust;
String upload_LL;
char bufLONGITUDE[10];
char bufLATITUDE[10];
char bufD[6];
String tcpcmd_led_on = "LED_Control,1";
String tcpcmd_led_off = "LED_Control,0";
LWiFiClient c2;
HttpClient http(c2);
unsigned long HB_timer;
unsigned long HB_sampletime_ms = 90000;  //90s
unsigned long UL_timer;
unsigned long UL_sampletime_ms = 60000;  //30s

//LED
Grove_LED_Bar bar(9, 8, 0);
int LED_BAR = 0;
int LED_FLASH = 0;
unsigned long led_timer;
unsigned long led_sampletime_ms = 2000;  //2s
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
unsigned long gps_sampletime_ms = 10000;  //5s
gpsSentenceInfoStruct info;
double latitude;
double longitude;
int satellites_num;
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
  //double latitude;
  //double longitude;
  int tmp, hour, minute, second;
  if(GPGGAstr[0] == '$')
  {
    tmp = getComma(1, GPGGAstr);
    hour     = (GPGGAstr[tmp + 0] - '0') * 10 + (GPGGAstr[tmp + 1] - '0');
    if(hour < 16) hour = hour+8;
    else  hour = hour + 8 - 24;
    minute   = (GPGGAstr[tmp + 2] - '0') * 10 + (GPGGAstr[tmp + 3] - '0');
    second    = (GPGGAstr[tmp + 4] - '0') * 10 + (GPGGAstr[tmp + 5] - '0');
    
    sprintf(buff, "UTC+8 timer %2d-%2d-%2d", hour, minute, second);
    Serial.println(buff);
    
    tmp = getComma(2, GPGGAstr);
    latitude = getDoubleNumber(&GPGGAstr[tmp]) / 100;
    tmp = getComma(4, GPGGAstr);
    longitude = getDoubleNumber(&GPGGAstr[tmp]) / 100;

    tmp = getComma(2, GPGGAstr);
    latitude = getDoubleNumber(&GPGGAstr[tmp])/100.0;
    int latitude_int=floor(latitude);
    double latitude_decimal=(latitude-latitude_int)*100.0/60.0;
    latitude=latitude_int+latitude_decimal;
    tmp = getComma(4, GPGGAstr);
    longitude = getDoubleNumber(&GPGGAstr[tmp])/100.0;
    int longitude_int=floor(longitude);
    double longitude_decimal=(longitude-longitude_int)*100.0/60.0;
    longitude=longitude_int+longitude_decimal;
    
    sprintf(buff, "latitude = %.4f, longitude = %.4f", latitude, longitude);
    Serial.println(buff); 
    
    tmp = getComma(7, GPGGAstr);
    satellites_num = getIntNumber(&GPGGAstr[tmp]);    
    sprintf(buff, "satellites number = %d", satellites_num);
    Serial.println(buff); 
  }
  else
  {
    //Serial.println("Not get data"); 
  }
}

void getconnectInfo(){
  //calling RESTful API to get TCP socket connection
  c2.print("GET /mcs/v2/devices/");
  c2.print(DEVICEID);
  c2.println("/connections.csv HTTP/1.1");
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  c2.println("Connection: close");
  c2.println();
  
  delay(500);

  int errorcount = 0;
  while (!c2.available())
  {
    Serial.println("waiting HTTP response: ");
    Serial.println(errorcount);
    errorcount += 1;
    if (errorcount > 10) {
      c2.stop();
      return;
    }
    delay(100);
  }
  int err = http.skipResponseHeaders();

  int bodyLen = http.contentLength();
  Serial.print("Content length is: ");
  Serial.println(bodyLen);
  Serial.println();
  char c;
  int ipcount = 0;
  int count = 0;
  int separater = 0;
  while (c2)
  {
    int v = c2.read();
    if (v != -1)
    {
      c = v;
      //Serial.print(c);
      connection_info[ipcount]=c;
      if(c==',')
      separater=ipcount;
      ipcount++;    
    }
    else
    {
      //Serial.println("no more content, disconnect");
      c2.stop();

    }
  }
  //Serial.println();
  Serial.print("The connection info: ");
  Serial.println(connection_info);
  int i;
  for(i=0;i<separater;i++)
  {  ip[i]=connection_info[i];
  }
  int j=0;
  separater++;
  for(i=separater;i<21 && j<5;i++)
  {  port[j]=connection_info[i];
     j++;
  }
  Serial.println("The TCP Socket connection instructions:");
  Serial.print("IP: ");
  Serial.println(ip);
  Serial.print("Port: ");
  Serial.println(port);
  portnum = atoi (port);
  //Serial.println(portnum);

} //getconnectInfo

void uploadstatus(){
  //calling RESTful API to upload datapoint to MCS to report LED status
  Serial.println("calling connection");
  LWiFiClient c2;  

  sprintf(bufD, "%.2f", concentration);
  sprintf(bufLATITUDE, "%.6f", latitude);
  sprintf(bufLONGITUDE, "%.6f", longitude);

  upload_dust = "Dust,," + String(bufD);
  upload_gps = "GPS,," + String(bufLATITUDE) + "," + String(bufLONGITUDE) + ",0";
  upload_LL = "LATITUDE,," + String(bufLATITUDE) + "\n" + "LONGITUDE,," + String(bufLONGITUDE);
  
  while (!c2.connect(SITE_URL, 80))
  {
    Serial.println("Re-Connecting to WebSite");
    //delay(1000);
  }
  //delay(100);
  if(digitalRead(13)==1) upload_led = "LED_Display,,1";
  else upload_led = "LED_Display,,0";
  if(satellites_num == 0) upload_data = upload_led + "\n" + upload_dust;
  else upload_data = upload_led + "\n" + upload_dust + "\n" + upload_gps + "\n" + upload_LL;
  int thislength = upload_data.length();
  HttpClient http(c2);
  c2.print("POST /mcs/v2/devices/");
  c2.print(DEVICEID);
  c2.println("/datapoints.csv HTTP/1.1");
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  c2.print("Content-Length: ");
  c2.println(thislength);
  c2.println("Content-Type: text/csv");
  c2.println("Connection: close");
  c2.println();
  //c2.println(upload_led);
  c2.println(upload_data);

  //delay(500);

  int errorcount = 0;
  while (!c2.available())
  {
    //Serial.print("waiting HTTP response: ");
    //Serial.println(errorcount);
    errorcount += 1;
    if (errorcount > 10) {
      c2.stop();
      return;
    }
    delay(100);
  }
  int err = http.skipResponseHeaders();

  int bodyLen = http.contentLength();
  Serial.print("Content length is: ");
  Serial.println(bodyLen);
  Serial.println();
  while (c2)
  {
    int v = c2.read();
    if (v != -1)
    {
      Serial.print(char(v));
    }
    else
    {
      //Serial.println("no more content, disconnect");
      c2.stop();

    }
    
  }
  Serial.println();
}



void connectTCP(){
  //establish TCP connection with TCP Server with designate IP and Port
  c.stop();
  //Serial.println("Connecting to TCP");
  //Serial.println(ip);
  //Serial.println(portnum);
  while (0 == c.connect(ip, portnum))
  {
    //Serial.println("Re-Connecting to TCP");    
    delay(1000);
  }  
  //Serial.println("send TCP connect");
  c.println(tcpdata);
  c.println();
  //Serial.println("waiting TCP response:");
} //connectTCP

void heartBeat(){
  Serial.println("send TCP heartBeat");
  c.println(tcpdata);
  c.println();
    
} //heartBeat

void setup(){
  LTask.begin();
  LWiFi.begin();
  bar.begin();
  bar.setLevel(1);
  //Serial.begin(115200);
  //while(!Serial) delay(1000); /* comment out this line when Serial is not present, ie. run this demo without connect to PC */
  bar.setLevel(2);
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    delay(1000);
  }
  bar.setLevel(3);
  Serial.println("calling connection");
  bar.setLevel(4);
  while (!c2.connect(SITE_URL, 80))
  {
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }
  delay(100);
  bar.setLevel(5);
  pinMode(13, OUTPUT);
  getconnectInfo();
  bar.setLevel(6);
  connectTCP();
  bar.setLevel(7);
  
  led_timer = millis();
  pinMode(Dust_PIN,INPUT);
  dust_timer = millis();
  gps_timer = millis();
  bar.setLevel(8);
  LGPS.powerOn();
  bar.setLevel(9);
  HB_timer = millis();
  UL_timer = millis(); 
  bar.setLevel(10);
}

void loop(){
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
    Serial.print("concentration = ");
    Serial.print(concentration);
    Serial.println(" pcs/0.01cf");
    //Serial.println("\n");
    lowpulseoccupancy = 0;
    dust_timer = millis();
  }
  if ((millis()-gps_timer) >= gps_sampletime_ms)
  {
    LGPS.getData(&info);
    //Serial.println((char*)info.GPGGA); 
    parseGPGGA((const char*)info.GPGGA);
    gps_timer = millis();
  }
  //Check for TCP socket command from MCS Server 
  String tcpcmd="";
  while (c.available())
   {
      int v = c.read();
      if (v != -1)
      {
        Serial.print((char)v);
        tcpcmd += (char)v;
        if (tcpcmd.substring(40).equals(tcpcmd_led_on)){
          digitalWrite(13, HIGH);
          Serial.print("Switch LED ON ");
          tcpcmd="";
        }else if(tcpcmd.substring(40).equals(tcpcmd_led_off)){  
          digitalWrite(13, LOW);
          Serial.print("Switch LED OFF");
          tcpcmd="";
        }
      }
   }
  if ((millis()-HB_timer) >= HB_sampletime_ms) {
    heartBeat();
    HB_timer = millis();
  }
  //Check for report datapoint status interval
  if ((millis()-UL_timer) >= UL_sampletime_ms) {
    uploadstatus();
    UL_timer = millis();
  }
}
