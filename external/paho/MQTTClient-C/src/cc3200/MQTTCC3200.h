/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef __MQTT_CC3200_
#define __MQTT_CC3200_


#include <stdint.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include "netapp.h"
#include "unistd.h"
#include "time.h"
#include "systick.h"

#ifdef SECURE_SOCKET

#define ROOTCA_FILENAME								"/certs/rootca.crt"
#define PRIVATEKEY_FILENAME 					""
#define TRUSTED_CERTIFICATE_FILENAME 	""

typedef struct SecurityParams {
	unsigned char sec_method;
	unsigned int cipher;
	char server_verify;
}SecureParams_t;

#endif //SECURE_SOCKET

typedef struct Timer Timer;

struct Timer {
	unsigned long systick_period;
	unsigned long end_time;
};

typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

void TimerInit(Timer*);

int cc3200_read(Network*, unsigned char*, int, int);
int cc3200_write(Network*, unsigned char*, int, int);
void cc3200_disconnect(Network*);
void NetworkInit(Network*);

int NetworkConnect(Network*, char*, int, void*);
//int TLSConnectNetwork(Network*, char*, int, SlSockSecureFiles_t*, unsigned char, unsigned int, char);

#endif
