/**
 * @pnandi_assignment3
 * @author  Priyankar Nandi <pnandi@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <getopt.h>

#include "../include/global.h"
#include "../include/logger.h"


//[PA3] Update Packet Start
/**
 * host details
 */
struct host_details{
	uint16_t num_fields; // Number of neighbours
	uint16_t server_port;// Port on which the host is running
	uint32_t server_ip;// Ip of the host
};

/**
 * structure that defines the details of
 * a server.
 */
struct server_details{
    uint32_t ip;//ip of the server
	uint16_t port;//port on which the server is running
	uint16_t padding;//padding
	uint16_t id;//id of the sever
	uint16_t cost;//cost of link of the host to the server
};

/**
 * structure of a UDP message
 */
struct udp_packet{
    struct host_details host_det;//static part of the that contains the host details
	struct server_details *serv_details;//Pointer to an array of structures which is dynamic
};
//[PA3] Update Packet End

//[PA3] Routing Table Start
struct routing_table_record{
	struct server_details serv_details; //members declared and explained above
	int next_hop_id; // next hop id
    int is_neighbour; //whether the server is a neighbour
    char ip_s[30]; // ip of the server in dotted decimal notation
    int fail_update_counter; // counter for each of the neighbours.Keeps track of the numberof failed updates by a neighbour.
    int is_server_started;//initialized to 0. Started to 1 when the 1st packet is received from the neighbour is received
    int is_neighbour_inactive;//1 when neighbour is started and was marked inactive due to fail updates
};

//pointer to an array of routing table records
struct routing_table_record* routing_table;

//[PA3] Routing Table End

struct neighbour_details_record{
  uint16_t server_id;
  uint16_t cost;
};

struct neighbour_details_record *neighbour_details;


int **dv; //pointer to distance vector matrix

/**
 * Print academic integrity
 */
void display_academic_integrity();

/**
 * parses the topology file to extract the details
 */
void parse_topology_file(char *filepath);

/**
 * set the host details
 */
void set_host_details(int server_id);
/**
 *  Updates the routing table with the cost of the path to
 *  the neighbour
 */
void update_routing_table(int neighbour_id,int neighbour_cost);

/**
 * Details of the host
 */
uint32_t host_ip;
char host_ip_s[128];
uint16_t host_port;
uint16_t host_id_g;

int num_servers;//number of servers
int num_neighbours;//number of neighbours

fd_set master_fds;//master file descriptor list
fd_set temp_fds;//temp file descriptor list
int fdmax = 0;
int nbytes;
char buf[MAX_BUF_LEN];
char cmdargv[10][100];//array to store the input

/**
 * Global var to hold the number of distance vector packets
 */
int pkt_cnt_g;

/**
 * sets up the server socket. Reference has been taken from beej guide
 */
int set_server_socket();
/**
 * Send UDP packet to neighbour
 */
int send_udp_packet(char *ip,int port,struct udp_packet packet,int size) ;
/**
 * sends distance vectors to neighbours from the routing table
 */
void send_dv_to_neighbours();
/**
 * Update routing table based on the distance vector sent by neighbour
 */
void update_dv_matrix_from_rcvd_packet(struct udp_packet packet);
/**
 * displays the routing table
 */
void display_routing_table();
/**
 * Finds the sender details based on ip
 */
struct server_details find_server_details(uint32_t ip,uint16_t port);

/**
 * Checks for inactive node and also increaments the fail_update_counter by1
 */
void check_for_inactive_nodes();
/**
 * Checks if a given string contains only numbers
 */
int validate_integer(char *s);

/**
 * Updates the cost between servers
 */
int update_cost();
/**
 * Display the count of packets received by the host
 */
void display_packets_count();
/**
 * Disable a link to the given server
 */
int disable_server();
/**
 * Crash the server
 */
void crash_server();

/**
 * Dump a update packet
 */
void dump_packet();

/**
 * initialize the distance vector matrix
 * Reference - http://stackoverflow.com/questions/11463455/c-programming-initialize-2d-array-dynamically
 */
void init_dv_matrix();
/**
 * displays the distance vector matrix
 */
void display_dv_matrix();

/**
 * Updates the routing table based on the distance vector matrix
 */
void update_routing_table_from_dv_matrix(int server_id,int server_cost,int next_hop_id);
/**
 * runs the Bellman-Ford algorithm on the distance vector matrix
 * Reference - slide 77 of chapter 4
 */
void run_bellman_ford();
/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 * Reference - sr.c template of PA2
 */
int isNumber(char *input);

/**
 * Checks if the server_id is a neighbour
 */
