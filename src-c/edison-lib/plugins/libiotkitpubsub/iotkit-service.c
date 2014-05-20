/*
 * The IoT Kit Agent publishes data to Cloud through Edison API
 * Copyright (c) 2014, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 */

#include "iotkit-service.h"
#include "dlfcn.h"

#include <signal.h>

void *handle=NULL;
char *err=NULL;

MQTTAsync client;
MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;


void (*msgArrhandler) (char *message, Context context) = NULL;

void handleSignal(int sig)
 {
    #if DEBUG
  	    printf("got interruption signal");
  	#endif

 	toStop = 1;
 }

 int messageArrived(void *ctx, char *topicName, int topicLen, MQTTAsync_message *message)
 {
 	int i;
 	char* payloadptr;
 	char *payloadmsg;
 	Context context;

    #if DEBUG
        printf("Message arrived\n");
        printf("topic: %s\n", topicName);
        printf("message:");

        payloadptr = message->payload;
        for(i=0; i<message->payloadlen; i++)
        {
            putchar(*payloadptr++);
        }
        putchar('\n');
 	#endif

    payloadmsg = (char *)malloc(message->payloadlen+1);
    strncpy(payloadmsg, message->payload, message->payloadlen+1);


    context.name = "topic";
    context.value = strdup(topicName);
    if(msgArrhandler != NULL){
        msgArrhandler(payloadmsg, context); // TODO: send {event: message, topic: topic}
    } else {
        printf("error: Receive Handler not set\n");
    }

 	MQTTAsync_freeMessage(&message);
 	MQTTAsync_free(topicName);
 	return 1;
 }

 void onSubscribe(void* context, MQTTAsync_successData* response)
 {
    #if DEBUG
 	    printf("Subscribe succeeded\n");
 	#endif
 }

 void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
 {
    #if DEBUG
 	    printf("Subscribe failed\n");
 	#endif

 	finished = 1;
 }

 void onDisconnect(void* context, MQTTAsync_successData* response)
 {
    #if DEBUG
 	    printf("Successful disconnection\n");
 	#endif

 	finished = 1;
 }


 void onSendFailure(void* context, MQTTAsync_failureData* response)
 {
    #if DEBUG
        printf("onSendFailure: message with token value %d delivery failed\n", response->token);
	#endif
 }



 void onSend(void* context, MQTTAsync_successData* response)
 {
	static last_send = 0;

	if (response->token - last_send != 1)
		printf("Error in onSend, token value %d, last_send %d\n", response->token, last_send);

	last_send++;

 	if ((response->token % 1000) == 0)
 		printf("onSend: message with token value %d delivery confirmed\n", response->token);
 }

 void deliveryComplete(void* context, MQTTAsync_token token)
 {
 	sent++;
 	if ((sent % 1000) == 0)
 		printf("deliveryComplete: message with token value %d delivery confirmed\n", token);
	if (sent != token)
		printf("Error, sent %d != token %d\n", sent, token);
//	if (sent == options.message_count)
//		toStop = 1;
 }

 void onConnectFailure(void* context, MQTTAsync_failureData* response)
 {
 	printf("Connect failed\n");
 	finished = 1;
 }


 void onConnect(void* context, MQTTAsync_successData* response)
 {
    #if DEBUG
 	    printf("Connected\n");
 	#endif

 	connected=1;
 }

 void connectionLost(void *context, char *cause)
 {
 	MQTTAsync client = (MQTTAsync)context;
 	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
 	int rc;

    #if DEBUG
 	    printf("\nConnection lost\n");
 	    printf("     cause: %s\n", cause);


 	    printf("Reconnecting\n");
 	#endif

 	conn_opts.keepAliveInterval = 20;
 	conn_opts.cleansession = 1;
 	conn_opts.onSuccess = onConnect;
 	conn_opts.onFailure = onConnectFailure;
 	conn_opts.context = client;
 	conn_opts.retryInterval = 1000;
 	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
 	{
 		printf("Failed to start connect, return code %d\n", rc);
 		finished = 1;
 	}
 }


#if DEBUG
void handleTrace(enum MQTTASYNC_TRACE_LEVELS level, char* message)
{
        printf("%s\n", message);
}
#endif


int registerSensor(char *sensorname, char *type, char *unit) {
    char mesg[256];
    sprintf(mesg, "{\"n\":%s,\"t\":%s}", sensorname,type);
//    {"firstname":"JESPER","surname":"AABERG","phone":["555-0100","555-0120"]};
    publish("data", mesg);
}

/**
 * @name initialise the MQTT service
 * @brief initialises the plugin.
 * @param[in] serviceDesc is the service description being registered for
 * @return boolean which specifies whether the connection is successfully established or not
 *
 * Establishes the connection with an MQTT broker
 */
