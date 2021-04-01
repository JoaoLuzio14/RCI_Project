/******************************************************************************
 *
 * File Name: connectivity.c
 * Autor:  G19 (RCI 20/21) - João Luzio (IST193096) & José Reis (IST193105)
 * Last Review: 31 Mar 2021
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include "ndn.h"

#define BUFFERSIZE 1024

int regNODE(int regFLAG, char* net, char* nodeIP, char* nodeTCP, char* regIP, char* regUDP){
	char str[100], auxstr[100], buffer[128+1];
	ssize_t n;
	struct addrinfo hints, *res;
	struct sockaddr addr;
	socklen_t addrlen;
	int fd, cnt, errcode;
	fd_set rfds;
	struct timeval tv;

	tv.tv_sec = 5;
    tv.tv_usec = 0;

    /* UDP node server connection */
	fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
	if(fd == -1) exit(1);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP socket

	if((errcode = getaddrinfo(regIP, regUDP, &hints, &res)) != 0){
		fprintf(stderr,"error: getaddrinfo: %s\n", gai_strerror (errcode));
		exit(1);
	}

	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);

	/* Register and Confirm */
	str[0] = '\0';
	auxstr[0] = '\0';
	buffer[0] = '\0';

	if(regFLAG == 1) strcpy(str, "REG ");
	else if(regFLAG == 0) strcpy(str, "UNREG ");

   	msg_build(auxstr, net, nodeIP, nodeTCP);
   	strcat(str, auxstr);
   	// printf("%s\n", str);
   	n = sendto(fd, str, strlen(str), 0, res->ai_addr, res->ai_addrlen);
	if(n == -1) exit(1);
					
	cnt = select(fd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
	if(cnt <= 0){
		printf("\tError establishing connection with node server!\n");
		freeaddrinfo(res);
		close(fd);
		if(regFLAG == 1) return 0;
		else if(regFLAG == 0) return 1;
	}

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, strlen(buffer)-1, 0, &addr, &addrlen);
	if(n == -1) exit(1);
	buffer[n] = '\0';
	// printf("%s\n", buffer);

	/* Close UDP socket */
	freeaddrinfo(res);
	close(fd);

	if(regFLAG == 1){
		if(strcmp(buffer, "OKREG") == 0){
			printf("\tSuccessfully joined %s!\n", net);
			return 1;
		}
		else{
			printf("\tThe connection to the net was not successful.\n");
  			return 0;
		} 
	}
	else if(regFLAG == 0){
		if(strcmp(buffer, "OKUNREG") == 0){
			printf("\tSuccessfully left %s!\n", net);
			return 0;
		}
		else{
			printf("\tThe desconnection to the net was not successful.\n");
  			return 1;
		} 
	}

	printf("An unexpected error as occured!\n");
	exit(1);
}

int getEXT(char* net, char* regIP, char* regUDP, char* bootIP, char* bootTCP){
	
	char str[BUFFERSIZE+1], buffer[BUFFERSIZE+1];
	char matrix[BUFFERSIZE][BUFFERSIZE];
	ssize_t n;
	struct addrinfo hints, *res;
	struct sockaddr addr;
	socklen_t addrlen;
	int fd, cnt, errcode, i, r;
	char *token;


	fd_set rfds;
	struct timeval tv;

	tv.tv_sec = 5;
    tv.tv_usec = 0;



    /* UDP node server connection */
	fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
	if(fd == -1) exit(1);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP socket

	if((errcode = getaddrinfo(regIP, regUDP, &hints, &res)) != 0){
		fprintf(stderr,"error: getaddrinfo: %s\n", gai_strerror (errcode));
		exit(1);
	}

	/* Register and Confirm */
	str[0] = '\0';
	buffer[0] = '\0';

	strcpy(str, "NODES ");
	strcat(str, net);

   	n = sendto(fd, str, strlen(str), 0, res->ai_addr, res->ai_addrlen);
	if(n == -1) exit(1);
			

	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);

	cnt = select(fd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
	if(cnt <= 0){
		printf("\tError establishing connection with node server!\n");
		freeaddrinfo(res);
		close(fd);
		return -1;
	}

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, strlen(buffer)-1, 0, &addr, &addrlen);
	if(n == -1) exit(1);
	buffer[n] = '\0';

	if(n == 11 + strlen(net)){
		bootIP = NULL;
		bootTCP = NULL;
		freeaddrinfo(res);
		close(fd);
		return 0;
	}

	token = strtok(buffer, "\n");
	i=0;
	while(token != NULL){
		strcpy(matrix[i], token);
		token = strtok(NULL, "\n");
		i++;
	}
	r = rand() % i; 
	if(r==0) r = 1;

	token = strtok(matrix[r], " ");
	i=0;
	while(token != NULL){
		if(i==0) strcpy(bootIP, token);
		if(i==1) strcpy(bootTCP, token);

		token = strtok(NULL, " ");
		i++;
	}

	/* Close UDP socket */
	freeaddrinfo(res);
	close(fd);
	return 0;
}