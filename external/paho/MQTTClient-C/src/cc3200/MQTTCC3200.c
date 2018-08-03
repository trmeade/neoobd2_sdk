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

#include "MQTTCC3200.h"

unsigned long MilliTimer;

void SysTickIntHandler(void) {
	MilliTimer++;
}

char TimerIsExpired(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}


void TimerCountdown(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + (timeout * 1000);
}


int TimerLeftMS(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}


void TimerInit(Timer* timer) {
	timer->end_time = 0;
}


int cc3200_read(Network* n, unsigned char* buffer, int len, int timeout_ms) {
	SlTimeval_t timeVal;
	SlFdSet_t fdset;
	int rc = 0;
	int recvLen = 0;

	SL_SOCKET_FD_ZERO(&fdset);
	SL_SOCKET_FD_SET(n->my_socket, &fdset);

	timeVal.tv_sec = 0;
	timeVal.tv_usec = timeout_ms * 1000;
	if (sl_Select(n->my_socket + 1, &fdset, NULL, NULL, &timeVal) == 1) {
		do {
			rc = sl_Recv(n->my_socket, buffer + recvLen, len - recvLen, 0);
			recvLen += rc;
		} while(recvLen < len);
	}
	return recvLen;
}


int cc3200_write(Network* n, unsigned char* buffer, int len, int timeout_ms) {
	SlTimeval_t timeVal;
	SlFdSet_t fdset;
	int rc = 0;
	int readySock;

	SL_SOCKET_FD_ZERO(&fdset);
	SL_SOCKET_FD_SET(n->my_socket, &fdset);

	timeVal.tv_sec = 0;
	timeVal.tv_usec = timeout_ms * 1000;
	do {
		/* TBD: If this library is for single socket then sl_Select does not make sense? */
		readySock = sl_Select(n->my_socket + 1, NULL, &fdset, NULL, &timeVal);
	} while(readySock != 1);
	rc = sl_Send(n->my_socket, buffer, len, 0);
	return rc;
}


void cc3200_disconnect(Network* n) {
	sl_Close(n->my_socket);
}


void NetworkInit(Network* n) {
	n->my_socket = 0;  // so this is just for single socket, no multiple socket threads should be implemented
	n->mqttread = cc3200_read;
	n->mqttwrite = cc3200_write;
	n->disconnect = cc3200_disconnect;
}

#if 0
int TLSConnectNetwork(Network *n, char* addr, int port, SlSockSecureFiles_t* certificates, unsigned char sec_method, unsigned int cipher, char server_verify) {
}
#endif

int NetworkConnect(Network* n, char* addr, int port, void * secureArgs)
{
	SlSockAddrIn_t sAddr;
	int addrSize;
	int retVal;
	unsigned long ipAddress;

	sl_NetAppDnsGetHostByName((int8_t*)addr, strlen(addr), &ipAddress, SL_AF_INET);

	sAddr.sin_family = SL_AF_INET;
	sAddr.sin_port = sl_Htons((unsigned short)port);
	sAddr.sin_addr.s_addr = sl_Htonl(ipAddress);

	addrSize = sizeof(SlSockAddrIn_t);

	n->my_socket = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
	if( n->my_socket < 0 ) {
		// error
		return -1;
	}
#ifdef SECURE_SOCKET

	if (secureArgs == NULL) {
		return -1;
	}
	SecureParams_t * args = ((SecureParams_t*)(secureArgs));

	/*Set Protocol, SL_SO_SEC_METHOD_TLSV1_2*/
	SlSockSecureMethod method;
	method.secureMethod = args->sec_method;
	retVal = sl_SetSockOpt(n->my_socket,
												 SL_SOL_SOCKET,
												 SL_SO_SECMETHOD,
												 &method, sizeof(method));
	if (retVal < 0) {
		return retVal;
	}

	/*set cipher, SL_SEC_MASK_TLS_RSA_WITH_AES_128_GCM_SHA256*/
	SlSockSecureMask mask;
	mask.secureMask = args->cipher;
	retVal = sl_SetSockOpt(n->my_socket,
												 SL_SOL_SOCKET,
												 SL_SO_SECURE_MASK,
												 &mask, sizeof(mask));
	if (retVal < 0) {
		return retVal;
	}

	retVal = sl_SetSockOpt( n->my_socket,
													SL_SOL_SOCKET,
													SL_SO_SECURE_FILES_CA_FILE_NAME,
													ROOTCA_FILENAME, strlen( ROOTCA_FILENAME ) );

#ifdef CLIENT_VERIFICATION

	retVal = sl_SetSockOpt( n->my_socket,
													SL_SOL_SOCKET,
													SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,
													PRIVATEKEY_FILENAME, strlen(PRIVATEKEY_FILENAME));

	if (retVal < 0) {
		return retVal;
	}

	retVal = sl_SetSockOpt ( n->my_socket,
													 SL_SOL_SOCKET,
													 SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
													 TRUSTED_CERTIFICATE_FILENAME, strlen(TRUSTED_CERTIFICATE_FILENAME));

	if (retVal < 0) {
	return retVal;
	}

#endif //CLIENT_VERIFICATION

#endif //SECURE_SOCKET

	retVal = sl_Connect(n->my_socket, ( SlSockAddr_t *)&sAddr, addrSize);
	if( retVal < 0 ) {
		// error
		sl_Close(n->my_socket);
		return retVal;
	}

	SysTickIntRegister(SysTickIntHandler);
	SysTickPeriodSet(80000);
	SysTickEnable();

	return retVal;
}
