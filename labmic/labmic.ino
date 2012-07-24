#include <SoftwareSerial.h>
#include <string.h>

SoftwareSerial sim900(7, 8);
int statusConfig;
String host;
String apn;
boolean configAPN(String apn);
void splitAndSend(String text);
void addSensor(String mote_id, String name, String value);
void addMote(String rxPackets, String txPackets, String rxBytes, String txBytes);
void initHttpRequest(String ip);
void finishHttpRequest();
void postMethodHttp();
boolean sendCommandAndWaitResp(String comando, String respostaEsperada, int tentativas, int delayEntreTentativas);
void debugMessage(String);
String getSerialData();
String getModemData();

void setup()
{
    sim900.begin(19200);    // the GPRS baud rate   
    Serial.begin(19200);    // the GPRS baud rate 
    statusConfig = 0;
    host = "127.0.0.1";
    apn = "tim";
}

boolean configAPN(String apn)
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
    if (!sendCommandAndWaitResp("AT+CSTT=\"" + apn +".br\", \"" + apn + "\", \"" + apn +"\"", "OK", 3, 1000)) {
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
        configAPN(apn);
    }
    String data = String(getSerialData());
    if (data.length() > 0) {
        splitAndSend(data);
    }
    delay(100);
}
void splitAndSend(String text)
{
    String id = getId(text);
    String key = getKey(text);
    String value = getValue(text);
    if ((id.length() > 0) && (key.length() > 0) && (value.length() > 0)) {
        addSensor(id, key, value);
    }
}

String getId(String text) {
    int a, b;
    String c = "{";
    String d = "}";
    String id = cutString(text, a, b);
    return id;
}

String getKey(String text) {
    int a, b;
    String c = "<\\";
    String d = "\\,";
    
    a = text.indexOf(c);
    b = text.indexOf(d);
    String key = cutString(text, a, b);
    return key;
}

String getValue(String text) {
    int a, b;
    String c = ",\\";
    String d = "\\>";
    a = text.indexOf(c);
    b = text.indexOf(d);
    String value = cutString(text, a, b);
    return value;
}

String cutString(String s, int inicio, int fim)
{
    String str = "";
    while(inicio < fim) {
        str.concat((char) s.charAt(inicio++));
    }
    if (str > 0) {
        return str;
    }
}

void addSensor(String mote_id, String name, String value)
{
    initHttpRequest(host);

    String dataPost = "data[Sensor][name]=" + name + "&data[Sensor][value]=" + value + "&data[Sensor][mote_id]=" + mote_id;
    
    String httpData = "AT+HTTPDATA=";
    String delayToSend = ",3000";

    postMethodHttp();

    sendCommandAndWaitResp(httpData + dataPost.length() + delayToSend, "OK", 3, 5000);// It is ready to receive data from uart , and DCD has been set to low, the arguments are size in bytes and timeout to send a message

    sim900.print(dataPost);
    delay(100);

    finishHttpRequest();
}

void addMote(int id, String rxPackets, String txPackets, String rxBytes, String txBytes)
{
    initHttpRequest(host);
    String httpData = "AT+HTTPDATA=";
    String delayToSend = ",3000";


    String postData = "data[Mote][rx_packets]="+ rxPackets + "&data[Mote][tx_packets]=" + txPackets + "&data[Mote][rx_bytes]="+ rxBytes + "&data[Mote][tx_bytes]=" + txBytes;

    postMethodHttp();
    
    sendCommandAndWaitResp(httpData + postData.length() + delayToSend, "OK", 3, 5000);// It is ready to receive data from uart , and DCD has been set to low, the arguments are size in bytes and timeout to send a message

    sim900.print(postData);
    delay(100);

    finishHttpRequest();
}

void initHttpRequest(String ip)
{
    sim900.println("AT+CSQ");
    delay(100);

    sendCommandAndWaitResp("AT+HTTPINIT", "OK", 3, 2000); //init the HTTP request
    sendCommandAndWaitResp("AT+HTTPPARA=\"URL\",\"" + ip + "\"", "OK", 3, 2000);// setting the httppara, the second parameter is the website you want to access

}

void postMethodHttp()
{
    // TODO: Verificar o status
    sendCommandAndWaitResp("AT+HTTPACTION=1", "OK", 3, 8000);// POST session start
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
        resposta = getModemData();
        if (resposta.indexOf(respostaEsperada) >= 0) {
            debugMessage("OK Comando: " + comando + " Resposta esperada: " + respostaEsperada);
            return true;
        }
        tentativas--;
    }
    debugMessage("ERRO Comando : " + comando + " Resposta esperada: " + respostaEsperada + "Resposta recebida: " + resposta);
    return false;
}

void debugMessage(String msg)
{
    Serial.println("DEBUG: " + msg);
}

String getModemData() {
    String resposta;
    while(sim900.available()!=0) {
        resposta.concat((char) sim900.read());
    }

    if (resposta.indexOf("CLOSED") >= 0) {
        debugMessage("!!! SERVER DOWN !!!");
        statusConfig = 0;
    }

    if(resposta.length() > 0) {
        return resposta;
    }
}

String getSerialData() {
    String data;
    while(Serial.available()!=0) {
        data.concat((char) Serial.read());
    }

    if(data.length() > 0) {
        return data;
    }
}
