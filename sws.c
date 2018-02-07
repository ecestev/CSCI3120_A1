/******************************************************************************
 ******************************************************************************

							 CSCI3120 - OPERATING SYSTEMS
						ASSIGNMENT # 1 - A SIMPLE WEB SERVER
				       			
   	 					 	    STEPHEN SAMPSON
					         	    B00568374
						  MAY 2016 - DALHOUSIE UNIVERSITY

 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 *                              INTIALIZATION                                 *
 ******************************************************************************/

/* Header files used in this program  */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "network.h"

/* Function Prototypes  */
int request_port(void);
int validate_port(int requested_port);
void parse(char **request_element, char *to_parse);

/******************************************************************************
 *                    				MAIN FUNCTION                                *
 ******************************************************************************/

int main(int argc, char *argv[]) {

/* SET UP AND INITIALIZE VARIABLES */
	int port_number = 0; 							/* port to initialize network on */
	int debug = 0; 	  /* debugging mode. If enabled, diagnostic info printed */
	int client_socket = 0;					/* socket # the client is connected on */
	char cwd[256];							  /* string for current working directory */
	char **request_elements = calloc(3, sizeof(char**)); 	 /* array of strings */
	char *to_parse = calloc(256, sizeof(char));			 /* string to be parsed */
	char *location;						  /*string for location of requested file */
	char *msg400 = "\nHTTP/1.1 400 BAD REQUEST\n"; 				  /* response 400 */
	char *msg404 = "\nHTTP/1.1 404 FILE NOT FOUND\n"; 			  /* response 404 */
	char *msg200 = "\nHTTP/1.1 200 OK\n"; 							  /* response 200 */
	char *filszerr = "\nRequested File Exceeds capabilities of sendfile()\n";
	FILE *requested_file;				  /* the actual file to be sent to client */

	/* CONFIGURE PROGRAM BEHAVIOR BASED ON COMMAND LINE ARGS  */
	if(argc==1)
		/* no arguments on command line - request a port */
		port_number = request_port();
	if(argc==2) 
		/* Port declared on command line - no debuggint */
		port_number = atoi(argv[1]);		
	if(argc==3) {
		/* Port declared on command line and debugging enabled */
		port_number = atoi(argv[1]);
		if (strcmp(argv[2],"-d")==0){;
			debug = 1;
			printf("\nDebugging Enabled\n");
			printf("Program is running from: %s\n", getcwd(cwd, sizeof(cwd)));
		}
	}


	/* BEGIN MAIN PROGRAM EXECUTION  */
 	if(debug==1)
		port_number = validate_port(port_number); /* validate port (debug mode) */
	network_init(port_number); 	  	  /* initialize network on specified port */
	while(1) {
		network_wait();   				 	  	  	  /*  wait for client to connect */
		if (debug==1)
			printf("\nClient Connected\n");
		client_socket = network_open(); 		 	 /* open a socket conn to client */
		if (client_socket > 0) {
			read(client_socket, to_parse, 256 ); 			 /* read client request */
			/* break HTTP request into its constituent elements */
			parse(request_elements, to_parse);	
			if((request_elements[0] == NULL) | (request_elements[1] == NULL)
			| (request_elements[2] == NULL)){
				if(debug==1)
					printf("Invalid Request: One of the request elements is NULL\n");
				write(client_socket, msg400, strlen(msg400));			
			} else {
				if((strcmp(request_elements[0], "GET") == 0)                                                                                                            
				&& (strcmp(request_elements[2], "HTTP/1.1") == 0)) {
				   location = request_elements[1];
					/* lines 88-102 (debug only) verify request parsed correctly */
					if (debug==1){
						printf("HTTP Request Parsed Successfully\n");	
						printf("Client Sent a Get Request\n");
						if(request_elements[1] != NULL) {
							printf("The Desired File Should Be Located at: ");
							printf(location);
						}
						if(request_elements[2] != NULL) {
							printf("\nThe Client is Requesting to use ");
							printf("%s", request_elements[2]);
							printf("\n");
						}
					}
					/* verify file was opened successfully before acting on it */
					if (fopen(location, "r") != NULL){	
						write(client_socket, msg200, strlen(msg200));  /* status OK */
						requested_file = fopen(location, "r");
						int fd = -1; 		  /* file descriptor for file to be sent  */
						fd = fileno(requested_file);
						fflush(stdout); 							/* flush output buffer  */
						if(debug==1)
							printf("The Stream has FD Number: %d\n", fd);
						struct stat buf;
						fstat(fd, &buf);	/* retrieve file stats for requested file */
						int filesize = buf.st_size;		/* size of requested file  */
						if(debug==1)
							printf("The File Size is: %d\n", filesize);
						if(filesize <= 2147479552) /* allowable sendfile() file size */
							/* send the file to the client */	
							sendfile(client_socket, fd, NULL, filesize);
						else
							write(client_socket, filszerr, strlen(filszerr));
						fflush(stdout);						 	 /* flush output buffer */
						fclose(requested_file);			  /* close the requested file */
						close(fd);
					} else {
						write(client_socket, msg404, strlen(msg404));
					}
				} else {
					write(client_socket, msg400, strlen(msg400));
				}
			} 
		write(client_socket, "\n", 2);
		/* Close Socket Connection  */
		close(client_socket);
		/* clear http request element strings if they hold data */
		int i=0;
		for(i=0; i<3; i++)
			if(request_elements[i] != NULL)
				memset(request_elements[i], '\0', strlen(request_elements[i]));
		}
	}
	free(request_elements);
	free(to_parse);
	return 0;
}		

/******************************************************************************
 *                           EXTERNAL FUNCTIONS                               *
 * ****************************************************************************/

/*
 * For debugging purposes or if no port is specified on 
 * the command line. Simply requests a port number and 
 * returns that value
 */
int request_port(void) {
	int requested_port = 0;
	printf("Enter a port number between 1024 and 65535: ");
	/* Scan user input for port and store for later use */
	scanf("%d", &requested_port);
	return requested_port;
}

/*
 *  For debugging purposes - validates that specified port
 *  is acceptable. If not, continues requesting a new port
 *  until an acceptable value has been entered at which 
 *  point this value returned to the main and stored as a 
 *  variable to be used when initializing the network 
 */
int validate_port(int validate_port) {
	while((validate_port<1024) | (validate_port>65535)) {
		printf("\nInvalid Port: %d...\n", validate_port);
		validate_port = request_port();
	}
	printf("Port Validated\n");
	printf("Waiting for Client Connection\n");
	return validate_port;
}

/*
 * A simple function to parse the HTTP request. This function
 * will store as many white space separated series of characters
 * as a string until there is no more room in the array of strings.
 */

void parse(char **request_elements, char *to_parse){
	char **ap;
 	char *inputstring = to_parse;
	for(ap=request_elements;(*ap=strsep(&inputstring, " \t\n\r")) != NULL;)
		if(**ap != '\0')
			if(++ap >= &request_elements[3])
				break;
}

/******************************************************************************
 *                              END OF FILE                                   *
 * ****************************************************************************/
		

