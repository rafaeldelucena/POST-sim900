#include <SoftwareSerial.h>
#include <string.h>

SoftwareSerial sim900(7, 8);
int statusConfig;
boolean configAPN();
void splitAndSend(String text);
boolean verifyMessage(String text);
void addSensor(String mote_id, String name, String value);
void addMote(String rxPackets, String txPackets, String rxBytes, String txBytes);
void initHttpRequest();
void finishHttpRequest();
void postMethodHttp();
boolean sendCommandAndWaitResp(String comando, String respostaEsperada, int tentativas, int delayEntreTentativas);
void ShowSerialData();
void debugMessage(String);

int mote_id=1;

void setup()
{
    sim900.begin(19200);    // the GPRS baud rate   
    Serial.begin(19200);    // the GPRS baud rate 
    statusConfig = 0;
}

boolean configAPN()
{
    if(!sendCommandAndWaitResp("AT+CIPSHUT", "OK", 3, 500)) {
        return false;
    }

    if (!sendCommandAndWaitResp("AT+CGATT?", "OK", 1, 100)) {
        return false;
    }

    if (!sendCommandAndWaitResp("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 3, 2000)) { //setting the SAPBR, the connection type is using gprs
        return false;
    }
    if (!sendCommandAndWaitResp("AT+CSTT=\"tim.br\", \"tim\", \"tim\"", "OK", 3, 1000)) {
        return false;
    }
    if (!sendCommandAndWaitResp("AT+SAPBR=1,1", "OK", 2, 1000)) { //setting the SAPBR, for detail you can refer to the AT command manual
        return false;
    } else {

    statusConfig = 1;
    return true;
    }
}

void loop()
{
    if (statusConfig == 0) {
        configAPN();
    }
    String input;
    if (Serial.available()) {
        while(Serial.available() != 0) {
            input.concat((char)Serial.read());
        }
    }

    if (input.length() >= 0) {
        splitAndSend(input);
    }
    delay(100);

    ShowSerialData();
}


// Recebe uma String "::key;value::"
// Separa em "key" "value"
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
    initHttpRequest();

    String dataPost = "_method=POST&data[Sensor][name]=" + name + "&data[Sensor][value]=" + value + "&data[Sensor][mote_id]=" + mote_id;

    String httpData = "AT+HTTPDATA=";
    String delayToSend = ",3000";

    sim900.println(httpData + dataPost.length() + delayToSend);// It is ready to receive data from uart , and DCD has been set to low, the arguments are size in bytes and timeout to send a message

    sim900.print(dataPost);
    finishHttpRequest();
}

void addMote(int id, String rxPackets, String txPackets, String rxBytes, String txBytes)
{
    initHttpRequest();
    String httpData = "AT+HTTPDATA=";
    String delayToSend = ",3000";

    String postData = "data[Mote][rx_packets]="+ rxPackets + "&data[Mote][tx_packets]=" + txPackets + "&data[Mote][rx_bytes]="+ rxBytes + "&data[Mote][tx_bytes]=" + txBytes;

    sendCommandAndWaitResp(httpData + postData.length() + delayToSend, "OK", 3, 3000);// It is ready to receive data from uart , and DCD has been set to low, the arguments are size in bytes and timeout to send a message

    sim900.print(postData);
    delay(3000);

    finishHttpRequest();
    mote_id++;
}

void initHttpRequest()
{
    sim900.println("AT+CSQ");
    delay(100);

    ShowSerialData();// this code is to show the data from gprs shield, in order to easily see the process of how the gprs shield submit a http request, and the following is for this purpose too.

    sendCommandAndWaitResp("AT+HTTPINIT", "OK", 3, 2000); //init the HTTP request
    ShowSerialData();

    sendCommandAndWaitResp("AT+HTTPPARA=\"URL\",\"150.162.10.20:8080\"", "OK", 3, 2000);// setting the httppara, the second parameter is the website you want to access
    ShowSerialData();
}

void postMethodHttp()
{
    // TODO: Verificar o status
    sendCommandAndWaitResp("AT+HTTPACTION=1", "OK", 3, 8000);// POST session start
    ShowSerialData();
}

void finishHttpRequest()
{
    sendCommandAndWaitResp("AT+HTTPTERM", "OK", 3 , 1000);
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
    String resposta;
    while(sim900.available()!=0) {
        resposta.concat((char)sim900.read());
    }

    if (resposta.indexOf("ERROR") >= 0) {
        debugMessage("!!! CME ERROR: "+ resposta + "!!!");
        statusConfig = 0;
    }
    if(resposta.length() > 0) {
        Serial.println(resposta);
    }
}

void debugMessage(String msg)
{
    Serial.println("DEBUG: " + msg);
}