int validateNeighbour(int server_id);
/**
 * Decreases the counter by 1 after each received message from the neighbour
 */
void decrease_counter(int neighbour_id);

/**
 * Sets the initial cost that is read from the topology file for this neighbour
 */
void set_initial_neighbour_cost(int neighbour_id);
/**
 * Sets the initial cost that is read from the topology file for this neighbour
 */
void set_initial_neighbour_cost(int neighbour_id);
/**
 * Finds the cost of the neighbour
 */
u_int16_t find_neighbour_cost(int neighbour_id);
/**
 * Checks if the neighbour was marked inactive
 */
int check_if_neigh_was_inactive(int neighbour_id);
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log();

	/**Validation of input arguments
	 * Reference - sr.c template of PA2
	 */
	//Check for number of arguments
	if(argc != 5){
		    printf("\nMissing arguments");
		    printf("\nUsage: %s -t <path-to-topology-file> -i <routing-update-interval>\n", argv[0]);
		   	exit(1);
	}
   /*
    * Parse the arguments
    * http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    */
   int opt;
   int interval;
   while((opt = getopt(argc, argv,"t:i:")) != -1){
       switch (opt){
           case 't':   //Do nothing.File will be validated later
                       break;

           case 'i':   if(!isNumber(optarg)){
        	            printf("Invalid value for -i\n");
                           exit(1);
                       }
                       interval = atoi(optarg);
                       break;

           case '?':   break;

           default:    cse4589_print_and_log("\nUsage: %s -t <path-to-topology-file> -i <routing-update-interval>\n", argv[0]);
                       exit(1);
       }
   }

	//set the count of packets received to 0
	pkt_cnt_g = 0;
	//variable declarations
	struct sockaddr_in neighbour_addr;//address of the neighbour
	socklen_t addr_len;//length of the address of the neighbour

	char *filepath = argv[2];
    //printf("\nFilepath %s and interval %s",filepath,interval_s);
	parse_topology_file(filepath);
	//handle the timeout value specified from the command line

	struct timeval timeout;
	timeout.tv_sec = interval;

	int server_sd = set_server_socket();
	if(server_sd==-1){
		printf("\nERROR: Cannot start server.\n");
		exit(1);
	}

	//clear the file descriptor  list
	FD_ZERO(&master_fds);
	FD_ZERO(&temp_fds);
	//set the standard input in the master list
	FD_SET(STDIN,&master_fds);
	FD_SET(server_sd,&master_fds);
	fdmax = server_sd;

	/*printf(USER_PROMPT);
	fflush(stdout);*/
	//send_dv_to_neighbours();
	/*printf("\nNeighbour cost starts");
	find_neighbour_cost(1);
	printf("\nNeighbour cost ends");*/

    while(1){
		temp_fds = master_fds; //copy the master list
	    int ret_select = select(fdmax + 1, &temp_fds, NULL,NULL, &timeout);
	    if (ret_select < 0){
	    	 printf("\nERROR: Select operation failed");
	    }else if(ret_select==0){
	    	 //printf("\nSelect Timeout expired");
	    	 //fflush(stdout);
	    	 timeout.tv_sec = interval;
	    	 check_for_inactive_nodes();
	    	 send_dv_to_neighbours();
	    }
	    else{

	    	//Loop through the set of socket descriptors to check which one is ready
			int sock_index;
			for (sock_index = 0; sock_index <= fdmax; sock_index++) {
				if (FD_ISSET(sock_index, &temp_fds)) {
					if (sock_index == STDIN){
						char *line = NULL;
						size_t len = 0;
						ssize_t read;
						if ((read = getline(&line, &len, stdin)) != -1) {
								//memset(&line,'\0',len);
					            char *cmdarg;
								int cmdargc = 0;
								int i;
								for (i = 0; i < read; i++) {
									if (line[i] == '\n')
										line[i] = '\0';
								}
								cmdarg = strtok(line, " ");
								while (cmdarg) {
									strcpy(cmdargv[cmdargc], cmdarg);
									cmdargc++;
							        cmdarg = strtok(NULL, " ");
								}
								if (!(strcasecmp(line, "update"))) {
									int ret_update_cost=update_cost();
									//printf("\nReturned value %d",ret_update_cost);
									if(ret_update_cost==-1){
										cse4589_print_and_log("%s:%s\n",cmdargv[0],"Sever id can be only integers");
										fflush(stdout);
									}
									else if(ret_update_cost==-2){
										cse4589_print_and_log("%s:%s\n",cmdargv[0],"Invalid host id");
										fflush(stdout);
									}
									else if(ret_update_cost==-3){
										cse4589_print_and_log("%s:%s\n",cmdargv[0],"Invalid neighbour id");
										fflush(stdout);
								    }
									else if(ret_update_cost==-4){
										cse4589_print_and_log("%s:%s\n",cmdargv[0],"Invalid Cost");
										fflush(stdout);
								    }
									else if(ret_update_cost==0){
										cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
										fflush(stdout);
									}

								}
								else if (!(strcasecmp(line, "step"))) {
									    cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
									    fflush(stdout);
									    send_dv_to_neighbours();
								}
								else if (!(strcasecmp(line, "packets"))) {
									cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
									fflush(stdout);
									display_packets_count();
								}
								else if (!(strcasecmp(line, "disable"))) {
									int ret_disable=disable_server();
									if(ret_disable==-1){
										cse4589_print_and_log("%s:%s\n",cmdargv[0],"Server not a neighbour");
										fflush(stdout);
									}
									else{
										cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
										fflush(stdout);
									}
								}
								else if (!(strcasecmp(line, "display"))) {
									cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
									fflush(stdout);
									display_routing_table();
								}
								else if (!(strcasecmp(line, "crash"))) {
									//cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
									//fflush(stdout);
									crash_server();
							    }
								else if (!(strcasecmp(line, "dump"))) {
									cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
									fflush(stdout);
									dump_packet();
							    }
								else if (!(strcasecmp(line, "academic_integrity"))) {
									cse4589_print_and_log("%s:SUCCESS\n",cmdargv[0]);
									fflush(stdout);
									display_academic_integrity();
								}
								else if (!(strcasecmp(line, "dv"))) {
									display_dv_matrix();
							    }
								else{
									cse4589_print_and_log("%s:%s\n",cmdargv[0],"Invalid command");
									fflush(stdout);
								}
						}
						/*printf(USER_PROMPT);
						fflush(stdout);*/
					}
					//There is message from some neighbour
					if(sock_index == server_sd){
						int size = sizeof(struct host_details)+((num_servers)*sizeof(struct server_details));
						void *rcvbuf = malloc (size);

						//printf("\nNo. of bytes received 2 %d",nbytes);

						if ((nbytes = recvfrom(sock_index, rcvbuf, size , 0,(struct sockaddr *)&neighbour_addr,&addr_len))==-1) {
							perror("recvfrom");
							//exit(1);
						}
						else{
							char s[INET_ADDRSTRLEN];
							/*printf("\nReceived %d bytes from %s",nbytes,inet_ntop(neighbour_addr.sin_family,&neighbour_addr.sin_addr,
									s, sizeof s));*/
							//pkt_cnt_g++;

							//unpack data
							struct host_details host_det;
							memcpy(&host_det.num_fields,rcvbuf,2);
							memcpy(&host_det.server_port,rcvbuf+2,2);
							memcpy(&host_det.server_ip,rcvbuf+4,4);

							//printf("\nReceived message num_fields %d",host_det.num_fields);
							struct server_details *serv_det = malloc((num_servers)*(sizeof(struct server_details)));
							int index;
							for(index=0;index<num_servers;index++){
										memcpy(&serv_det[index],rcvbuf+sizeof(struct host_details)+index*sizeof(struct server_details),sizeof(struct server_details));
										//printf("\nRECEIVED MSG:: IP:%d Port:%d Cost:%d",serv_det[index].ip,serv_det[index].port,serv_det[index].cost);
							}
							fflush(stdout);
							struct udp_packet packet;
							packet.host_det = host_det;
							packet.serv_details = serv_det;
							update_dv_matrix_from_rcvd_packet(packet);
						}
					}
				}
			}

	    }
	}

	int i;
	//printf("\nInside main");
	/*for(i=0;i<num_servers;i++){
        struct in_addr ip_addr;
        ip_addr.s_addr = routing_table[i].serv_details.ip;
 	    printf("\nServer id - %d Server ip - %s Server port - %d Cost %d Is_Neighbour%d\n",routing_table[i].serv_details.id,
 			   inet_ntoa(ip_addr),routing_table[i].serv_details.port,routing_table[i].serv_details.cost,routing_table[i].is_neighbour);
	}*/

   //display_routing_table();

	/*Clear LOGFILE and DUMPFILE*/
	fclose(fopen(LOGFILE, "w"));
	fclose(fopen(DUMPFILE, "wb"));

	/*Start Here*/

	return 0;
}



