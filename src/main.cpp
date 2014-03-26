/*
 * main.cpp
 *
 *  Created on: Dec 28, 2013
 *      Author: hubaiz
 */

#include "main.h"


using namespace std;

int main() {

	syslog(LOG_INFO, "DslrDashboardServer starting"); // prints DslrDashboardServer

//	if (fork() == 0)
//		startUdpListener();

	pthread_t myThread;
	int r = pthread_create(&myThread, NULL, udpThread, NULL);

	if (r)
		syslog(LOG_ERR, "error creating client thread");

	startSocketServer(TCP_PORT);

	return 0;
}

void * udpThread(void *param) {
	startUdpListener();

	pthread_exit(NULL);
}

int createUdpSocket()
{
	struct sockaddr_in LocalHost;
	int             UDPsocket;

	LocalHost.sin_family = AF_INET;
	LocalHost.sin_port = htons(UDP_PORT);
	LocalHost.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((UDPsocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "can't create UDP socket: %d", UDPsocket);
		return -1;
	}
	reusePort(UDPsocket);

	if (bind(UDPsocket, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {
		syslog(LOG_ERR, "Error binding UDP socket");
		close(UDPsocket);
		return -1;
	}
	return UDPsocket;

}
void startUdpListener() {

	syslog(LOG_INFO, "Starting UDP listener");

//	struct sockaddr_in LocalHost;
	int             UDPsocket;
//
//	LocalHost.sin_family = AF_INET;
//	LocalHost.sin_port = htons(UDP_PORT);
//	LocalHost.sin_addr.s_addr = htonl(INADDR_ANY);
//
	struct sockaddr_in GroupAddress;

	GroupAddress.sin_family = AF_INET;
	GroupAddress.sin_port = htons(UDP_PORT);
	GroupAddress.sin_addr.s_addr = inet_addr(UDP_GROUP);
//
//	if ((UDPsocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
//		syslog(LOG_ERR, "can't create UDP socket: %d", UDPsocket);
//		exit(-1);
//	}
//	reusePort(UDPsocket);
//
//	if (bind(UDPsocket, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {
//
//		syslog(LOG_ERR, "Error binding UDP socket");
//
//		exit(-1);
//	}

	while ((UDPsocket = createUdpSocket()) < 0) {
		syslog(LOG_INFO, "Error creating UDP socket, retry after 3 seconds");
		sleep(3);
	}
	joinGroup(UDPsocket, (char *)UDP_GROUP);

	/** allow multicast datagrams to be transmitted to a distance
            according to the value put in "TimeToLive" variable */

	u_char TimeToLive = 2;
	setTTLvalue(UDPsocket, &TimeToLive);

	u_char loop = 0;		/* enable loopback */
	setLoopback(UDPsocket, loop);


	const int				on = 1;
	if (setsockopt(UDPsocket, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0){
		//printf("Error settings IP_PKTINFO");
		syslog(LOG_ERR, "Error settings IP_PKTINFO");
	}

	ssize_t             bytes = 0;
	char recvBuf[MAX_LEN]; //, str[INET6_ADDRSTRLEN];
	char cmbuf[0x100];
	struct msghdr mh;
	struct iovec	iov[1];

	for (;;) {

		// the remote/source sockaddr is put here
		struct sockaddr_in peeraddr;

		mh.msg_name = &peeraddr;
		mh.msg_namelen = sizeof(peeraddr);
		mh.msg_control = &cmbuf[0];
		mh.msg_controllen = sizeof(cmbuf);

		iov[0].iov_base = recvBuf;
		iov[0].iov_len = MAX_LEN;
		mh.msg_iov = iov;
		mh.msg_iovlen = 1;

		bytes = recvmsg(UDPsocket, &mh, 0);

		if (bytes > 0) {

			syslog(LOG_INFO, "recv: %d", bytes);

			int diff = strncmp(&recvBuf[0], DD_CLIENT, sizeof(DD_CLIENT)-1);
			if (diff == 0)
			{
				syslog(LOG_INFO, "Client multicast query %s", recvBuf);
				int len = sizeof(DD_SERVER) - 1 + bytes - (sizeof(DD_CLIENT) -1);
				char *buf = (char *)malloc(len);
				strncpy(buf, DD_SERVER, sizeof(DD_SERVER)-1);
				strncpy(&buf[sizeof(DD_SERVER)-1], &recvBuf[sizeof(DD_CLIENT)-1], bytes-(sizeof(DD_CLIENT) - 1));
				syslog(LOG_INFO, "Server message length: %d   msg: %s",len, buf);
				if (sendto(UDPsocket, buf, len, 0, (SA *) & GroupAddress, sizeof(GroupAddress)) < 0) {
						syslog(LOG_ERR, "Error sending multicast response");
						exit(-1);
				}

				free(buf);
			}

//			for ( // iterate through all the control headers
//					struct cmsghdr *cmsg = CMSG_FIRSTHDR(&mh);
//					cmsg != NULL;
//					cmsg = CMSG_NXTHDR(&mh, cmsg))
//			{
//				// ignore the control headers that don't match what we want
//				if (cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_PKTINFO)
//					continue;
//
//				struct in_pktinfo *pi = (in_pktinfo *)CMSG_DATA(cmsg);
//
//				// at this point, peeraddr is the source sockaddr
//				// pi->ipi_spec_dst is the destination in_addr
//				// pi->ipi_addr is the receiving interface in_addr
//
//				syslog(LOG_INFO,"tst %s %d %s", inet_ntoa(pi->ipi_addr), pi->ipi_ifindex, inet_ntoa(pi->ipi_spec_dst));
//
//				write(1, recvBuf, bytes);
//			}
		}
	}
}
//		memset(recvBuf, '\0', MAX_LEN);
//		//bytes = Recvfrom_flags(UDPsocket, recvBuf, MAX_LEN, &flags, (SA *)&GroupAddress, &len, &pktinfo);
//
//		bytes = recv(UDPsocket, recvBuf, MAX_LEN, 0);
//		//printf("intf %d %d", pktinfo.ipi_ifindex, pktinfo.ipi_addr);
//
//		syslog(LOG_INFO, "intf %d %d", pktinfo.ipi_ifindex, pktinfo.ipi_addr);
//
//		if (bytes < 0) {
//			syslog(LOG_ERR, "error in reading from multicast socket");
//			exit(-1);
//		}
///*
//                else if (bytes == 0)
//			printf("zero bytes read\n");
//*/
//		else {		/* print the message to STDOUT */
//			syslog(LOG_INFO, "message: %s", recvBuf);
//			if (write(1, recvBuf, bytes) < 0) {
//				syslog(LOG_ERR, "error in write to STDOUT ");
//				exit(-1);
//			}
//
//			if (sendto(UDPsocket, recvBuf, MAX_LEN, 0, (SA *) & GroupAddress, sizeof(GroupAddress)) < 0) {
//						printf("error in sendto \n");
//						exit(-1);
//					}
//		}
//	}
//
//}

void startSocketServer(int port) {
	int sockfd;
	int clientSocket;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	syslog(LOG_INFO, "Starting socket server on port %d", port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		syslog(LOG_ERR, "ERROR opening socket: %d", sockfd);
		return;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

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

	Communicator communicator;
	communicator.handleClientConnection(clientSocket);

	pthread_exit(NULL);
}

/*
 * This function sets the socket option to make the local host join the
 * mulicast group
 */

void joinGroup(int s, char *group)
{
	syslog(LOG_INFO, "join multicast group: %s", group);
	struct sockaddr_in groupStruct;
	struct ip_mreq  mreq;	/* multicast group info structure */

	if ((groupStruct.sin_addr.s_addr = inet_addr(group)) == -1)
		syslog(LOG_ERR, "error in inet_addr");

	/* check if group address is indeed a Class D address */
	mreq.imr_multiaddr = groupStruct.sin_addr;
	mreq.imr_interface.s_addr = INADDR_ANY;

	int r = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq));
	if ( r < 0) {
		syslog(LOG_ERR, "error in joining multicast group %d", r);
	}
}

/*
 * This function removes the process from the group
 */

void leaveGroup(int recvSock, char *group)
{
	struct sockaddr_in groupStruct;
	struct ip_mreq  dreq;	/* multicast group info structure */

	if ((groupStruct.sin_addr.s_addr = inet_addr(group)) == -1)
		syslog(LOG_ERR, "error in inet_addr");

	dreq.imr_multiaddr = groupStruct.sin_addr;
	dreq.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(recvSock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		       (char *) &dreq, sizeof(dreq)) == -1) {
		syslog(LOG_ERR, "error in leaving multicast group");
	}
	syslog(LOG_INFO, "process quitting multicast group %s ", group);
}


/*
 * This function sets a socket option that allows multipule processes to bind
 * to the same port
 */

void reusePort(int s)
{
	int             one = 1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one)) == -1) {
		syslog(LOG_ERR, "error in setsockopt,SO_REUSEPORT");
	}
}

/*
 * This function sets the Time-To-Live value
 */

void setTTLvalue(int s, u_char * ttl_value)
{
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *) ttl_value,
		       sizeof(u_char)) == -1) {
		syslog(LOG_ERR, "error in setting multicast TTL value");
	}
}


/*
 * By default, messages sent to the multicast group are looped back to the local
 * host. this function disables that. loop = 1  means enable loopback loop
 * = 0  means disable loopback NOTE : by default, loopback is enabled */

void setLoopback(int s, u_char loop)
{
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &loop,
		       sizeof(u_char)) == -1) {
		syslog(LOG_ERR, "error in disabling loopback");
	}
}

