#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
//
#include <Arduino.h>
#include <U8g2lib.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//============WiFi名称和密码================//
const char* ssid     = "CU_A2eT";
const char* password = "1028507471";
//==========目标服务器网址和端口==============//
const char* host = "116.62.81.138";  //api.seniverse.com
const uint16_t port = 80 ;
//===============地区设置===================//
String City = "ip";//城市
String My_Key = "S2zymm_ymdywI-zHY";//禁止泄露
//===============OLED引脚===================//
//
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/     14, /* data=*/12 , /* cs=*/ 3, /* dc=*/ 15, /* reset=*/ 13);

//定义天气图标代码
#define SUN  0
#define SUN_CLOUD  1
#define CLOUD 2
#define RAIN 3


//=================变量=====================//
typedef struct
{
  int Hour;
  int Minute;
  int Second;
  int Year;
  int Month;
  int Day;
} STime;      //时间日期结构体
typedef struct
{
  int gws;
  int gwg;
  int dws;
  int dwg;
  int sds;
  int sdg;
} dat;      //最高温最低温和湿度的结构体  十位和个位
typedef struct
{
  int zuigaowendu;
  int zuidiwendu;
  int shidu;
  int tianqitubiao;
} tianqixinxi; //高温最低温和湿度的结构体

tianqixinxi day1, day2, day3;
dat Nume;
STime dTime, hTime;
int OnTime = -1;   //计数显示变量  10s时间 5s今明后天天气
bool DatFlag = true; //处理接收json数据的标志位
unsigned long getTime = 0;  //获取网络天气和时间  5s请求一次
String inputString = "";  //接收到的数据
//请求URL
String url = "/v3/weather/daily.json?key=" + My_Key + "&location=" + City + "&language=zh-Hans&unit=c&start=0&days=3";
//请求数据
String urlDat = "key=" + My_Key + "&location=" + City + "&language=zh-Hans&unit=c&start=0&days=3";
WiFiClient client;
/*************************************************************************************
   使用取模软件：PCTOLCD 2002完美版
   取模方式为：阴码，逐行式，逆向
   字体：32X32 仿宋
   图标：60X60
**************************************************************************************/
static const unsigned char PROGMEM qintian[] [32]=
{
//晴(0) 阴(1) 天(2)

{0x00,0x04,0x00,0x04,0xDE,0x7F,0x12,0x04,0x92,0x3F,0x12,0x04,0xD2,0x7F,0x1E,0x00,//
0x92,0x3F,0x92,0x20,0x92,0x3F,0x92,0x20,0x9E,0x3F,0x92,0x20,0x80,0x28,0x80,0x10,},///*"晴",0*/

{0x00,0x00,0xBE,0x3F,0xA2,0x20,0x92,0x20,0x92,0x20,0x8A,0x3F,0x92,0x20,0x92,0x20,//
0xA2,0x20,0xA2,0x3F,0xA2,0x20,0x96,0x20,0x4A,0x20,0x42,0x20,0x22,0x28,0x12,0x10,},///*"阴",1*/

{0x00,0x00,0xFC,0x1F,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0xFF,0x7F,0x80,0x00,//
0x40,0x01,0x40,0x01,0x20,0x02,0x20,0x02,0x10,0x04,0x08,0x08,0x04,0x10,0x03,0x60,},///*"天",2*/

};

static const unsigned char PROGMEM duoyun[] [32]=
{
//多(0) 云(1)

{0x40,0x00,0x40,0x00,0xE0,0x0F,0x10,0x04,0x1C,0x02,0x20,0x01,0xC0,0x02,0x30,0x01,//
0x8E,0x1F,0x40,0x10,0x30,0x08,0x4C,0x04,0x80,0x02,0x80,0x01,0x70,0x00,0x0E,0x00,},///*"多",0*/

{0x00,0x00,0xFC,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x7F,0x40,0x00,//
0x20,0x00,0x20,0x00,0x10,0x02,0x08,0x04,0x04,0x08,0xFE,0x1F,0x04,0x10,0x00,0x10,},///*"云",1*/

};