/**
 * sets up the server socket. Reference has been taken from beej guide
 */
int set_server_socket() {
	int yes = 1;
	int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket < 0) {
		perror("\nError while creating server socket");
		return -1;
	}
	struct sockaddr_in server_addr;
	//clear the bits
	memset(&server_addr, 0, sizeof(server_addr));
	//set the server address parameters
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(host_port);
	server_addr.sin_addr.s_addr = inet_addr(host_ip_s);
	//binds to the port
	if (bind(server_socket, (struct sockaddr *) &server_addr,
			sizeof(server_addr)) < 0) {
		perror("bind");
		return -1;
	}
	return server_socket;
}

/**
 * parses the topology file to extract the details
 * Reference - http://man7.org/linux/man-pages/man3/getline.3.html
 */
void parse_topology_file(char *filepath){
   FILE *file;
   char *mode = "r";
   char *line = NULL;
   size_t len = 0;
   ssize_t read;
   file=fopen(filepath,mode);
   if(file==NULL){
	   printf("\nERROR:: Invalid location of topology file\n");
	   fflush(stdout);
	   exit(1);
   }

   //number of servers
   if ((read = getline(&line, &len, file)) != -1) {
	   char num_servers_s[5];
	   strcpy(num_servers_s,line);
	   num_servers=atoi(num_servers_s);
   }

   //initialize the distance vector matrix based on the number of servers
   init_dv_matrix();

   routing_table = (struct routing_table_record *)malloc((num_servers+1)*sizeof(struct routing_table_record));

   //number of neighbours
    if ((read = getline(&line, &len, file)) != -1) {
 	   char num_neighbours_s[5];
 	   strcpy(num_neighbours_s,line);
 	   num_neighbours=atoi(num_neighbours_s);
    }

   int i;
   int routing_table_index;
   //read the entries for the servers
   for(i=1;i<=num_servers;i++){
	   read = getline(&line, &len, file);
	   int index;
	   char *arg;
	   arg = strtok(line," ");
	   for(index=1;index<=3;index++){
		   //server id
		    if(index==1){
                char *server_id_s = arg;
                int server_id=atoi(server_id_s);
                //store the data in the same index as of its server
                routing_table_index = server_id;
                routing_table[routing_table_index].serv_details.id = server_id;
         	   //set the default cost to infinity
                routing_table[routing_table_index].serv_details.cost = INFINITY;
         	   //set the default next hop to -1
                routing_table[routing_table_index].next_hop_id = -1;
                //initialize the fail update counter to 0
                routing_table[routing_table_index].fail_update_counter = 0;

                routing_table[routing_table_index].is_server_started = 0;
		    }
		    //server ip
		    else if(index==2){
		    	strcpy(routing_table[routing_table_index].ip_s,arg);
		    	struct sockaddr_in server_ip;
		    	if(inet_aton(arg, &server_ip.sin_addr)!=0){
		    		routing_table[routing_table_index].serv_details.ip = server_ip.sin_addr.s_addr;
		    	}
		    }
		    //server port
		    else if(index==3){
		    	char *server_port_s = arg;
		    	int server_port=atoi(server_port_s);
		    	routing_table[routing_table_index].serv_details.port = server_port;
		    }
		    arg = strtok(NULL," ");
	   }
   }

   //read the path cost entries for the neighbours
    int server_index,neigh_index;
    neighbour_details = (struct neighbour_details_record *)malloc((num_neighbours+1)*sizeof(struct neighbour_details_record));
    for(server_index=1;server_index<=num_neighbours;server_index++){
	   int neighbour_id;
	   int neighbour_cost;
 	   read = getline(&line, &len, file);
 	   char *arg;
 	   arg = strtok(line," ");
 	   for(neigh_index=1;neigh_index<=3;neigh_index++){
 		    //id of the host
 		    if(server_index==1 && neigh_index==1){
                 char *host_id_s = arg;
                 int host_id_g=atoi(host_id_s);
                 set_host_details(host_id_g);
 		    }
 		    else if(neigh_index==2){
 		    	neighbour_id = atoi(arg);
 		    	neighbour_details[server_index].server_id = neighbour_id;
 		    }
 		    else if(neigh_index==3){
                neighbour_cost = atoi(arg);
                neighbour_details[server_index].cost = neighbour_cost;
                update_routing_table(neighbour_id,neighbour_cost);
 		    }
 		    arg = strtok(NULL," ");
 	   }
	    //update the dv matrix
       dv[host_id_g][neighbour_id]=neighbour_cost;
    }
}

