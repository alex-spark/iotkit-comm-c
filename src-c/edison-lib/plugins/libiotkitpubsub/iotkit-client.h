/*
 * IoTKit client plugin to enable subscribe feature through Edison API
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

#ifndef EDISON_IOTKIT_ASYNC_H_
#define EDISON_IOTKIT_ASYNC_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "edisonapi.h"

#include <MQTTAsync.h>

#define CLIENTID    "IoTClientSub"
#define QOS         1
#define TIMEOUT     10000L
volatile MQTTAsync_token deliveredtoken;


#ifndef DEBUG
    #define DEBUG 0
#endif

volatile int toStop = 0;
 volatile int finished = 0;
 volatile int subscribed = 0;
 volatile int connected = 0;
 volatile int quietMode = 0;
 volatile int sent = 0;
 volatile int delivery = 0;

char *interface = "edison-iotkit-client-interface"; // specifies the plugin interface json

void registerSensor(char *sensorname, char *type);
int init(void *serviceDesc);
int send(char *message, Context context);
int subscribe();
int unsubscribe(char *topic);
int receive(void (*) (char *topic, Context context));
//values for type --> open, ssl
//int createClient(char *host, int port, char *type, void *sslargs);
//int createService();
int done();


#endif /* EDISON_IOTKIT_ASYNC_H_ */
