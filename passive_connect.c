#define __PASSIVE_CONNECT_C__

#include "common.h"

/*
Generate a passive PORT number from the variables sent by server.
*/
int passive_port_number(char *message)
{
	int i = INITIALISE;
	int count = INITIALISE;
	int port = INITIALISE;
	char *token;
	char delim[]=" ,)";/* Delimiters for strtok()*/

	while(message[i] != '\0' && count < 4)/* To reach to th first PORT variable */
	{
		if(message[i] == ',')
		{
			count++;
		}
		i++;
	}
				
	count = 0;

	token = strtok(message + i,delim);
	while(token != NULL)
	{
		if(isdigit(token[0]))
		{
			if(count == 1)
			{
				port += atoi(token);
			}
						
			if(count == 0)
			{
				port = atoi(token)*256;
				count++;
			}
						
		}
		token = strtok(NULL,delim);
	}
	return port;
	
}

/* Create PASSIVE socket and connect to server */
int func_to_connect_passive(char *address,int port)
{
	#define RECVTIMEOUT_SEC		10 
	int newsockfd;
	int	sockopt;
	struct sockaddr_in new_serverAddress;
	
	newsockfd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&new_serverAddress,sizeof(new_serverAddress));

	new_serverAddress.sin_family = AF_INET;
	new_serverAddress.sin_addr.s_addr = inet_addr(address);
	new_serverAddress.sin_port = htons(port);

	// setup recv timeout
	// recv with timeout https://stackoverflow.com/a/2939145/3616311
	struct timeval tv_nsfd;
	tv_nsfd.tv_sec = RECVTIMEOUT_SEC;	
	tv_nsfd.tv_usec = 100;

	if ((sockopt=setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_nsfd, sizeof(tv_nsfd))) == -1) { 
		// Enable reuse of a socket
		printf("%s(%d)\r\n", __FUNCTION__, __LINE__);
		perror("setsockopt"); 
		exit(1); 
	}

	connect(newsockfd,(struct sockaddr *)&new_serverAddress,sizeof(new_serverAddress));
	
	return newsockfd;
}
