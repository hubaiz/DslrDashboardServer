/*
 * main.h
 *
 *  Created on: Dec 28, 2013
 *      Author: hubaiz
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "communicator.h"

#define DD_CLIENT "DslrDashboardClient"
#define DD_SERVER "DslrDashboardServer"
#define TCP_PORT 4757
#define UDP_PORT 4757
#define UDP_GROUP "224.1.2.3"
#define SA      struct sockaddr
#define MAX_LEN 512
#define HAVE_MSGHDR_MSG_CONTROL 1

int createUdpSocket();
void startSocketServer(int port);
void * udpThread(void *);
void startUdpListener();
void * clientThread(void *);
void joinGroup(int s, char *group);
void leaveGroup(int recvSock, char *group);
void reusePort(int s);
void setTTLvalue(int s, u_char * ttl_value);
void setLoopback(int s, u_char loop);
void displayDaddr(int s);


#endif /* MAIN_H_ */
