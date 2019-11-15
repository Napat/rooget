#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>//socket
#include <sys/socket.h>//socket
#include <stdlib.h>//sizeof
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>//isdigit()
#include <fcntl.h>//open()
#include <dirent.h>
#include <sys/stat.h>//stat()
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h> // inet_pton() 
#include <termios.h> // struct termios _old, _new
#include <errno.h> // strerror(errno)

#define MAXSZ               4096
#define INITIALISE          0
#define IP4_MAXLEN          16

typedef enum{
	FTPCOMMU_STATE_0 = 0,
	FTPCOMMU_STATE_1,
    FTPCOMMU_STATE_2,
    FTPCOMMU_STATE_3
} ftpcommu_state_e;

typedef enum{
	RUNMODE_CLI = 0,
	RUNMODE_GET
} runmode_e;

typedef struct input_args_s {
    runmode_e runmode;    
    char ip_address[IP4_MAXLEN];
    int32_t port;
    char user[MAXSZ];
    char pass[MAXSZ];
    char filepath[MAXSZ];
} input_args_t;

// ftpcommu_s ftp client communication struct
typedef struct ftpcommu_s {    
    int sockfd;
    struct sockaddr_in serverAddress;
    char msg_from_serv[MAXSZ];      // message from client to server
    char msg_to_serv[MAXSZ];        // message from server to client
} ftpcommu_t;

#ifdef __PASSIVE_CONNECT_C__
    const char passive[]="PASV\r\n";
#else
    extern const char passive[];
    // xxx;
#endif

#endif /* __COMMON_H__ */
