#include "mqtt.h"

/*
void wifiStaSetup() {
  mqttSet("IP", tochararray(cstr, WiFi.localIP().toString()));
  mqttSet("RSSI", tochararray(cstr, WiFi.RSSI()));
}
*/
const byte hex[17] = "0123456789ABCDEF";

void MqttClass::init()
{
	// IPAddress ip, uint16_t port
  strcpy(_server, "192.168.178.88");
  _port = 1883;
  strcpy(_user, "culfw01");
  strcpy(_pass, "tCc7hTXoEOrMHpXSyOKe");
  strcpy(_clientname, "culfw01");
	strcpy(_pre, "esp/");
  strcpy(_sub, "set");
  strcpy(_lwt, "lwt");

  //DEBUG1_PRINTLN(para.mqtt_server);
	//setClient(WiFi);
  setServer(_server, _port);
	set("init", "up", true, 10);
	taskActive = false;
 	newMessage = false;
}

char* MqttClass::Task(){
  if (test()) {
	  send();
    loop();
  }
  if (!taskActive) {
		//Serial.println("MQTT Task active");
		taskActive = true;
	}
	if (newMessage) {
		//Serial.println("MQTT Task "); Serial.println(smessage);
  	newMessage = false;
		return smessage;
	}
	return "";
}

void MqttClass::callback(char* topic, byte* payload, unsigned int mLength) {
  //Serial.print("mqttRecv "); Serial.println(topic); //Serial.print(": "); Serial.print(" <");    Serial.print(payload); Serial.println(">");
  unsigned int i=0;

	for (i = 0; i < mLength; i++){
    smessage[i] = payload[i];
  }
	smessage[i] = '\n';
	smessage[i+1] = '\0';
  yield();
	//publish("esp/culfw01/ack", payload, mLength, true);
	newMessage = true;
  set("ack", smessage);
	
}


/* MQTT connect-errors
    -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
    -3 : MQTT_CONNECTION_LOST - the network connection was broken
    -2 : MQTT_CONNECT_FAILED - the network connection failed
    -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
    0 : MQTT_CONNECTED - the cient is connected
    1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
    2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
    3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
    4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
    5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
*/
void MqttClass::failed(){
	//Serial.print("failed, rc=");  Serial.println(state());
	set("mqttConnected", tochararray(cstr, state()));
}

boolean MqttClass::test() {
  // Loop until we're reconnected
  if (!connected()) {
    // MQTT disconnected
		if (isConnected){
      failed();
      isConnected = false;
		}
		//DEBUG1_PRINT("MQTT...");
		// Attempt to connect
		// boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage)
		String lSubscribe = _pre;
		lSubscribe += _clientname;
		lSubscribe += "/";
		String lLwt = lSubscribe + _lwt;
		lSubscribe += _sub;
		if (connect(_clientname, _user, _pass, lLwt.c_str(), 0, 1, "down")) {
			//DEBUG1_PRINTLN(" ok");
			// ... and subscribe to topic
			boolean erg = subscribe(lSubscribe.c_str());
			Serial.print("MQTT Connect "); Serial.print(erg); Serial.print(": "); Serial.println(lSubscribe);
			isConnected = true;
			//timer1.deactivate();
			set(_lwt, "up");
		}
	}
	return isConnected;
}

void MqttClass::set(char* key, char* value) {
	set(key, value, true, 0);
}

void MqttClass::set(char* key, char* value, boolean retain) {
	set(key, value, retain, 0);
}

void MqttClass::set(char* key, char* value, boolean retain, int qos) {
  byte payload = strlen(_pre)+strlen(_clientname)+1+strlen(key);
  if (payload < MQTT_MAX_TOPIC_SIZE) {
    strncpy(mPayloadKey[mPayloadSet], _pre, strlen(_pre));
    strncat(mPayloadKey[mPayloadSet], _clientname, strlen(_clientname));
    strncat(mPayloadKey[mPayloadSet], "/", 1);
    strncat(mPayloadKey[mPayloadSet], key, strlen(key));
    mPayloadKey[mPayloadSet][payload] = '\0';
    payload = strlen(value);
    strncpy(mPayloadValue[mPayloadSet], value, payload);
    mPayloadValue[mPayloadSet][payload] = '\0';
    mPayloadRetain[mPayloadSet] = retain;
    mPayloadQos[mPayloadSet] = qos;
    /*
		DEBUG1_PRINT("mqttSet  ");
    DEBUG1_PRINT(mPayloadSet);
    DEBUG1_PRINT(": ");
    DEBUG1_PRINT(mPayloadKey[mPayloadSet]);
    DEBUG1_PRINT(" <");
    DEBUG1_PRINT(value);
    DEBUG1_PRINTLN(">");
		*/
    mPayloadSet++;
    if (mPayloadSet > MAX_MESSAGES-1) {
      mPayloadSet = 0;
    }
    memset(mPayloadKey[mPayloadSet], 0, MQTT_MAX_TOPIC_SIZE);
  }
}