static const unsigned char PROGMEM yutian[] [32]=
{
//雨(0) 大(1) 中(2) 小(3)

{0x00,0x00,0xFF,0x7F,0x80,0x00,0x80,0x00,0x80,0x00,0xFE,0x3F,0x82,0x20,0x82,0x20,//
0x92,0x22,0xA2,0x24,0x82,0x20,0x92,0x22,0xA2,0x24,0x82,0x20,0x82,0x28,0x02,0x10,},///*"雨",0*/

{0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0xFF,0x7F,0x80,0x00,0x80,0x00,//
0x40,0x01,0x40,0x01,0x20,0x02,0x20,0x02,0x10,0x04,0x08,0x08,0x04,0x10,0x03,0x60,},///*"大",1*/

{0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0xFC,0x1F,0x84,0x10,0x84,0x10,0x84,0x10,//
0x84,0x10,0x84,0x10,0xFC,0x1F,0x84,0x10,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,},///*"中",2*/

{0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x88,0x08,0x88,0x10,0x88,0x20,//
0x84,0x20,0x84,0x40,0x82,0x40,0x81,0x40,0x80,0x00,0x80,0x00,0xA0,0x00,0x40,0x00,},///*"小",3*/

};

static const unsigned char PROGMEM fuhao[] [32] =
{
// :(0) %(1) ℃(2) -(3)

{0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x30,0x00,0x00,0x00,0x00,0x00,0x00,},//*":",0*/

{0xF0,0x00,0x08,0x31,0xF0,0x0C,0x80,0x03,0x60,0x1E,0x18,0x21,0x00,0x1E,0x00,0x00,},//*"%",1*/

{0x06,0x00,0x89,0x2F,0x69,0x30,0x36,0x20,0x10,0x20,0x18,0x00,0x18,0x00,0x18,0x00,//
0x18,0x00,0x18,0x00,0x18,0x00,0x10,0x00,0x30,0x20,0x60,0x10,0x80,0x0F,0x00,0x00,},//*"℃",2*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//*"-",3*/

};
static const unsigned char PROGMEM shidu[] [32] =
{
 //湿(0) 度(1)

{0x00,0x00,0xE4,0x1F,0x28,0x10,0x28,0x10,0xE1,0x1F,0x22,0x10,0x22,0x10,0xE8,0x1F,//
0x88,0x04,0x84,0x04,0x97,0x24,0xA4,0x14,0xC4,0x0C,0x84,0x04,0xF4,0x7F,0x00,0x00,},///*"湿",0*/

{0x80,0x00,0x00,0x01,0xFC,0x7F,0x44,0x04,0x44,0x04,0xFC,0x3F,0x44,0x04,0x44,0x04,//
0xC4,0x07,0x04,0x00,0xF4,0x0F,0x24,0x08,0x42,0x04,0x82,0x03,0x61,0x0C,0x1C,0x70,},///*"度",1*/

};
static const unsigned char PROGMEM date[] [32] =
{
// 月(0) 日(1)

{0x00,0x00,0xF8,0x1F,0x08,0x10,0x08,0x10,0x08,0x10,0xF8,0x1F,0x08,0x10,0x08,0x10,//
0x08,0x10,0xF8,0x1F,0x08,0x10,0x08,0x10,0x04,0x10,0x04,0x10,0x02,0x14,0x01,0x08,},/*"月" ,0*/

{0x00,0x00,0xF8,0x0F,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0xF8,0x0F,//
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0xF8,0x0F,0x08,0x08,},/*"日" ,1*/

};
//====================================================数组END=======================================================================//

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.clearBuffer();          // clear the internal memory
//  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.setFont(u8g2_font_unifont_t_symbols); 
  u8g2.drawStr(0, 18, "connecting to ");
  u8g2.setCursor(0, 36);   //设置光标处
  u8g2.print(ssid);
  u8g2.sendBuffer();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) //等待连接
  {
//    u8g2.setCursor(0, 50);   //设置光标处
    int x = 0;
    while(x<=40){
    u8g2.drawStr( x , 50 , ".");
    u8g2.sendBuffer();
    x+=15;
    delay(500);
    }
    delay(100);
  }
  Serial.println("WiFi connected");
  u8g2.clearBuffer();
  u8g2.drawStr(0, 18, "WiFi connected");
  u8g2.setCursor(0, 36);   //设置光标处
  u8g2.print("IP:");
  u8g2.print(WiFi.localIP());
  u8g2.setCursor(0, 54);
  u8g2.print("Please wait...");
  u8g2.sendBuffer();
  Serial.println("Getting Data");
  delay(3000);
}
void loop()
{
    standDisplay();
    GET_Weather();
    DateHandle();      
    while(OnTime<4){
        OnTime++;  
        DisplayTianqi();
    }
    OnTime=-1;
}

