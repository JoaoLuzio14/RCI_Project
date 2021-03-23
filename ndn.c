/******************************************************************************
 *
 * File Name: ndn.c
 * Autor:  G19 (RCI 20/21) - João Luzio (IST193096) & José Reis (IST193105)
 * Last Review: 22 Mar 2021
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include "ndn.h"

#define DEFAULT_HOST "tejo.tecnico.ulisboa.pt"
#define DEFAULT_IP "139.136.138.142"
#define DEFAULT_PORT "59000"

int regNODE(int regFLAG, int fd, char* net, char* nodeIP, char* nodeTCP, struct sockaddr** ai_addr, socklen_t ai_addrlen);

int main(int argc, char **argv){

	/* Common Variables */
	int i, errcode, endFLAG = 0;
	char nodeIP[20], nodeTCP[20], regIP[20], regUDP[20];

	/* User Interface Variables */
	char user_str[64], cmd[64], net[64], nodeID[64], bootIP[64], bootTCP[64];
	int cmd_code, joined = 0;

	/* Node Topology Variables */
	node_info extern_node, backup_node;

	/* UDP Server Variables */
	struct addrinfo hints, *res;
	struct in_addr addr;
	int fd;
	ssize_t n;
	char buffer[128+1];

	/* TCP Server Variables */	

	/* Argument Process */	


	if(argc < 3){
		printf("Invalid number of arguments. Very few arguments inserted.\nTypical usage: 'ndn IP TCP regIP regUDP'.\n");
		exit(1);
	}
	else if(argc > 5){
		printf("Invalid number of arguments. Too many arguments inserted.\nTypical usage: 'ndn IP TCP regIP regUDP'.\n ");
		exit(1);
	}
	else if(argc == 3){
		printf("Valid number of arguments. Some arguments might have been ommited and some values will be set by deafault.\n");
		strcpy(regIP, DEFAULT_IP);
		strcpy(regUDP, DEFAULT_PORT);
	}
	else if(argc == 4){
		printf("Valid number of arguments. Some arguments might have been ommited and some values will be set by deafault.\n");
		strcpy(regIP, argv[3]);
		strcpy(regUDP, DEFAULT_PORT);
	}
	else if(argc == 5){
		printf("Valid number of arguments.\n");
		strcpy(regIP, argv[3]);
		strcpy(regUDP, argv[4]);
	}
	
	strcpy(nodeIP, argv[1]);
	strcpy(nodeTCP, argv[2]);
	if((check_ip(nodeIP) == 0) || (check_ip(regIP) == 0)){
		printf("Error in IP format verification.\n");
		exit(1);
	}
	if((atoi(regUDP) < 0) || (atoi(regUDP) > 65535)){
		printf("Error specifying UDP port.\n");
		exit(1);
	}
	printf("Arguments are valid.\n\n\n\n\n");


	/* UDP Node Server Connection Setup */
	fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
	if(fd == -1) exit(1);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP socket

	errcode = getaddrinfo(DEFAULT_HOST, regUDP, &hints, &res);
	if(errcode != 0) exit(1);
	
	/* User Interface */
	printf("Node Interface:\n");
	while(1){
		printf(">>> ");
		fflush(stdout);
		
		if(fgets(user_str, 64, stdin)!= NULL){
			errcode = sscanf(user_str, "%s", cmd);
			if(errcode != 1) continue;
			cmd_code = get_cmd(cmd); // Get command code
		}

   		switch(cmd_code){
   			case 1:
   				if(joined != 0){ // Already joined?
   					printf("\tNode already joined a net!\n");
   					break;
   				}

   				errcode = sscanf(user_str, "%s %s %s %s %s", cmd, net, nodeID, bootIP, bootTCP);
   				if((errcode != 3) && (errcode != 5)){
   					printf("\tInvalid command syntax. The ideal executable command is: 'join net id bootIP bootTCP'.\n");
   					break;
   				}
   				else{
   					joined = regNODE(1, fd, net, nodeIP, nodeTCP, &(res->ai_addr), res->ai_addrlen);
   					if(joined == 0) break;
   				}

   				if(errcode == 3){ // Indirect Join

   				}
   				else if(errcode == 5){ // Direct Join

   				}

   				break;

   			case 2:
   				if(joined != 1){ // Not joined yet?
   					printf("\tNode does not have joined any net!\n");
   					break;
   				}

   				joined = regNODE(0, fd, net, nodeIP, nodeTCP, &(res->ai_addr), res->ai_addrlen);
   				if(joined == 1) break;

   				memset(net, '0', sizeof(net));
   				break;

   			case 3:
   				printf("\tShutting down all connections and closing the node...\n");
   				if(joined == 1){
   					joined = regNODE(0, fd, net, nodeIP, nodeTCP, &(res->ai_addr), res->ai_addrlen);
   				}
   				endFLAG = 1;
   				if((endFLAG = 1) && (joined == 0)) printf("\tSucess! Node shut down.\n");
   				break;

   			default:
   				printf("\tInvalid or unknown command: %s\n", cmd);
   				break;
   		}
   		if(endFLAG == 1) break;
	}

	/* UDP Node Server Connection Close */
	freeaddrinfo(res);
	close(fd);
	exit(0);
}

int regNODE(int regFLAG, int fd, char* net, char* nodeIP, char* nodeTCP, struct sockaddr** ai_addr, socklen_t ai_addrlen){
	char str[100], auxstr[100];
	ssize_t n;
	struct sockaddr addr;
	socklen_t addrlen;
	char buffer[128+1];

	str[0] = '\0';
	auxstr[0] = '\0';

	if(regFLAG == 1) strcpy(str, "REG ");
	else if(regFLAG == 0) strcpy(str, "UNREG ");

   	msg_build(auxstr, net, nodeIP, nodeTCP);
   	strcat(str, auxstr);
   	// printf("%s\n", str);
   	n = sendto(fd, str, strlen(str), 0, *ai_addr, ai_addrlen);
	if(n == -1) exit(1);
					
	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, strlen(buffer)-1, 0, &addr, &addrlen);
	if(n == -1) exit(1);
	buffer[n] = '\0';
	// printf("%s\n", buffer);

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