/**
 * set the host details
 */
void set_host_details(int server_id){
	struct routing_table_record *ptr = routing_table;
	int i;
	for(i=1;i<=num_servers;i++){
       if(routing_table[i].serv_details.id == server_id){
    	   host_ip=routing_table[i].serv_details.ip;
    	   host_port=routing_table[i].serv_details.port;
    	   host_id_g=routing_table[i].serv_details.id;

    	   struct in_addr ip_addr;
    	   ip_addr.s_addr = routing_table[i].serv_details.ip;
    	   strcpy(host_ip_s,inet_ntoa(ip_addr));
    	   //set the cost to 0
           routing_table[i].serv_details.cost = 0;

    	   //set the next hop to the host itself
    	   routing_table[i].next_hop_id = host_id_g;

           break;
       }
	}
}

/**
 *  Updates the routing table with the cost of the path to
 *  the neighbour
 */
void update_routing_table(int neighbour_id,int neighbour_cost){
	struct routing_table_record *ptr = routing_table;
	int i;
	for(i=1;i<=num_servers;i++){
	   if(routing_table[i].serv_details.id == neighbour_id){
		   routing_table[i].is_neighbour = 1;
		   routing_table[i].serv_details.cost = neighbour_cost;
		   routing_table[i].next_hop_id = neighbour_id;
		   break;
	   }
    }
}

