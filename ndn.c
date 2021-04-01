/******************************************************************************
 *
 * File Name: ndn.c
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

#define max(A,B) ((A)>=(B)?(A):(B))
#define DEFAULT_HOST "tejo.tecnico.ulisboa.pt"
#define DEFAULT_IP "193.136.138.142"
#define DEFAULT_PORT "59000"

int main(int argc, char **argv){

	/* Common Variables */
	int i, errcode, endFLAG = 0;
	char nodeIP[20], nodeTCP[20], regIP[20], regUDP[20];
	fd_set ready_sockets;
	enum {unreg, reg, getout} state;
	int maxfd, cntr;

	/* User Interface Variables */
	char user_str[64], cmd[64], net[64], nodeID[64], bootIP[64], bootTCP[64];
	int fd, cmd_code, joined = 0;


	/* Node Topology Variables */
	contact extern_node, backup_node;

	/* TCP Server Variables */	
	struct addrinfo hints, *res;
	struct in_addr addr;
	ssize_t n;
	char buffer[128+1];
	int fd_server;

	/* Argument Process */	
	printf("\n");
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

	/* TCP Server Connection */

	if((fd_server=socket(AF_INET,SOCK_STREAM,0))==-1)exit(1); //TCP type of socket

	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET;		//IPv4
	hints.ai_socktype=SOCK_STREAM;  //TCP socket
	hints.ai_flags=AI_PASSIVE;

	if((errcode=getaddrinfo(NULL, nodeTCP, &hints, &res))!=0) 
		fprintf(stderr,"TCP error: getaddrinfo: %s\n",gai_strerror(errcode));
 
	if (bind(fd_server, res->ai_addr, res->ai_addrlen)==1){
		printf("Error registering server address with the system (bind)");
		exit(1);
	}
	if (listen(fd_server, 10) == -1){
		printf("Error in instructing the kernal to accept incoming connections (listen)");
	}

	/* User Interface */

	state = unreg; // Inicial State Defined
	printf("Node Interface:\n");

	while(1){

		printf(">>> "); // Command Line Prompt
		fflush(stdout);

		FD_ZERO(&ready_sockets);
		switch(state){
			case unreg: 
				FD_SET(0, &ready_sockets);
				maxfd = 0;
				break;
			case reg:
				FD_SET(0, &ready_sockets);
				FD_SET(fd_server, &ready_sockets);
				maxfd = max(0, fd_server);
				break;
			case getout: 
				endFLAG = 1;
				break;
		}
		if(endFLAG == 1) break;

		cntr = select(maxfd + 1, &ready_sockets, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
		if(cntr <= 0){
			printf("Error: Unexpected (select)!\n");
			exit(1);
		}

		for(; cntr; --cntr){
			switch(state){
				case unreg:
					if(FD_ISSET(0, &ready_sockets)){
						FD_CLR(0, &ready_sockets);
						if(fgets(user_str, 64, stdin)!= NULL){

							if(sscanf(user_str, "%s", cmd) == 1){
								cmd_code = get_cmd(cmd); // Get command code
							}
							else break;

							switch(cmd_code){
								case 0: // unknown
									printf("\tInvalid or unknown command: %s\n", cmd);
					   				break;

					   			case 1: // join
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
						   				if(errcode == 3){ // Indirect Join
						   					if(getEXT(net, regIP, regUDP, bootIP, bootTCP) == -1) break;
						   				}
						   				if(errcode == 5){ // Direct Join
						   					if(check_ip(bootIP) == 0){
												printf("Error in IP format verification.\n");
												break;
											}
											if((atoi(bootTCP) < 0) || (atoi(bootTCP) > 65535)){
												printf("Error specifying TCP port.\n");
												break;
											}
						   				}
										strcpy(extern_node.node_ip, bootIP);
										strcpy(extern_node.node_TCP, bootTCP);

										if((extern_node.node_ip != NULL) && (extern_node.node_TCP != NULL)){ // Connect to external neighbour
											// connect to external neighbour
										}

					   					joined = regNODE(1, net, nodeIP, nodeTCP, regIP, regUDP);
					   					if(joined == 0) break;
					   					else if(joined == 1) state = reg;
					   				}

					   				break;

					   			case 3: // exit
					   				state = getout;
					   				if((state == getout) && (joined == 0)) printf("\tSucess! Node shut down.\n");
					   				break;

					   			default:
					   				printf("\tNode does not have joined any net yet!\n");
					   				break;
					   		}				
						}
					}
					break;

				case reg:
					if(FD_ISSET(0, &ready_sockets)){
						
						FD_CLR(0, &ready_sockets);
						if(fgets(user_str, 64, stdin)!= NULL){

							if(sscanf(user_str, "%s", cmd) == 1){
								cmd_code = get_cmd(cmd); // Get command code
							}
							else break;

							switch(cmd_code){
								case 0: // unknown
									printf("\tInvalid or unknown command: %s\n", cmd);
					   				break;

					   			case 2: // leave
					   				if(joined != 1){ // Not joined yet?
					   					printf("\tNode does not have joined any net!\n");
					   					break;
					   				}

					   				joined = regNODE(0, net, nodeIP, nodeTCP, regIP, regUDP);
					   				if(joined == 1) break;
					   				else if(joined == 0) state = unreg;

					   				memset(net, '\0', sizeof(net));
					   				break;

					   			case 3: // exit
					   				printf("\tShutting down all connections and closing the node...\n");
					   				if(joined == 1){
					   					joined = regNODE(0, net, nodeIP, nodeTCP, regIP, regUDP);
					   				}
					   				state = getout;
					   				if((state == getout) && (joined == 0)) printf("\tSucess! Node shut down.\n");
					   				break;

					   			case 4: // show topology
					   				if(extern_node.node_ip == NULL){
					   					printf("\tThe node is alone\n");
					   				}
					   				printf("\tEXTERNAL NEIGHBOUR: %s %s\n", extern_node.node_ip, extern_node.node_TCP);
					   				printf("\tBACKUP NEIGHBOUR: %s %s\n", backup_node.node_ip, backup_node.node_TCP);
					   				break;

					   			default:
					   				printf("\tNode already joined a net!\n");
					   				break;
					   		}				
						}
					}
					else if(FD_ISSET(fd_server, &ready_sockets)){
						FD_CLR(fd_server, &ready_sockets);
						// check new connection to the server
					}
					break;

				case getout: break;

				default:
					printf("\tInvalid state: %d\n", state);
   					break;
			}
		}
	}
	freeaddrinfo(res);
	close(fd_server);
	exit(0);
}