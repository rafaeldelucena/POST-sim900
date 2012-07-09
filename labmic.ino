#include <SoftwareSerial.h>
#include <string.h>
 
SoftwareSerial sim900(7, 8);
int statusConnection;
void configAPN();
void splitAndSend(String text);
boolean verifyMessage(String text);
void addSensor(String mote_id, String name, String value);
void addMote(String rxPackets, String txPackets, String rxBytes, String txBytes);
void InitHttpRequest();
void FinishHttpRequest();
boolean sendCommandAndWaitResp(String comando, String respostaEsperada, int tentativas, int delayEntreTentativas);
void ShowSerialData();

int mote_id=1;

void setup()
{
  sim900.begin(19200);               // the GPRS baud rate   
  Serial.begin(19200);    // the GPRS baud rate 
  delay(500);
  configAPN();
}

void configAPN()
{
  sim900.println("AT+CIPSHUT");
  delay(200);

  sim900.println("AT+CGATT?");
  delay(100);
 
  ShowSerialData();
 
  sim900.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  delay(1000);
 
  ShowSerialData();
 
  sendCommandAndWaitResp("AT+CSTT=\"tim.br\", \"tim\", \"tim\"", "OK", 3, 100);
  delay(4000);
 
  ShowSerialData();
 
  sim900.println("AT+SAPBR=1,1");//setting the SAPBR, for detail you can refer to the AT command mamual
  delay(2000);
 
  ShowSerialData();
  
}

void loop()
{
    String input;
    if (Serial.available()) {
        while(Serial.available() != 0) {
            input.concat((char)Serial.read());
        }

    }

    if (input.length() >= 0)
    {
        splitAndSend(input);
    }
}

void splitAndSend(String text)
{
 
 String str;
 String strOne;
 String strTwo;
 String strOneT;
 String strTwoT;
 int i = text.indexOf(';');
 strOne = text.substring(0,i);
 Serial.println(strOne);
 if(!verifyMessage(strOne))
     return;
 int indexOfDouble = strOne.indexOf(':');
 strOneT = strOne.substring(indexOfDouble+2, strOne.length());
 
 strTwo = text.substring(i+1,text.length());
  Serial.println(strTwo);
 if(!verifyMessage(strTwo)) {
   return;
 }
 indexOfDouble = strTwo.indexOf(':');
 strTwoT = strTwo.substring(0,strTwo.length()-3);
  
 addSensor((String)mote_id, strOneT, strTwoT);
}

boolean verifyMessage(String text)
{
  String message(text);
  int indexOfFirstDoubleDot = message.indexOf(':');
  if(message.charAt(indexOfFirstDoubleDot) == ':' && message.charAt(indexOfFirstDoubleDot+1) == ':')
      return true;
      
  return false;
  
}

void addSensor(String mote_id, String name, String value)
{
  InitHttpRequest();
  
  String dataPost = "_method=POST&data[Sensor][name]=" + name + "&data[Sensor][value]=" + value + "&data[Sensor][mote_id]=" + mote_id;
 
  String httpData = "AT+HTTPDATA=";
  String delayToSend = ",3000";
  
  sim900.println(httpData + dataPost.length() + delayToSend);// It is ready to receive data from uart , and DCD has been set to low, the arguments are size in bytes and timeout to send a message
  
  sim900.print(dataPost);
  FinishHttpRequest();
}

void addMote(int id, String rxPackets, String txPackets, String rxBytes, String txBytes)
{
  InitHttpRequest();
  String httpData = "AT+HTTPDATA=";
  String delayToSend = ",3000";
  
  String postData = "_method=POST&data[Mote][rx_packets]="+ rxPackets + "&data[Mote][tx_packets]=" + txPackets + "&data[Mote][rx_bytes]="+ rxBytes + "&data[Mote][tx_bytes]=" + txBytes;
  
  sim900.println(httpData + postData.length() + delayToSend);// It is ready to receive data from uart , and DCD has been set to low, the arguments are size in bytes and timeout to send a message
  
  sim900.print(postData);
  delay(3000);
  
  FinishHttpRequest();
  mote_id++;
}

void InitHttpRequest()
{
  sim900.println("AT+CSQ");
  delay(100);
 
  ShowSerialData();// this code is to show the data from gprs shield, in order to easily see the process of how the gprs shield submit a http request, and the following is for this purpose too.
 
  sim900.println("AT+HTTPINIT"); //init the HTTP request
 
  delay(2000); 
  ShowSerialData();

  Serial.println("AT+HTTPPARA=\"URL\",\"150.162.10.20:8080\"");// setting the httppara, the second parameter is the website you want to access
  delay(1000);
 
  ShowSerialData();
}

void FinishHttpRequest()
{
  sim900.println("AT+HTTPACTION=1");// POST session start
  delay(10000);//the delay is very important, the delay time is base on the return from the website, if the return datas are very large, the time required longer.
  
  ShowSerialData();
 
  sim900.println("AT+HTTPTERM");
  delay(300);
 
  ShowSerialData();
}

boolean sendCommandAndWaitResp(String comando, String respostaEsperada, int tentativas, int delayEntreTentativas) {
    String resposta;
    while (tentativas > 0) {

        sim900.println(comando);
        delay(delayEntreTentativas);

        resposta = "";
        while(sim900.available()!=0) {
            resposta.concat((char)sim900.read());
        }

        tentativas--;
    }
    return false;
}
 
void ShowSerialData() {
  String resposta = "";
  while(Serial.available() != 0) {
    resposta.concat((char)Serial.read());
    delay(10);
  }

  if (resposta.length() > 0) {
      Serial.println(resposta);
     splitAndSend(resposta);
  }
}