/**
 * Send UDP packet to neighbour
 */
int send_udp_packet(char *ip,int port,struct udp_packet packet,int size) {

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	//create the server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);
	//display_routing_table();
	void *sndbuf=malloc(size);
	//printf("\nSize %d",size);
	//testing
	memcpy(sndbuf,&packet.host_det,sizeof(struct host_details));
		int index;
		for(index=0;index<num_servers;index++){
			memcpy(sndbuf+sizeof(struct host_details)+index*sizeof(struct server_details),&packet.serv_details[index],sizeof(struct server_details));
		}

		//testing
		int numbytes;
		if ((numbytes = sendto(sock, sndbuf, size, 0,(struct sockaddr *)&server_addr, sizeof server_addr)) == -1) {
			perror("talker: sendto");
			exit(1);
		}
   // printf("\nMessage sent------------");
	//unpack data
	struct host_details host_det;
	/*memcpy(&host_det.num_fields,sndbuf,2);
	memcpy(&host_det.server_port,sndbuf+2,2);
	memcpy(&host_det.server_ip,sndbuf+4,4);
	display_routing_table();
    struct server_details *serv_det = malloc((num_servers)*(sizeof(struct server_details)));
    //printf("\nNo. of fields %d",host_det.num_fields);
    display_routing_table();
    printf("\n==============================================");
	for(index=0;index<num_servers;index++){
			memcpy(&serv_det[index],sndbuf+sizeof(struct host_details)+index*sizeof(struct server_details),sizeof(struct server_details));
			printf("\nCost sent for %d is %d. Port is %d",index,serv_det[index].cost,serv_det[index].port);
	}*/
	//printf("\n==============================================");
	//display_routing_table();
	//printf("\nSent message num_fields %d",host_det.num_fields);
        printf("\nSent %d bytes to %s\n",numbytes,ip);
	fflush(stdout);
	return sock;
}

/**
 * Sends the distance vectors to neighbours
 */
void send_dv_to_neighbours(){
	int i;
	//1. make UDP packet

	struct host_details host_det;
	host_det.num_fields = htons(num_neighbours);
	host_det.server_port =  htons(host_port);
	host_det.server_ip = host_ip;

	struct udp_packet packet;
	packet.host_det = host_det;
    packet.serv_details=(struct server_details *)malloc(num_servers*sizeof(struct server_details));

    int neighbour_index=0;
	for(i=1;i<=num_servers;i++){
		//if(routing_table[i].serv_details.id != host_id_g){
					        //printf("\nNeighbour Index %d",neighbour_index);
							packet.serv_details[neighbour_index].ip = routing_table[i].serv_details.ip;
							packet.serv_details[neighbour_index].port = htons(routing_table[i].serv_details.port);
							packet.serv_details[neighbour_index].padding = htons(0);
							packet.serv_details[neighbour_index].id = htons(routing_table[i].serv_details.id);
							packet.serv_details[neighbour_index].cost = htons(routing_table[i].serv_details.cost);
						       //printf("\nCost sent actual %d",routing_table[i].serv_details.cost);
						    /*printf("\nCost sent Big Endian %d",packet.serv_details[neighbour_index].cost);*/
							neighbour_index++;
		//}
	}
	int packet_size = sizeof(host_det)+(num_servers)*sizeof(struct server_details);
	//printf("\nHost det size:: %lu",sizeof(host_det));
	//printf("\nServer details size:: %lu",sizeof(struct server_details));
	//2. send UDP packet
	for(i=1;i<=num_servers;i++){
			  if(routing_table[i].is_neighbour == 1){
				  send_udp_packet(routing_table[i].ip_s,routing_table[i].serv_details.port,packet,packet_size);
			  }
	}
}

