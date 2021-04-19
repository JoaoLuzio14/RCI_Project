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
	enum {unreg, reg, busy, getout} state;
	int maxfd, cntr;

	/* User Interface Variables */
	char user_str[64], cmd[64], net[64], nodeID[64], bootIP[64], bootTCP[64];
	int fd, cmd_code, counter, joined = 0;
	char host[NI_MAXHOST], service[NI_MAXSERV]; // consts in <netdb.h>
	struct sockaddr addr_tcp;
	socklen_t addrlen_tcp;
	fd_set rfds;
	struct timeval tv;

	/* Node Topology Variables */
	contact extern_node, backup_node;

	/* Expedition Tables Variables */
	nodeinfo *head_table, *new_table, *aux_table;

	/* TCP Server Variables */	
	struct addrinfo hints, *res, *p;
	struct in_addr addr, *addraux;
	socklen_t addrlen;
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

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	printf("Arguments are valid.\n\n\n\n\n");

	/* TCP Server Connection */

	if((fd_server=socket(AF_INET,SOCK_STREAM,0)) == -1) exit(1); //TCP type of socket

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; //IPv4
	hints.ai_socktype = SOCK_STREAM; //TCP socket
	hints.ai_flags = AI_PASSIVE;

	if((errcode=getaddrinfo(NULL, nodeTCP, &hints, &res))!=0){
		fprintf(stderr,"TCP error: getaddrinfo: %s\n",gai_strerror(errcode));
	}
 
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

	printf(">>> "); // Command Line Prompt
	fflush(stdout);

	while(1){

		FD_ZERO(&ready_sockets);
		switch(state){
			case unreg: 
				FD_SET(0, &ready_sockets);
				maxfd = 0;
				break;
			case busy:
				FD_SET(fd, &ready_sockets);
				maxfd = fd;
				break;
			case reg:
				FD_SET(0, &ready_sockets);
				FD_SET(fd_server, &ready_sockets);
				maxfd = max(0, fd_server);
				aux_table = head_table;
				while(aux_table != NULL){
					if(aux_table->fd != 0){
						FD_SET(aux_table->fd, &ready_sockets);
						maxfd = max(aux_table->fd, maxfd);
					}
					aux_table = (nodeinfo *)aux_table->next;
				}
				break;
			case getout: 
				endFLAG = 1;
				break;
		}
		if(endFLAG == 1) break;

		if(state == busy){
			cntr = select(maxfd + 1, &ready_sockets, (fd_set *)NULL, (fd_set *)NULL, &tv);
		}
		else{
			cntr = select(maxfd + 1, &ready_sockets, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
		}
		if(cntr <= 0){
			printf("Error: Unexpected (select)!\n");
			exit(1);
		}

		/* Nodes Connected Messages Processing */	
		if(state == reg){
			aux_table = head_table;
			while(aux_table != NULL){
				if((aux_table->fd != 0) && (FD_ISSET(aux_table->fd, &ready_sockets))){
					FD_CLR(aux_table->fd, &ready_sockets);
					cntr--;

					bzero(buffer, sizeof(buffer));
					n = read(aux_table->fd, buffer, sizeof(buffer));
					if(n <= 0){
						printf("\tError receiving a message from a neighbour node!\n");
						break;
					}
					
					bzero(cmd, sizeof(cmd));
					if(sscanf(buffer, "%s", cmd) == 1){
						cmd_code = get_msg(cmd); // Get command code
					}
					else continue;

					printf("\t%d\n", cmd_code);
					switch(cmd_code){
						case 1:
							// Update Expedition Table
							new_table = (nodeinfo*)calloc(1, sizeof(nodeinfo));
							sscanf(buffer, "%s %s", user_str, new_table->id);
							new_table->fd = aux_table->fd;
							new_table->next = NULL;
							errcode = table_in(head_table, new_table);
							// Propagate the Message
							new_table = head_table;
							while(new_table != NULL){
								if((new_table->fd != 0) && (new_table->fd != aux_table->fd)){
									n = write(aux_table->fd, buffer, sizeof(buffer));
									if(n <= 0){
										printf("\tError sending message!\n");
										break;
									}
								}
								new_table = (nodeinfo*)new_table->next;
							}
							break;
						default: // Unknown Message Received
							break;
					}

				}
				aux_table = (nodeinfo *)aux_table->next;
			}		
		}
		/* Server and User Messages Processing */
		for(; cntr; --cntr){
			switch(state){
				case busy:
					if(FD_ISSET(fd, &ready_sockets)){
						FD_CLR(fd, &ready_sockets);

						bzero(buffer, sizeof(buffer));
						n = read(fd, buffer, sizeof(buffer));
						if(n <= 0){
							printf("\tError receiving a message from a neighbour node!\n");
							break;
						}
						
						bzero(cmd, sizeof(cmd));
						if((sscanf(buffer, "%s", cmd) == 1) && (strcmp(cmd, "ADVERTISE") == 0)){
							// Update Expedition Table
							new_table = (nodeinfo*)calloc(1, sizeof(nodeinfo));
							sscanf(buffer, "%s %s", user_str, new_table->id);
							new_table->fd = fd;
							new_table->next = NULL;
							errcode = table_in(head_table, new_table);
							// Propagate the Message
							new_table = head_table;
							while(new_table != NULL){
								if((new_table->fd != 0) && (new_table->fd != fd)){
									n = write(fd, buffer, sizeof(buffer));
									if(n <= 0){
										printf("\tError sending message!\n");
										break;
									}
								}
								new_table = (nodeinfo*)new_table->next;
							}
						}
						else break;
						state = reg;
					}
					break;
				case unreg:
					if(FD_ISSET(0, &ready_sockets)){
						FD_CLR(0, &ready_sockets);
						if(fgets(user_str, 64, stdin)!= NULL){
							bzero(cmd, sizeof(cmd));
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
						   					bootIP[0] = '\0';
						   					bootTCP[0] = '\0';
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
										strcpy(extern_node.node_tcp, bootTCP);

										if((extern_node.node_ip[0] != '\0') && (extern_node.node_tcp[0] != '\0')){ // Connect to external neighbour
											fd = tcp_connection(extern_node.node_ip, extern_node.node_tcp);
											// Send Self Info to External
											bzero(buffer, sizeof(buffer));
											sprintf(buffer, "NEW %s %s\n", nodeIP, nodeTCP);
											n = write(fd, buffer, sizeof(buffer));
											if(n <= 0){
												printf("\tError sending node info!\n");
												break;
											}

											FD_ZERO(&rfds);
											FD_SET(fd, &rfds);

											counter = select(fd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
											if(counter <= 0){
												printf("\tError establishing connection with external neighbour node!\n");
												close(fd);
												break;
											}
											bzero(buffer, sizeof(buffer));
											n = read(fd, buffer, sizeof(buffer));
											if(n <= 0){
												printf("\tError receiving node info!\n");
												break;
											}
											// Get External Neighbour Info
											if((sscanf(buffer, "%s %s %s", user_str, backup_node.node_ip, backup_node.node_tcp) != 3) || (strcmp(user_str, "EXTERN") != 0) || (n <= 0)){
												printf("\t%s\n", buffer);
												printf("\tError getting external neighbour node information.\n");
												close(fd);
												break;
											}
											// Self Advertise
											bzero(buffer, sizeof(buffer));
											sprintf(buffer, "ADVERTISE %s\n", nodeID);
											n = write(fd, buffer, sizeof(buffer));
											if(n <= 0){
												printf("\tError sending node info!\n");
												break;
											}

										}

										// Start Expedition Table
										head_table = (nodeinfo*)calloc(1, sizeof(nodeinfo));
										strcpy(head_table->id, nodeID);
										head_table->fd = 0;
										head_table->next = NULL;

										// Register Node in Node Server
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
							if(state != getout){
					   			printf(">>> "); // Command Line Prompt
								fflush(stdout);			
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
					   				// shut down all connection with other nodes here
					   				joined = regNODE(0, net, nodeIP, nodeTCP, regIP, regUDP);
					   				if(joined == 1) break;
					   				else if(joined == 0) state = unreg;

					   				errcode = table_free(head_table); // free expedition table

					   				memset(net, '\0', sizeof(net));
					   				break;

					   			case 3: // exit
					   				printf("\tShutting down all connections and closing the node...\n");
					   				// shut down all connection with other nodes here
					   				if(joined == 1){
					   					joined = regNODE(0, net, nodeIP, nodeTCP, regIP, regUDP);
					   				}
					   				state = getout;

					   				errcode = table_free(head_table); // free expedition table

					   				if((state == getout) && (joined == 0)) printf("\tSucess! Node shut down.\n");
					   				break;

					   			case 4: // show topology
					   				if(extern_node.node_ip[0] == '\0'){
					   					printf("\tThe node is alone in the net.\n");
					   					break;
					   				}
					   				else{
						   				printf("\tEXTERNAL NEIGHBOUR: %s %s\n", extern_node.node_ip, extern_node.node_tcp);
						   				printf("\tBACKUP NEIGHBOUR: %s %s\n", backup_node.node_ip, backup_node.node_tcp);
					   				}
					   				break;

					   			case 5: // show routing
					   				printf("\tEXPEDITION TABLE:\n");
					   				aux_table = head_table;
									while(aux_table != NULL){
										printf("\tID:%s\tfd:%d\n", aux_table->id, aux_table->fd);
										aux_table = (nodeinfo *)aux_table->next;
									}		
					   				break;

					   			default:
					   				printf("\tNode already joined a net!\n");
					   				break;
					   		}	
					   		if(state != getout){
					   			printf(">>> "); // Command Line Prompt
								fflush(stdout);			
					   		}	
						}
					}
					else if(FD_ISSET(fd_server, &ready_sockets)){
						FD_CLR(fd_server, &ready_sockets);

						addrlen_tcp = sizeof(addr_tcp);
						if((fd = accept(fd_server, &addr_tcp, &addrlen_tcp)) == -1) exit(1);

						FD_ZERO(&rfds);
						FD_SET(fd, &rfds);

						counter = select(fd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
						if(counter <= 0){
							printf("\tError establishing connection with internal neighbour node!\n");
							close(fd);
							break;
						}

						bzero(buffer, sizeof(buffer));
						n = read(fd, buffer, sizeof(buffer));
						if(n <= 0){
							printf("\tError receiving node info!\n");
							break;
						}

						bootIP[0] = '\0';
						bootTCP[0] = '\0';
						if((sscanf(buffer, "%s %s %s", user_str, bootIP, bootTCP) != 3) || (strcmp(user_str, "NEW") != 0) || (n <= 0)){
							printf("\t%s\n", buffer);
							printf("\tError getting external neighbour node information.\n");
							close(fd);
							break;
						}
						else if(extern_node.node_ip[0] == '\0'){
							strcpy(extern_node.node_ip, bootIP);
							strcpy(extern_node.node_tcp, bootTCP);
							strcpy(backup_node.node_ip, nodeIP);
							strcpy(backup_node.node_tcp, nodeTCP);
						}

						bzero(buffer, sizeof(buffer));
						sprintf(buffer, "EXTERN %s %s\n", extern_node.node_ip, extern_node.node_tcp);
						n = write(fd, buffer, sizeof(buffer));
						if(n <= 0){
							printf("\tError sending node info!\n");
							break;
						}
						// send new connection my expedition table
						aux_table = head_table;
						while(aux_table != NULL){
							if(aux_table->fd != 0){
								bzero(buffer, sizeof(buffer));
								sprintf(buffer, "ADVERTISE %s\n", aux_table->id);
								n = write(fd, buffer, sizeof(buffer));
								if(n <= 0){
									printf("\tError sending expedition table!\n");
									break;
								}
							}
							aux_table = (nodeinfo*)aux_table->next;
						}
						state = busy;
					}
					break;

				case getout: break;

				default:
					printf("\tInvalid state: %d\n", state);
   					break;
			}
		}
	}
	close(fd); // remove later!

	freeaddrinfo(res);
	close(fd_server);
	exit(0);
}