// 绘制固定元素
void standDisplay(){
    u8g2.clearBuffer();
    u8g2.drawXBMP( 84,  32, 16, 16, fuhao[3]); //“-”
    u8g2.drawXBMP( 112, 32, 16, 16, fuhao[2]); //“℃”
    u8g2.drawXBMP( 64,  48, 16, 16, shidu[0]); //湿度
    u8g2.drawXBMP( 80,  48, 16, 16, shidu[1]); 
    u8g2.drawXBMP( 80,  0, 16, 16, date[0]);  //月
    u8g2.drawXBMP( 112,  0, 16, 16, date[1]); //日
    u8g2.setCursor(112, 64);
    u8g2.print("%");
    u8g2.sendBuffer();
}

//获取天气信息
void GET_Weather()
{
  if ((millis() - getTime > 5000)) //10s
  {
    getTime = millis();
    //Serial.print("connecting to ");
    if (!client.connect(host, port))
    {
      Serial.println("服务器连接失败");
      return;
    }
    //Serial.print("Requesting URL: ");
    // 发送请求报文
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +  //请求行  请求方法 ＋ 请求地址 + 协议版本
                 "Host: " + host + "\r\n" +                //请求头部
                 "Connection: close\r\n" +                //处理完成后断开连接
                 "\r\n" +                                 //空行
                 urlDat);                                 //请求数据
    delay(100);
    while (client.available()) //接收数据
    {
      String line = client.readStringUntil('\r');
      inputString += line;
      delay(100);
    }
    Serial.println("--------------result(GET_Weather)------------");
    Serial.println(inputString); 
    Serial.println("--------------result(GET_Weather)------------");
    client.stop();      //断开与服务器连接以节约资源
    DatFlag = true;
    // Serial.println(inputString);
  }
}

//处理获取到的初始信息
void DateHandle()
{
  if (DatFlag)
  {
    DatFlag = false;
    int jsonBeginAt = inputString.indexOf("{");   //判断json数据完整性
    int jsonEndAt = inputString.lastIndexOf("}");

    if (jsonBeginAt != -1 && jsonEndAt != -1)
    {
      //净化json数据
      inputString = inputString.substring(jsonBeginAt, jsonEndAt + 1);//取得一个完整的JSON字符串
      Serial.println("--------------result(DateHandle)------------");
      Serial.println(inputString); 
      Serial.println("--------------result(DateHandle)------------");
     processMessage();//对数据进行处理
     Serial.println("2error here!!!!!!!!!!!!!!!!!!!!!!!!!!");
      
      inputString = "";
      Serial.println("--------------initialize_inputString------------");
      Serial.println(inputString);
      Serial.println("--------------initialize_inputString------------");
    }
  }
}