/**
 * Update dv matrix from received packet
 */
void update_dv_matrix_from_rcvd_packet(struct udp_packet packet){


	struct server_details sender_det = find_server_details(packet.host_det.server_ip,ntohs(packet.host_det.server_port));
	int neighbour_id = sender_det.id;
	//if the sending server is a neighbour accept the messages else reject
	if(validateNeighbour(neighbour_id)){
		pkt_cnt_g++;
		cse4589_print_and_log("RECEIVED A MESSAGE FROM SERVER %d\n", neighbour_id);
                fflush(stdout);
		decrease_counter(neighbour_id);
		int i;
		//printf("\nBefore::::::::::");
			//display_dv_matrix();
		for(i=0;i<num_servers;i++){
				int server_id = ntohs(packet.serv_details[i].id);
				int cost = ntohs(packet.serv_details[i].cost);
	/*		    printf("\nCost received actual %d",cost);
				printf("\nCost received Big Endian %d",packet.serv_details[i].cost);*/
				dv[neighbour_id][server_id] = cost;
				//if the server has been started previously,was marked inactive due to fail updates
				//becomes active again
				// set the cost from the neighbour matrix here
				if(server_id==host_id_g && check_if_neigh_was_inactive(neighbour_id)){
				   set_initial_neighbour_cost(neighbour_id);
				}
				cse4589_print_and_log("%-15d%-15d\n", server_id,cost);
                                fflush(stdout);
		}
		//printf("\nAfter::::::::::");
		//display_dv_matrix();
	/*	printf("\nBefore::::::::::routing_table");
		display_routing_table();*/
		run_bellman_ford();
		/*printf(USER_PROMPT);
		fflush(stdout);*/
		/*printf("\nAfter::::::::::routing_table");
		display_routing_table();*/
    }
}


/**
 * Print academic integrity
 */
void display_academic_integrity(){
	cse4589_print_and_log("I have read and understood the course academic integrity policy "
            "located at "
            "http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.ht"
             "ml#integrity");
}

/**
 * displays the routing table
 */
void display_routing_table(){
	//printf("\nDisplaying routing table");
	int i;
	for(i=1;i<=num_servers;i++){
		cse4589_print_and_log("%-15d%-15d%-15d\n",routing_table[i].serv_details.id,routing_table[i].next_hop_id,routing_table[i].serv_details.cost);
	}

}

/**
 * Finds the sender details based on ip
 */
struct server_details find_server_details(uint32_t ip,uint16_t port){
	struct server_details sender;
	int i;
	for(i=1;i<=num_servers;i++){
		    //ACTUAL
	        if(routing_table[i].serv_details.ip == ip){
		   // if(routing_table[i].serv_details.port == port){
				 sender = routing_table[i].serv_details;
				 break;
			}
	}
	return sender;
}

/**
 * Checks for inactive nodes
 */
void check_for_inactive_nodes(){
	//printf("\nChecking for inactive nodes");
	int i;
	for(i=1;i<=num_servers;i++){
		//if the node is a neighbour and it has once sent update
		if(routing_table[i].is_neighbour==1 && routing_table[i].is_server_started==1){
		    routing_table[i].fail_update_counter++;
		    //printf("\nIncreasing counter %d",routing_table[i].fail_update_counter);
		    //printf("\nChecking inactive node %d",routing_table[i].serv_details.id);
			if(routing_table[i].fail_update_counter >= 4){
				     //printf("\nFound inactive node %d",routing_table[i].serv_details.id);
				     routing_table[i].next_hop_id = -1;
				     routing_table[i].fail_update_counter = INFINITY;
				     routing_table[i].is_neighbour_inactive = 1;
				     int server_id = routing_table[i].serv_details.id;
				     dv[host_id_g][server_id]=INFINITY;
				     run_bellman_ford();
					 break;
			}
		}
	}
}


/**
 * Checks if a given string contains only numbers. Returns 1 if valid and 0 if invalid
 */
int validate_integer(char *s){

	int flag = 1;
	int i;
	for (i = 0; i < strlen(s); i++) //Checking that each character of the string is numeric
	{
		    if (s[i]!='\n' && !isdigit(s[i]))
	        {
	            flag = 0;
	            break;
	        }
	}
	return flag;
}


/**
 * Updates the cost between servers
 */
