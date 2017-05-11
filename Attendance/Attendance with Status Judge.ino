#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include "MFRC522.h"
#include "SPI.h"

#if !defined(ARDUINO_ARCH_ESP8266)
#error "Uncomment to the ESP8266"
#endif

//*-- IoT Information
#define SSID "ourdorm"
#define PASS "11021053"
#define HOST "api.thingspeak.com" // ThingSpeak IP Address: 184.106.153.149
#define PORT 80
#define READAPIKEY "J8OGS6CAVVJ9P4TC"  // READ APIKEY for the CHANNEL_ID

MFRC522 mfrc522(D4, D8);  // Create MFRC522 instance
int status = WL_IDLE_STATUS;
WiFiClient  client;
String Field1, Field2;

unsigned long Channel_Num = 267190;
const char *Write_Key = "J8OGS6CAVVJ9P4TC";

void setup() {
  Serial.begin(9600);
  Serial.println("ESP8266 Ready!");
  delay(250);

  // Connecting to a WiFi network
  Serial.print("Connect to ");
  Serial.println( SSID );
  WiFi.begin( SSID, PASS );

  //Initial ThingSpeak Upload
  ThingSpeak.begin(client);

  // 持續等待並連接到指定的 WiFi SSID
  while ( WiFi.status() != WL_CONNECTED )
  {
    delay(500);
    Serial.print( "." );
  }
  Serial.println( "" );

  Serial.println( "WiFi connected" );
  Serial.print( "IP address: " );
  Serial.println( WiFi.localIP() );
  Serial.println( "" );

  Serial.println("RFID-MFRC522 Booting....");
  pinMode(D8, OUTPUT);
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522

  Serial.println("System Boot Successful\n==============================================");
}

void loop() {
  digitalWrite(D8, LOW);             //Set D8 as Beeper, Port status set LOW (DisEngage)

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID
    byte idSize = mfrc522.uid.size;   // 取得UID的長度

    digitalWrite(D8, HIGH);           //Beeper Engage Once
    delay(250);
    digitalWrite(D8, LOW);

    Serial.print("PICC type: ");      // 顯示卡片類型
    // 根據卡片回應的SAK值（mfrc522.uid.sak）判斷卡片類型
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    Serial.print("UID Size: ");       // 顯示卡片的UID長度值
    Serial.println(idSize);

    String Card_Num;
    for (byte i = 0; i < idSize; i++) {  // 逐一顯示UID碼
      //Serial.println(id[i], HEX);       // 以16進位顯示UID值
      Card_Num += id[i];                  //合併卡號 -> String
    }
    Serial.print("Card Num: ");             //Output Card_Num String
    Serial.println(Card_Num);
    Serial.println("");
    
    Field1 = receiveField(Channel_Num, 1);  // channel / filed_id
    Field2 = receiveField(Channel_Num, 2);  // channel / filed_id

    //ThingSpeak Data Upload Setting
    ThingSpeak.setField(1 , Card_Num);
    ThingSpeak.setField(2 , statusJudge(Card_Num));

    //Show ThingSpeak Upload Status
    Serial.println("ThingSpeak Data Upload Start");
    if (ThingSpeak.writeFields(Channel_Num, Write_Key))
      Serial.println("ThingSpeak Upload Success");
    else
      Serial.println("ThingSpeak Upload Fail");

    delay(500);

    Serial.println();
    mfrc522.PICC_HaltA();  // 讓卡片進入停止模式
  }
}

String receiveField( uint32_t channel_id, uint8_t field_id)
{
    // 設定 ESP8266 作為 Client 端
    WiFiClient client;
    
    if( !client.connect( HOST, PORT ) )
    {
        Serial.println( "Connection Failed" );
        return "";
    }
    else
    {
        ////// 使用 GET 取回最近 1000 筆的 FIELD_ID 資料 //////
        //https://api.thingspeak.com/channels/CHANNEL_ID/fields/FIELD_ID/last.json
        //String GET = "GET /channels/" + String(channel_id) + "/fields/" + String(field_id) + "/last.json?key=" + READAPIKEY + "&results=" + String(record);
        //https://api.thingspeak.com/channels/267190/fields/1.json?key=J8OGS6CAVVJ9P4TC&results=1000
        //https://api.thingspeak.com/channels/267190/fields/2.json?key=J8OGS6CAVVJ9P4TC&results=1000
        //----

        Serial.print("Get Fields ");
        Serial.print(field_id);
        Serial.println(" Data...");
        String GET = "GET /channels/" + String(channel_id) + "/fields/" + String(field_id) + ".json?key=" + String(Write_Key) + "&results=1000";
        String getStr = GET + " HTTP/1.1\r\n";
        client.print( getStr );
        client.print( "Host: api.thingspeak.com\n" );
        client.print( "Connection: close\r\n\r\n" );
        
        delay(1000);
        
        // 讀取所有從 ThingSpeak IoT Server 的回應並輸出到串列埠
        while(client.available())
        {
            String line = client.readStringUntil('\r');
            if ((line.indexOf("channel")) != -1){
              client.stop();
              return line;
            }
        }
        client.stop();
        return "";
    }
}

String statusJudge(String CardNum){
  int start1 = Field1.lastIndexOf(CardNum) - 11;
  Field1.remove(start1);

  int start2 = Field1.lastIndexOf(":");
  String temp = Field1.substring(start2);    //最後一筆所在位置

  //Serial.println(Field2);
  Serial.println(temp);
  temp += ",";
  
  String temp2 = Field2.substring(Field2.indexOf(temp));
  temp2.remove(25);
  Serial.println(temp2);

  if (temp2.indexOf("上班") != -1) {
    return "下班";
  } else if (temp2.indexOf("下班") != -1) {
    return "上班";
  }
}
