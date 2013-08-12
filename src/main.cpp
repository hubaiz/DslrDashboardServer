//============================================================================
// Name        : DDServer.cpp
// Author      : Zoltan Hubai
// Version     :
// Copyright   : Copyright by Zoltan Hubai
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <pthread.h>
#include "main.h"
#include "communicator.h"

using namespace std;

int main() {

	cout << "DslrDashboardServer" << endl; // prints DslrDashboardServer

	startSocketServer();

	return 0;
}

void startSocketServer() {
	int sockfd, portno = 4757;
	int clientSocket;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	syslog(LOG_INFO, "Starting socket server on port %d", portno);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		syslog(LOG_ERR, "ERROR opening socket: %d", sockfd);
		return;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		syslog(LOG_ERR, "ERROR on binding");
		return;
	}
	listen(sockfd, 5);

	clilen = sizeof(cli_addr);

	while (true) {
		syslog(LOG_INFO, "Awaiting client connection");
		clientSocket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (clientSocket < 0) {
			syslog(LOG_ERR, "Error on accept client connection %d", clientSocket);
		} else {
			syslog(LOG_INFO, "Incoming client connection");

			pthread_t myThread;

			int r = pthread_create(&myThread, NULL, clientThread, &clientSocket);

			if (r)
				syslog(LOG_ERR, "error creating client thread");
			// close client socket
//			close(clientSocket);
//			syslog(LOG_INFO, "Client finished");
		}
	}
}

void * clientThread(void * param) {
	int clientSocket = *(unsigned int *)param;

	Communicator communicator(clientSocket);
	communicator.handleClientConnection();

	pthread_exit(NULL);
}