int update_cost(){
    char *server_1_s = cmdargv[1];
    char *server_2_s= cmdargv[2];
    if(!(validate_integer(server_1_s) && validate_integer(server_2_s))){
    	return -1;
    }
    int server_1 = atoi(server_1_s);
    int server_2 = atoi(server_2_s);
    char *cost_str = cmdargv[3];
/*    printf("\nHost id %d",host_id_g);
    printf("\nServer 1 %d",server_1);
    printf("\nServer 2 %d",server_2);
    printf("\nCost String %s",cost_str);*/

    int cost_l,neighbour_id_l;

    //set the cost
    if(!strcasecmp(cost_str,"inf")){
    	cost_l = INFINITY;
    }
    else{
    	cost_l = atoi(cost_str);
    }

    //if the server 1 id is not equal to the host id flag error
    if(server_1 != host_id_g){
       return -2;
    }


    if(!(!strcasecmp(cost_str,"inf")|| validate_integer(cost_str))){
           return -4;
    }

    neighbour_id_l = server_2;

    if(!validateNeighbour(neighbour_id_l)){
    	return -3;
    }

    int i;
    for(i=1;i<=num_servers;i++){
    		if(routing_table[i].serv_details.id==neighbour_id_l){
    			        /*printf("\nBefore::");
    			        display_dv_matrix();*/
                        dv[host_id_g][neighbour_id_l]=cost_l;
    			    /*    printf("\nAfter::");
    			        display_dv_matrix();*/
                        run_bellman_ford();
                        //send updates to neighbours
                        //send_dv_to_neighbours();
    					break;
    		}
    }

    return 0;
}

/**
 * Display the count of packets received by the host
 */
void display_packets_count(){
	cse4589_print_and_log("%d\n",pkt_cnt_g);
	fflush(stdout);
	pkt_cnt_g = 0;
}


/**
 * Disable a link to the given server
 */
int disable_server(){
	char *server_id_s  = cmdargv[1];
	//checks if the sever id is an integer
	if(!validate_integer(server_id_s)){
        return -1;
	}
	int server_id = atoi(server_id_s);
	if(!validateNeighbour(server_id)){
        return -1;
	}
	int i;
	int link_disabled=0;
	for(i=1;i<=num_servers;i++){
		if(routing_table[i].serv_details.id==server_id && routing_table[i].is_neighbour==1){
                        dv[host_id_g][server_id]=INFINITY;
                        routing_table[i].is_neighbour = 0;
                        routing_table[i].fail_update_counter = 0;
						run_bellman_ford();
						break;
		}
	}
    return 0;
}

/**
 * Crash the server
 */
void crash_server(){
    //infinite while loop to crash the server
    while(1){

    }
}

/**
 * Dump a update packet
 */
void dump_packet(){
	int i;
	//1. make UDP packet

	struct host_details host_det;
	host_det.num_fields = htons(num_servers);
	//printf("\nNum of fields %d",num_servers);
	//printf("\nBig endian %d",host_det.num_fields);
	host_det.server_port =  htons(host_port);
	host_det.server_ip = host_ip;

	struct udp_packet packet;
	packet.host_det = host_det;
    packet.serv_details=(struct server_details *)malloc(num_servers*sizeof(struct server_details));

    int neighbour_index=0;
	for(i=1;i<=num_servers;i++){
							packet.serv_details[neighbour_index].ip = routing_table[i].serv_details.ip;
							packet.serv_details[neighbour_index].port = htons(routing_table[i].serv_details.port);
							packet.serv_details[neighbour_index].padding = htonl(0);
							packet.serv_details[neighbour_index].id = htons(routing_table[i].serv_details.id);
							packet.serv_details[neighbour_index].cost = htons(routing_table[i].serv_details.cost);
							neighbour_index++;
	}
	int packet_size = sizeof(host_det)+(num_servers)*sizeof(struct server_details);
	void *sndbuf=malloc(packet_size);
	//printf("\nSize %d",size);
	//testing
	memcpy(sndbuf,&packet.host_det,sizeof(struct host_details));
	int index;
	for(index=0;index<num_servers;index++){
			memcpy(sndbuf+sizeof(struct host_details)+index*sizeof(struct server_details),&packet.serv_details[index],sizeof(struct server_details));
	}
	//dump the packet
	//printf("\nPacket Size %d",packet_size);
	cse4589_dump_packet(sndbuf, packet_size) ;
	struct host_details host_det1;
	memcpy(&host_det1.num_fields,sndbuf,2);
    //printf("\nNum of updates %d",host_det1.num_fields);
}


/**
 * initialize the distance vector matrix
 * Reference - http://stackoverflow.com/questions/11463455/c-programming-initialize-2d-array-dynamically
 */