// Sends a payload to the broker
void MqttClass::send() {
  if (test() /*&& mqttConnected*/ && mPayloadKey[mPayloadPublish][0] != '\0') {
    yield();
    int erg = publish(mPayloadKey[mPayloadPublish], mPayloadValue[mPayloadPublish], mPayloadRetain[mPayloadSet]);
    if (!erg && mPayloadQos[mPayloadPublish]-- > 0){
      set(mPayloadKey[mPayloadPublish], mPayloadValue[mPayloadPublish], mPayloadRetain[mPayloadSet], mPayloadQos[mPayloadPublish]);
    }
		
    //Serial.print("mqttSend "); Serial.print(mPayloadPublish); Serial.print(": "); Serial.print(mPayloadKey[mPayloadPublish]); Serial.print(" <");    Serial.print(mPayloadValue[mPayloadPublish]); Serial.println(">");
		
    memset(mPayloadKey[mPayloadPublish], 0, MQTT_MAX_TOPIC_SIZE);
    mPayloadPublish++;
    if (mPayloadPublish > MAX_MESSAGES-1) {
      mPayloadPublish = 0;
    }
  }
}

void MqttClass::func(char *in)
{
  if(in[1] == 'i') {
    init();
  } else if(in[1] == 'p') {
		// Publish
    set("func",in+2);
  } else if(in[1] == 's') {
		// Status
    Serial.print("Connect: "); Serial.print(isConnected); Serial.print(_server); Serial.print(":"); Serial.println(_port);
    Serial.print("Subscribe: "); Serial.print(_pre); Serial.print(_clientname); Serial.print("/"); Serial.println(_sub);
  }
}
char* MqttClass::tochararray(char* cvalue, int value){
  itoa(value, cvalue, 10);
  return cvalue;
}
char* MqttClass::tochararray(char* cvalue, unsigned int value){
  utoa(value, cvalue, 10);
  return cvalue;
}

char* MqttClass::tochararray(char* cvalue, unsigned int value, int radix){
  utoa(value, cvalue, radix);
  return cvalue;
}

char* MqttClass::tochararray(char* cvalue, float value, int len, int nachkomma){
  dtostrf(value, len, nachkomma, cvalue);
  return cvalue;
}

char* MqttClass::tochararray(char* cvalue, char* value1, char value2){
  strcpy(cvalue, value1);
  byte len = strlen(value1);
  cvalue[len] = hex[(value2 & 0xF0) >> 4];
  cvalue[len+1] = hex[value2 & 0x0F];
  cvalue[len+2] = '\0';
  return cvalue;
}
char* MqttClass::tochararray(char* cvalue, char* value1, char* value2){
  strcpy(cvalue, value1);
  if (value2[0] != '\0')
    strcat(cvalue, value2);
  cvalue[strlen(value1)+strlen(value2)] = '\0';
  return cvalue;
}
char* MqttClass::tochararray(char* cvalue, char* value1){
  strcpy(cvalue, value1);
  return cvalue;
}
char* MqttClass::tochararray(char* cvalue, String value1){
  value1.toCharArray(cvalue, MQTT_MAX_PACKET_SIZE);
  return cvalue;
}
char* MqttClass::tochararray(char* cvalue, String value1, String value2){
  char cpart[MQTT_MAX_PACKET_SIZE];
  value1.toCharArray(cvalue, MQTT_MAX_PACKET_SIZE);
  value2.toCharArray(cpart, MQTT_MAX_PACKET_SIZE);
  strcat(cvalue, cpart);
  return cvalue;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MQTT)
//MqttClass mqtt();
#endif