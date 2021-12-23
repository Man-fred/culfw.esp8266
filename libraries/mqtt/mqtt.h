#ifndef __MQTT_H_
#define __MQTT_H_

#include "board.h"
#include <ESP8266WiFi.h>
#include "fncollection.h"

#define MAX_MESSAGES 40
#define MQTT_MAX_TOPIC_SIZE 50
#define MQTT_MAX_PACKET_SIZE 256 // default 128

#include <PubSubClient.h>

// Subscribe:   <_pre><_clientname>/<_sub>
// LWT:         <_pre><_clientname>/<_lwt>
// Publish:     <_pre><_clientname>/<key [from set()]>
// acknowledge: <_pre><_clientname>/ack

class MqttClass : public PubSubClient {
public:
	MqttClass() : PubSubClient() {};
	MqttClass(Client& client) :   PubSubClient(client) {};
	
	void init(void);
	char* Task(void);
	void failed(void);
	void func(char *);
  void set(char* key, char* value, boolean retain, int qos);
	void set(char* key, char* value, boolean retain);
	void set(char* key, char* value);
	void send();
 	void callback(char* topic, byte* payload, unsigned int mLength);

private:
  boolean test();
	//PubSubClient client;
  boolean isConnected;

  char _server[EE_STR_LEN];
  int  _port;
  char _user[EE_STR_LEN];
  char _pass[EE_STR_LEN];
  char _clientname[EE_STR_LEN];
  
  char _pre[EE_STR_LEN];
  char _sub[EE_STR_LEN];
  char _lwt[EE_STR_LEN];

	// buffers for receiving and sending data
	char mPayloadKey[MAX_MESSAGES][MQTT_MAX_TOPIC_SIZE];
	char mPayloadValue[MAX_MESSAGES][MQTT_MAX_PACKET_SIZE];
	boolean mPayloadRetain[MAX_MESSAGES];
	int mPayloadQos[MAX_MESSAGES];
	int mPayloadSet = 0;
	int mPayloadPublish = 0;

  char* tochararray(char* cvalue, int value);
  char* tochararray(char* cvalue, unsigned int value);
  char* tochararray(char* cvalue, unsigned int value, int radix);
  char* tochararray(char* cvalue, float value, int len, int nachkomma);
  char* tochararray(char* cvalue, char* value1, char value2);
  char* tochararray(char* cvalue, char* value1, char* value2);
  char* tochararray(char* cvalue, char* value1);
  char* tochararray(char* cvalue, String value1);
  char* tochararray(char* cvalue, String value1, String value2);

  // last published payload
	char cstr[MQTT_MAX_PACKET_SIZE];
	char cmessage[MQTT_MAX_PACKET_SIZE];
	char cpart[MQTT_MAX_PACKET_SIZE];
	
	// last subscribed payload
  char smessage[MQTT_MAX_PACKET_SIZE];
	boolean taskActive;
	boolean newMessage;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MQTT)
//extern MqttClass mqtt();
#endif

#endif