void init_dv_matrix(){
    int dim_x = num_servers+1; // to ignore any index subscripted by 0
    int dim_y = num_servers+1; // to ignore any index subscripted by 0
    dv = (int **) malloc(sizeof(int) * dim_x);// allocate memory to the rows
    int i;
    for (i = 0; i < dim_y; i++) {
    	dv[i] = (int *) malloc(sizeof(int) * dim_y);
    }
    //initialize the matrix entries to infinity except for (j,k) such that j=k
    int j,k;
    for(j=1;j<dim_x;j++){
    	 for(k=1;k<dim_y;k++){
            if(j==k){
            	dv[j][k] = 0;
            }
            else{
            	dv[j][k] = INFINITY;
            }

    	 }
   }

}

/**
 * Updates the routing table based on the distance vector matrix
 * Bellman Ford algorithm
 */
void update_routing_table_from_dv_matrix(int server_id,int server_cost,int next_hop_id){
   int i,j;
   for(i=1;i<=num_servers;i++){
		if(routing_table[i].serv_details.id==server_id){
			routing_table[i].serv_details.cost = server_cost;
			routing_table[i].next_hop_id = next_hop_id;
			break;
		}
   }
}

/**
 * displays the distance vector matrix
 */
void display_dv_matrix(){
    int dim_x = num_servers+1; // to ignore any index subscripted by 0
    int dim_y = num_servers+1;
	int j,k;
	for(j=1;j<dim_x;j++){
		 for(k=1;k<dim_y;k++){
			printf("%d ",dv[j][k]);
		 }
		 printf("\n");
   }
}


/**
 * runs the Bellman-Ford algorithm on the distance vector matrix
 * Reference - slide 77 of chapter 4
 */
void run_bellman_ford(){
   int u = host_id_g;
   int z;//destination host
   int w;//neighbours
   for(z=1;z<=num_servers;z++){
	   if(z!=host_id_g){
		   int cost = INFINITY;
		   int next_hop_id = -1;
		   for(w=1;w<=num_servers;w++){
				 if(validateNeighbour(w)){
					 int new_cost = dv[u][w]+dv[w][z];
					 if(new_cost<=cost){
						  cost = new_cost;
						  next_hop_id = w;
					 }
				 }
		   }
		   //handled the case when dv[u][w] and dv[w][z] are both infinity
		   if(cost>=INFINITY){
			   cost = INFINITY;
			   next_hop_id = -1;
		   }
		   update_routing_table_from_dv_matrix(z,cost,next_hop_id);
	   }
   }
}


/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 * Reference - sr.c template of PA2
 */
int isNumber(char *input)
{
    while (*input){
        if (!isdigit(*input))
            return 0;
        else
            input += 1;
    }

    return 1;
}

/**
 * Checks if the server_id is a neighbour
 */
int validateNeighbour(int server_id){
   int flag = 0;
   int i;
   for(i=1;i<=num_servers;i++){
   		if(routing_table[i].serv_details.id==server_id && routing_table[i].is_neighbour==1){
                flag =1;
   				break;
   		}
   	}
    return flag;
}

/**
 * Decreases the counter by 1 after each received message from the neighbour.
 * Also marks that the server is started
 */
void decrease_counter(int neighbour_id){
	int i;
	for(i=1;i<=num_servers;i++){
	   		if(routing_table[i].serv_details.id==neighbour_id){
	   			    //printf("\nDecreasing counter %d",routing_table[i].fail_update_counter);
	   			    routing_table[i].fail_update_counter = 0;
	   			    routing_table[i].is_server_started = 1;
	   				break;
	   		}
	}
}


/**
 * Sets the initial cost that is read from the topology file for this neighbour
 */
void set_initial_neighbour_cost(int neighbour_id){
	//printf("\nSetting initial cost");
    int cost=find_neighbour_cost(neighbour_id);
    dv[host_id_g][neighbour_id]=cost;
    dv[neighbour_id][host_id_g]=cost;
}

/**
 * Finds the cost of the neighbour
 */
uint16_t find_neighbour_cost(int neighbour_id){

	int i;
	int cost;
	for(i=1;i<=num_neighbours;i++){
		        //printf("\n%d %d",neighbour_details[i].server_id,neighbour_details[i].cost);
		   		if(neighbour_details[i].server_id==neighbour_id){
		   			    cost=neighbour_details[i].cost;
		   				break;
		   		}
	}
    return cost;

}


/**
 * Checks if the neighbour was marked inactive
 */
int check_if_neigh_was_inactive(int neighbour_id){
	int flag = 0;
	int i;
	for(i=1;i<=num_servers;i++){
			if(routing_table[i].serv_details.id==neighbour_id && routing_table[i].is_neighbour_inactive==1
					&& routing_table[i].is_neighbour==1){
					routing_table[i].is_neighbour_inactive = 0;
					flag = 1;
					break;
			}
    }
	return flag;
}