int init(void *serviceDesc)
 {

    ServiceDescription *serviceDescription = (ServiceDescription *)serviceDesc;
 	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
 	int rc = 0;
 	char uri[256];

//         if(strcmp(type, "ssl") == 0){
//             sprintf(uri, "ssl://%s:%d", host, port);
//         }else {

            if(serviceDescription->address != NULL){
 	 	        sprintf(uri, "tcp://%s:%d", serviceDescription->address, serviceDescription->port);
 	 	    } else {
 	 	        sprintf(uri, "tcp://localhost:%d", serviceDescription->port);
 	 	    }
// 	 	}
 		// Default settings:
 	 	int i=0;


        #if DEBUG
            printf("URL:%s\n", uri);
        #endif

 	 	MQTTAsync_token token;

 	 	quietMode = 0;

        #if DEBUG
 	 	    printf("\nGoing to mqtt async create\n");
 	 	#endif
 	 	MQTTAsync_create(&client, uri, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

#if DEBUG
 	    printf("\ncompleted mqtt async create\n");
 		MQTTAsync_setTraceCallback(handleTrace);
 		MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_PROTOCOL);
#endif

 	 	MQTTAsync_setCallbacks(client, client, connectionLost, messageArrived, deliveryComplete);

 	 	conn_opts.cleansession = 0;
 	 	conn_opts.onSuccess = onConnect;
 	 	conn_opts.onFailure = onConnectFailure;
 	 	conn_opts.context = client;
 	 	conn_opts.keepAliveInterval = 0;
 	 	conn_opts.retryInterval = 0;
 	 	//conn_opts.maxInflight= 30;

         /*
 	 	// TODO: SSL based client needs to be implemented
 	 	conn_opts.ssl = &sslopts;
 	 	conn_opts.ssl->trustStore = "./certs/client.crt";
 	 	conn_opts.ssl->keyStore = "./certs/client.key";
 	 	conn_opts.ssl->enableServerCertAuth = 0;
 	 	*/

 	 	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
 	 	{
 	 		printf("Failed to start connect, return code %d\n", rc);
 	 		exit(1);
 	 	}

 	 	#if DEBUG
 	 	    printf("Waiting for connect\n");
 	 	#endif

 	 	while (connected == 0 && finished == 0 && toStop == 0) {

 	 	    #if DEBUG
 	 		    printf("Waiting for connect: %d %d %d\n", connected, finished, toStop);
 	 		#endif

 	 		sleep(1);
 	 	}

     return rc;
 }

/**
 * @name publish to a topic
 * @brief publish to a topic with an MQTT broker
 * @param[in] message which belongs to the topic
 * @param[in] context a context object
 * @return boolean which specifies whether successfully published or not
 */
 int sendTo(char *message,Context context) {

 	char *topic;
    if(context.name != NULL && context.value != NULL && strcmp(context.name, "topic") == 0){
        topic = context.value;
    }
    else {
        printf("Topic not available in the send command");
        return -1;
    }
    return publish(topic,message);
 }


/**
 * @name publish to a topic
 * @brief publish to a topic with an MQTT broker
 * @param[in] topic which needs to be subscribed to
 * @param[in] message based on the topic
 */
 int publish(char *topic, char *message){
    #if DEBUG
         printf("Invoked IOT-KIT: publish()\n");
     #endif
     	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
      	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

      	int rc = 0;
      	unsigned long i;
        struct timeval tv;
        gettimeofday(&tv,NULL);

        #if DEBUG
            printf("start seconds : %ld\n",tv.tv_sec);
        #endif

            opts.onSuccess = onSend;
            opts.onFailure = onSendFailure;
            opts.context = client;
            pubmsg.payload = message;
            pubmsg.payloadlen = strlen(message);
            pubmsg.qos = QOS;
            pubmsg.retained = 0;
            deliveredtoken = 0;
            usleep(100);

            if ((rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts))
                != MQTTASYNC_SUCCESS)
            {
                printf("Failed to start sendMessage, return code %d\n", rc);
                exit(-1);
            }


        gettimeofday(&tv,NULL);


         #if DEBUG
            printf("end seconds : %ld\n",tv.tv_sec);
        #endif

      	return rc;

 }


int manageClient(char *topic){
    #if DEBUG
        printf("Invoked IOT-KIT: manageClient()\n");
    #endif
}

// TODO: validate whether we need to return any value or not
/**
 * @name subscribe to a topic
 * @brief registers the client's callback to be invoked on receiving a message from MQTT broker
 * @param handler to be registered as a callback
 */
int receive(void (*handler) (char *message, Context context)){

    #if DEBUG
        printf("Invoked IOT-KIT: setReceivedMessageHandler()\n");
    #endif

    msgArrhandler = handler;
}

/**
 * @name cleanup the MQTT client
 * @brief used to close the connections and for cleanup activities
 * @return boolean which specifies whether the connection is disconnected or not
 */
 int done() {

 	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;

 	int rc = 0;

 	opts.onSuccess = onDisconnect;
 	opts.context = client;

 	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
 	{
 		printf("Failed to start disconnect, return code %d\n", rc);
 	 	exit(-1);
 	}
 	finished = 1;
 	toStop = 0;

 	MQTTAsync_destroy(&client);

 	return rc;
 }