//处理数据，将数据转换后赋值给相应变量
void processMessage()
{
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 3*JSON_OBJECT_SIZE(14) + 800;
DynamicJsonDocument doc(capacity);

const char* json = "{\"results\":[{\"location\":{\"id\":\"WW0P9B6381U6\",\"name\":\"济源\",\"country\":\"CN\",\"path\":\"济源,济源,河南,中国\",\"timezone\":\"Asia/Shanghai\",\"timezone_offset\":\"+08:00\"},\"daily\":[{\"date\":\"2020-04-04\",\"text_day\":\"多云\",\"code_day\":\"4\",\"text_night\":\"多云\",\"code_night\":\"4\",\"high\":\"21\",\"low\":\"5\",\"rainfall\":\"0.0\",\"precip\":\"\",\"wind_direction\":\"西南\",\"wind_direction_degree\":\"225\",\"wind_speed\":\"16.20\",\"wind_scale\":\"3\",\"humidity\":\"35\"},{\"date\":\"2020-04-05\",\"text_day\":\"多云\",\"code_day\":\"4\",\"text_night\":\"多云\",\"code_night\":\"4\",\"high\":\"17\",\"low\":\"6\",\"rainfall\":\"0.0\",\"precip\":\"\",\"wind_direction\":\"西南\",\"wind_direction_degree\":\"225\",\"wind_speed\":\"25.20\",\"wind_scale\":\"4\",\"humidity\":\"63\"},{\"date\":\"2020-04-06\",\"text_day\":\"晴\",\"code_day\":\"0\",\"text_night\":\"多云\",\"code_night\":\"4\",\"high\":\"21\",\"low\":\"8\",\"rainfall\":\"0.0\",\"precip\":\"\",\"wind_direction\":\"西南\",\"wind_direction_degree\":\"225\",\"wind_speed\":\"25.20\",\"wind_scale\":\"4\",\"humidity\":\"56\"}],\"last_update\":\"2020-04-04T11:17:52+08:00\"}]}";

deserializeJson(doc, json);

JsonObject results_0 = doc["results"][0];

JsonObject results_0_location = results_0["location"];
const char* results_0_location_id = results_0_location["id"]; // "WW0P9B6381U6"
const char* results_0_location_name = results_0_location["name"]; // "济源"
const char* results_0_location_country = results_0_location["country"]; // "CN"
const char* results_0_location_path = results_0_location["path"]; // "济源,济源,河南,中国"
const char* results_0_location_timezone = results_0_location["timezone"]; // "Asia/Shanghai"
const char* results_0_location_timezone_offset = results_0_location["timezone_offset"]; // "+08:00"

JsonArray results_0_daily = results_0["daily"];

JsonObject results_0_daily_0 = results_0_daily[0];
const char* results_0_daily_0_date = results_0_daily_0["date"]; // "2020-04-04"
const char* results_0_daily_0_text_day = results_0_daily_0["text_day"]; // "多云"
const char* results_0_daily_0_code_day = results_0_daily_0["code_day"]; // "4"
const char* results_0_daily_0_text_night = results_0_daily_0["text_night"]; // "多云"
const char* results_0_daily_0_code_night = results_0_daily_0["code_night"]; // "4"
const char* results_0_daily_0_high = results_0_daily_0["high"]; // "21"
const char* results_0_daily_0_low = results_0_daily_0["low"]; // "5"
const char* results_0_daily_0_rainfall = results_0_daily_0["rainfall"]; // "0.0"
const char* results_0_daily_0_precip = results_0_daily_0["precip"]; // ""
const char* results_0_daily_0_wind_direction = results_0_daily_0["wind_direction"]; // "西南"
const char* results_0_daily_0_wind_direction_degree = results_0_daily_0["wind_direction_degree"]; // "225"
const char* results_0_daily_0_wind_speed = results_0_daily_0["wind_speed"]; // "16.20"
const char* results_0_daily_0_wind_scale = results_0_daily_0["wind_scale"]; // "3"
const char* results_0_daily_0_humidity = results_0_daily_0["humidity"]; // "35"

JsonObject results_0_daily_1 = results_0_daily[1];
const char* results_0_daily_1_date = results_0_daily_1["date"]; // "2020-04-05"
const char* results_0_daily_1_text_day = results_0_daily_1["text_day"]; // "多云"
const char* results_0_daily_1_code_day = results_0_daily_1["code_day"]; // "4"
const char* results_0_daily_1_text_night = results_0_daily_1["text_night"]; // "多云"
const char* results_0_daily_1_code_night = results_0_daily_1["code_night"]; // "4"
const char* results_0_daily_1_high = results_0_daily_1["high"]; // "17"
const char* results_0_daily_1_low = results_0_daily_1["low"]; // "6"
const char* results_0_daily_1_rainfall = results_0_daily_1["rainfall"]; // "0.0"
const char* results_0_daily_1_precip = results_0_daily_1["precip"]; // ""
const char* results_0_daily_1_wind_direction = results_0_daily_1["wind_direction"]; // "西南"
const char* results_0_daily_1_wind_direction_degree = results_0_daily_1["wind_direction_degree"]; // "225"
const char* results_0_daily_1_wind_speed = results_0_daily_1["wind_speed"]; // "25.20"
const char* results_0_daily_1_wind_scale = results_0_daily_1["wind_scale"]; // "4"
const char* results_0_daily_1_humidity = results_0_daily_1["humidity"]; // "63"

JsonObject results_0_daily_2 = results_0_daily[2];
const char* results_0_daily_2_date = results_0_daily_2["date"]; // "2020-04-06"
const char* results_0_daily_2_text_day = results_0_daily_2["text_day"]; // "晴"
const char* results_0_daily_2_code_day = results_0_daily_2["code_day"]; // "0"
const char* results_0_daily_2_text_night = results_0_daily_2["text_night"]; // "多云"
const char* results_0_daily_2_code_night = results_0_daily_2["code_night"]; // "4"
const char* results_0_daily_2_high = results_0_daily_2["high"]; // "21"
const char* results_0_daily_2_low = results_0_daily_2["low"]; // "8"
const char* results_0_daily_2_rainfall = results_0_daily_2["rainfall"]; // "0.0"
const char* results_0_daily_2_precip = results_0_daily_2["precip"]; // ""
const char* results_0_daily_2_wind_direction = results_0_daily_2["wind_direction"]; // "西南"
const char* results_0_daily_2_wind_direction_degree = results_0_daily_2["wind_direction_degree"]; // "225"
const char* results_0_daily_2_wind_speed = results_0_daily_2["wind_speed"]; // "25.20"
const char* results_0_daily_2_wind_scale = results_0_daily_2["wind_scale"]; // "4"
const char* results_0_daily_2_humidity = results_0_daily_2["humidity"]; // "56"

const char* results_0_last_update = results_0["last_update"]; // "2020-04-04T11:17:52+08:00"

  String riqi = results_0_last_update;  //将日期取出处理
  hTime.Year = (riqi.substring(0, 4)).toInt();
  hTime.Month = (riqi.substring(5, 7)).toInt();
  hTime.Day = (riqi.substring(8, 10)).toInt();
Serial.println("---------------------");
  Serial.println(hTime.Year);
  Serial.println(hTime.Month);
  Serial.println(hTime.Day);
Serial.println("---------------------");

  day1.tianqitubiao = atoi(results_0_daily_0_code_day);//获取今天天气信息
    if (day1.tianqitubiao>=0 && day1.tianqitubiao <= 3)
    {
        day1.tianqitubiao=SUN;//晴天
    }else if (day1.tianqitubiao>=4 && day1.tianqitubiao <= 8){
        day1.tianqitubiao=CLOUD;//多云
    }else if(day1.tianqitubiao= 9){
        day1.tianqitubiao=SUN_CLOUD;//阴天
    }else if(day1.tianqitubiao>=10 && day1.tianqitubiao <= 19){
        day1.tianqitubiao=RAIN;//雨天
    }else if(day1.tianqitubiao>=20 && day1.tianqitubiao <= 25){
        day1.tianqitubiao=4;//雪天
    }else if(day1.tianqitubiao>=26 && day1.tianqitubiao <= 29){
        day1.tianqitubiao=5;//扬尘
    }else if(day1.tianqitubiao>=30 && day1.tianqitubiao <= 31){
        day1.tianqitubiao=6;//雾霾
    }else if(day1.tianqitubiao>=32 && day1.tianqitubiao <= 36){
        day1.tianqitubiao=7;//大风
    }else (day1.tianqitubiao>36){
        day1.tianqitubiao=8;//未知天气
    }
    
  day1.zuigaowendu = atoi(results_0_daily_0_high);
  day1.zuidiwendu = atoi(results_0_daily_0_low);
  day1.shidu = atoi(results_0_daily_0_humidity);
Serial.println("---------------------");
  Serial.println(day1.tianqitubiao);
  Serial.println(day1.zuigaowendu);
  Serial.println(day1.zuidiwendu);
  Serial.println(day1.shidu);
Serial.println("---------------------");
  day2.tianqitubiao = atoi(results_0_daily_1_code_day);
  if (day2.tianqitubiao>=0 && day2.tianqitubiao <= 3)
    {
        day2.tianqitubiao=SUN;//晴天
    }else if (day2.tianqitubiao>=4 && day2.tianqitubiao <= 8){
        day2.tianqitubiao=CLOUD;//多云
    }else if(day2.tianqitubiao= 9){
        day2.tianqitubiao=SUN_CLOUD;//阴天
    }else if(day2.tianqitubiao>=10 && day2.tianqitubiao <= 19){
        day2.tianqitubiao=RAIN;//雨天
    }else if(day2.tianqitubiao>=20 && day2.tianqitubiao <= 25){
        day2.tianqitubiao=4;//雪天
    }else if(day2.tianqitubiao>=26 && day2.tianqitubiao <= 29){
        day2.tianqitubiao=5;//扬尘
    }else if(day2.tianqitubiao>=30 && day2.tianqitubiao <= 31){
        day2.tianqitubiao=6;//雾霾
    }else if(day2.tianqitubiao>=32 && day2.tianqitubiao <= 36){
        day2.tianqitubiao=7;//大风
    }else (day2.tianqitubiao>36){
        day2.tianqitubiao=8;//未知天气
    }
  day2.zuigaowendu = atoi(results_0_daily_1_high);
  day2.zuidiwendu = atoi(results_0_daily_1_low);
  day2.shidu = atoi(results_0_daily_1_humidity);

  day3.tianqitubiao = atoi(results_0_daily_2_code_day);
  if (day3.tianqitubiao>=0 && day3.tianqitubiao <= 3)
    {
        day3.tianqitubiao=SUN;//晴天
    }else if (day3.tianqitubiao>=4 && day3.tianqitubiao <= 8){
        day3.tianqitubiao=CLOUD;//多云
    }else if(day3.tianqitubiao= 9){
        day3.tianqitubiao=SUN_CLOUD;//阴天
    }else if(day3.tianqitubiao>=10 && day3.tianqitubiao <= 19){
        day3.tianqitubiao=RAIN;//雨天
    }else if(day3.tianqitubiao>=20 && day3.tianqitubiao <= 25){
        day3.tianqitubiao=4;//雪天
    }else if(day3.tianqitubiao>=26 && day3.tianqitubiao <= 29){
        day3.tianqitubiao=5;//扬尘
    }else if(day3.tianqitubiao>=30 && day3.tianqitubiao <= 31){
        day3.tianqitubiao=6;//雾霾
    }else if(day3.tianqitubiao>=32 && day3.tianqitubiao <= 36){
        day3.tianqitubiao=7;//大风
    }else (day3.tianqitubiao>36){
        day3.tianqitubiao=8;//未知天气
    }
  day3.zuigaowendu = atoi(results_0_daily_2_high);
  day3.zuidiwendu = atoi(results_0_daily_2_low);
  day3.shidu = atoi(results_0_daily_2_humidity);
}

/**************************************************
   函数名称：DisplayZZXS
   函数功能：传入第几天将天气数据存入
   参数说明：dday
**************************************************/
void DisplayZZXS(int dday)
{
  switch (dday)
  {
    case 0: display_tq(dday, day1.tianqitubiao, day1.zuidiwendu, day1.zuigaowendu, day1.shidu);
      break;
    case 1: display_tq(dday, day2.tianqitubiao, day2.zuidiwendu, day2.zuigaowendu, day2.shidu);
      break;
    case 2: display_tq(dday, day3.tianqitubiao, day3.zuidiwendu, day3.zuigaowendu, day3.shidu);
      break;
  }
}
/**************************************************
   函数名称：DisplayTianqi
   函数功能：显示天气
   参数说明：无
**************************************************/
void DisplayTianqi()
{
    DisplayZZXS(OnTime);
    Serial.println("打印天气中");
}

// void tianqi_dat(int dw, int gw, int sd)
// {
//   Nume.dws = dw / 10;
//   Nume.dwg = dw % 10;
//   Nume.gws = gw / 10;
//   Nume.gwg = gw % 10;
//   Nume.sds = sd / 10;
//   Nume.sdg = sd % 10;
// }
/*****************************************************
   函数名称：显示天气函数
   参数说明：
   dday：第几天
   tq：  天气
   dw：  最低气温
   gw：  最高气温
   sd：  湿度
******************************************************/
void display_tq(int dday, int tq, int dw, int gw, int sd)
{
//   tianqi_dat(dw, gw, sd);
  //u8g2.clearBuffer();
  switch (dday)
  {
    // case 0: u8g2.drawXBMP(64, 0, 16, 16, jintian[0]); //今
    //   break;
    // case 1: u8g2.drawXBMP(64, 0, 16, 16, jintian[1]); //
    //   break;
    // case 2: u8g2.drawXBMP(64, 0, 16, 16, jintian[2]); //
    //   break;
  }
//   u8g2.drawXBMP(80, 0, 16, 16, jintian[3]); //天
  switch (tq)
  {
    case SUN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(0, 50, 69);  
      break;
    case SUN_CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(0, 50, 65);  
      break;
    case CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(0, 50, 64);  
      break;
    case RAIN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(0, 50, 67);  
      break;
  }
    // u8g2.drawXBMP(64, 32, 8, 16, num[Nume.dws]); //最低温
    // u8g2.drawXBMP(72, 32, 8, 16, num[Nume.dwg]); //最低温
    // u8g2.drawXBMP(88, 32, 8, 16, num[Nume.gws]); //最高温
    // u8g2.drawXBMP(96, 32, 8, 16, num[Nume.gwg]); //最高温
    u8g2.setFont(u8g2_font_unifont_t_symbols); 
    u8g2.setCursor(64,45);
    u8g2.print(dw);//最低温度int
    u8g2.setCursor(96,45);
    u8g2.print(gw);//最高温度int
    u8g2.setCursor(96,61);
    u8g2.print(sd);//湿度int
    u8g2.sendBuffer();
    delay(3000);
}
