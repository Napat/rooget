#include "common.h"
#include "list_client.h"
#include "passive_connect.h"
#include "list_content.h"
#include "get_content.h"
#include "put_content.h"
#include "put_unique.h"
#include "switchs.h"	// switchs()

// Validating IP Address
bool is_valid_ip(char *ip)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip, &(sa.sin_addr));
    return result != 0;
}

void help(char *argv[])
{
	printf( "Help\n"
                "\t %s <cli/get> <ip> [port] [user] [password] [filename]\n"
				"example:\n"  
				"\t %s get 127.0.0.1 21 napat 6543210 test.txt\n"
				"\t %s cli 127.0.0.1\n"
                , argv[0]
				, argv[0]
				, argv[0]
            );	
	exit(1);
}

bool is_numberstr(char *text)
{
    int j = strlen(text);
    while(j--)
    {
        if(text[j] >= '0' && text[j] <= '9')
            continue;
        return false;
    }
    return true;
}

// get_line OK return length of input line, otherwise negative value
int32_t get_line(char *prmpt, char *buff, size_t sz) 
{	// https://stackoverflow.com/a/2430310/3616311
    int ch, extra;

    // Size zero or one cannot store enough, so don't even
    // try - we need space for at least newline and terminator.
    if(sz < 2)
	{
        return -3;	// SMALL_BUFF
	}

    // Output prompt.
    if(prmpt != NULL) 
	{
        printf("%s", prmpt);
        fflush(stdout);
    }

    // Get line with buffer overrun protection.
    if(fgets(buff, sz, stdin) == NULL)
	{        
		return -1;	// NO_INPUT: buffer overrun
	}

    // If it was too long, there'll be no newline. In that case, we flush
    // to end of line so that excess doesn't affect the next call.
    size_t lastPos = strlen(buff) - 1;
    if(buff[lastPos] != '\n') 
	{
        extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
		{
            extra = 1;
		}
		return (extra == 1) ? -2 : strlen(buff);		// TOO_LONG : OK
    }

    // Otherwise remove newline and give string back to caller.
    //buff[lastPos] = '\0';
	// Remove trailing return and newline characters
	if((buff[lastPos] == '\n') || (buff[lastPos] == '\r'))
	{
		buff[lastPos] = '\0';
	}
	
	if((buff[lastPos - 1] == '\n') || (buff[lastPos - 1] == '\r'))
	{	
		buff[lastPos - 1] = '\0';
	}

    return strlen(buff);	// OK
}

ssize_t myfgets(FILE *stream, char *buf, size_t size)
{//https://codereview.stackexchange.com/a/119232
	if (size == 0)
		return 0;

	size_t count;
	int c = 0;
	for (count = 0; c != '\n' && count < size - 1; count++) 
	{
		c = getc(stream);

		if (c == EOF) {
			if (count == 0)
			return -1;
			break;
		}

		buf[count] = (char) c;
	}

	buf[count] = '\0';
	return (ssize_t) count;
}

ssize_t getmypass(char *prompt, char **lineptr, size_t *n, FILE *stream)
{ 	// https://stackoverflow.com/a/30801407/3616311
    struct termios _old, _new;
    int nread;

    // Turn echoing off and fail if we can’t. */
    if (tcgetattr (fileno (stream), &_old) != 0)
    {    
		return -1;
	}
    _new = _old;
    _new.c_lflag &= ~ECHO;
    if (tcsetattr (fileno (stream), TCSAFLUSH, &_new) != 0)
    {    
		return -1;
	}

    // Display the prompt
    if (prompt)
        printf("%s", prompt);

    // Read the password.
    //nread = getline(lineptr, n, stream);	// getline() is not support in c90 c99. It was originally GNU extensions and standardized in POSIX.1-2008.
	nread = myfgets(stream, *lineptr, *n);

    // Remove the carriage return
    if (nread >= 1 && (*lineptr)[nread - 1] == '\n')
    {
        (*lineptr)[nread-1] = 0;
        nread--;
    }
    printf("\n");

    // Restore terminal.
    (void) tcsetattr(fileno (stream), TCSAFLUSH, &_old);

    return nread;
}

// is_valid_args This function(argument parsing) should rewrite with getopt or somthing else. 
// But I'm too lazy to do it. Someone plz help...
bool is_valid_args(input_args_t *pinput_args, int argc, char *argv[]) {
    struct hostent *host;
    int ip_valid;
    int cnt;

	if(argc < 3)
	{
		help(argv);
	}

	// cli/get
    switchs(argv[1]) {
        icases("cli")
            printf("RUN MODE: cli\n");
            pinput_args->runmode = RUNMODE_CLI;
            break;
        icases("get")
            printf("RUN MODE: get\n");
            pinput_args->runmode = RUNMODE_GET;

			// get 127.0.0.1 21 napat 6543210 test.txt
			if(argc < 6)
			{
				help(argv);
			}
            break;
        defaults
            printf("Unknown runmode!!\n");
            exit(1);
            break;
    } switchs_end;
	
	// ip
	if(isdigit(argv[2][0]))
	{
		ip_valid = is_valid_ip(argv[2]);
	
		if(ip_valid == 0)
		{
			printf("Error: Invalid ip-address!\n");
			exit(1);
		}

		strcpy(pinput_args->ip_address, argv[2]);
	}
	else
	{
		host = gethostbyname(argv[2]);
		if(host == NULL)
		{
			switch(h_errno)
			{
				case NO_ADDRESS:
					printf("The requested name is valid but doesn't have any IP Address\n");
					break;
				case NO_RECOVERY:
					printf("A non-recoverable name server error occured\n");
					break;
				case TRY_AGAIN:
					printf("A temporary error occurred on authoritative name server. Try again later.\n");
				case HOST_NOT_FOUND:
					printf("Unknown host.\n");
					break;
				default:
					printf("Unknown error.\n");
			}
			exit(1);
		}
		while(host->h_addr_list[cnt] != NULL)
		{
			sprintf(pinput_args->ip_address,"%u.%u.%u.%u",
					host->h_addr_list[cnt][0] & 0x000000FF, 
					host->h_addr_list[cnt][1] & 0x000000FF,
					host->h_addr_list[cnt][2] & 0x000000FF,
					host->h_addr_list[cnt][3] & 0x000000FF
                    );
			cnt++;
		}
	}
	fprintf(stdout, "IP: %s\n", pinput_args->ip_address);

	// port
	if(argc < 4)
	{
		pinput_args->port = 21;
	}
	else if(is_numberstr(argv[3]))
	{
		pinput_args->port = (int)strtol(argv[3], (char **)NULL, 10);
		fprintf(stdout, "PORT: %d\n", pinput_args->port);
	} 
	else
	{
		help(argv);
	}

	// user
	if(argc < 5)
	{
		if(pinput_args->runmode == RUNMODE_CLI)
		{
			strncpy(pinput_args->user, "<none>", sizeof(pinput_args->user));
		}
		else
		{
			strncpy(pinput_args->user, "anonymous", sizeof(pinput_args->user));
		}
	}
	else
	{
		strncpy(pinput_args->user, argv[4], sizeof(pinput_args->user));
	}
	
	fprintf(stdout, "User: %s\n", pinput_args->user);
	
	// pass
	if(argc < 6)
	{
		if(pinput_args->runmode == RUNMODE_CLI)
		{
			strncpy(pinput_args->pass, "<none>", sizeof(pinput_args->pass));
		}
		else
		{
			strncpy(pinput_args->pass, "anonymous", sizeof(pinput_args->pass));
		}
	}
	else
	{
		strncpy(pinput_args->pass, argv[5], sizeof(pinput_args->pass));
	}
	
	//fprintf(stdout, "Password: %s\n", pinput_args->pass);

	// filepath
	if(argc < 7)
	{
		if(pinput_args->filepath == RUNMODE_CLI)
		{
			strncpy(pinput_args->filepath, "<none>", sizeof(pinput_args->filepath));
		}
		else
		{
			help(argv);
		}
	}
	else
	{
		strncpy(pinput_args->filepath, argv[6], sizeof(pinput_args->filepath));
	}
	fprintf(stdout, "File path: %s\n", pinput_args->filepath);

    return true;
}

bool ftp_initsock(ftpcommu_t *pftpcommu, input_args_t *pinput_args) {
    ssize_t len;
    ftpcommu_state_e ftpcommu_state;

	pftpcommu->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(pftpcommu->sockfd < 0)
	{
        perror("ERROR opening socket");
		exit(1);
	}

	bzero(&pftpcommu->serverAddress, sizeof(pftpcommu->serverAddress));

	pftpcommu->serverAddress.sin_family = AF_INET;
	pftpcommu->serverAddress.sin_addr.s_addr = inet_addr(pinput_args->ip_address);
	pftpcommu->serverAddress.sin_port = htons(pinput_args->port);

	// Connect to server
	if(connect(pftpcommu->sockfd, (struct sockaddr *)&pftpcommu->serverAddress, sizeof(pftpcommu->serverAddress)) < 0)
    {
        perror("ERROR connecting");
        exit(1);
	}

	printf("Connected to %s.\n", pinput_args->ip_address);

	// Receive message from server "Server will send 220"
	while((len = recv(pftpcommu->sockfd,  pftpcommu->msg_from_serv, MAXSZ, 0)) > 0 )
	{
		pftpcommu->msg_from_serv[len] = '\0';
		printf("%s\n", pftpcommu->msg_from_serv);
		fflush(stdout);
	
		if( strstr(pftpcommu->msg_from_serv, "220 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "421 ") > 0
        )
        {	
			break;
        }
	}

	if(strstr(pftpcommu->msg_from_serv, "421 ") > 0)
    {
		exit(1);
	}

	// if no user in argument
	if(strncmp(pinput_args->user, "<none>", sizeof(pinput_args->user)) == 0)
	{
		while( get_line("Username: ", pinput_args->user, sizeof(pinput_args->user)) < 0);
	}
	sprintf(pftpcommu->msg_to_serv, "USER %s\r\n", pinput_args->user);
    send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);

	// Receive message from server after sending user name.
	// Message with code 331 asks you to enter password corresponding to user.
	// Message with code 230 means no password is required for the entered username(LOGIN successful).
	while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0)
	{	
		pftpcommu->msg_from_serv[len] = '\0';
		if(strncmp(pftpcommu->msg_from_serv, "331",3) == 0)
		{
			ftpcommu_state = 1;
		}
	
		if(strncmp(pftpcommu->msg_from_serv, "230",3) == 0)
		{
			ftpcommu_state = 2;
		}
		
		if(strncmp(pftpcommu->msg_from_serv, "530",3) == 0)
		{
			ftpcommu_state = 0;	
		}

		if( strstr(pftpcommu->msg_from_serv, "230 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "332 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "331 ") > 0
        )
        {
			break;
        }
		fflush(stdout);
	}

	if(ftpcommu_state == 1)
	{
		// if no password in argument
		if(strncmp(pinput_args->pass, "<none>", sizeof(pinput_args->pass)) == 0)
		{
			char *ppwd = pinput_args->pass;
			size_t pass_maxlen = sizeof(pinput_args->pass);
			getmypass("Password: ", &ppwd, &pass_maxlen, stdin);
		}

		sprintf(pftpcommu->msg_to_serv, "PASS %s\r\n", pinput_args->pass);
		send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);

		while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0)
		{
			pftpcommu->msg_from_serv[len] = '\0';

			if(strncmp(pftpcommu->msg_from_serv, "230", 3) == 0)
			{
				ftpcommu_state = 2;
			}
			
			if(strncmp(pftpcommu->msg_from_serv, "530", 3) == 0)
			{
				ftpcommu_state = 0;	
			}
	
			if(strncmp(pftpcommu->msg_from_serv, "501", 3) == 0)
			{
				ftpcommu_state = 3;	
			}

			printf("%s\n",pftpcommu->msg_from_serv);
			fflush(stdout);

			if( strstr(pftpcommu->msg_from_serv, "230 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "332 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "503 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "202 ") > 0
            )
            {
				break;
            }
			
		}
	}	

	if(ftpcommu_state == 3)
	{
		send(pftpcommu->sockfd, pinput_args->pass, strlen(pinput_args->pass), 0);
		while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0)
		{
			if(strncmp(pftpcommu->msg_from_serv, "230", 3) == 0)
			{
				ftpcommu_state = 2;
			}
			
			if(strncmp(pftpcommu->msg_from_serv, "530", 3) == 0)
			{
				ftpcommu_state = 0;	
			}
		
			if(strncmp(pftpcommu->msg_from_serv, "501", 3) == 0)
			{
				ftpcommu_state = 3;	
			}
			printf("%s\n",pftpcommu->msg_from_serv);
			fflush(stdout);	

			if( strstr(pftpcommu->msg_from_serv, "230 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "332 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "503 ") > 0 || 
                strstr(pftpcommu->msg_from_serv, "202 ") > 0
            )
            {
                    break;	
            }
		}
	}
	

	if(ftpcommu_state == 0)
	{
		exit(1);
	}

	// Systen type(Server)
	sprintf(pftpcommu->msg_to_serv, "SYST\r\n");
	send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);

	while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0)
	{
		pftpcommu->msg_from_serv[len] = '\0';
		printf("%s\n", pftpcommu->msg_from_serv);
		if( strstr(pftpcommu->msg_from_serv, "215 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
            strstr(pftpcommu->msg_from_serv, "502 ") > 0
        )
        { 	
			break;
        }
	}
	
    return true;
}

void runmode_cli(ftpcommu_t *pftpcommu, input_args_t *pinput_args)
{
	int len;
	char user_input[MAXSZ];
	char dir[MAXSZ];
	char working_dir[MAXSZ];
	char old_name[MAXSZ];
	char new_name[MAXSZ];
	int dir_check;
	
	ftpcommu_state_e ftpcommu_state = 0;

	// user operation loop
	while(1)
	{
		ftpcommu_state = 0;
	
		bzero(user_input, sizeof(user_input));
		bzero(pftpcommu->msg_to_serv, sizeof(pftpcommu->msg_to_serv));
		bzero(pftpcommu->msg_from_serv, sizeof(pftpcommu->msg_from_serv));
		bzero(working_dir, sizeof(working_dir));
		bzero(old_name, sizeof(old_name));
		bzero(new_name, sizeof(new_name));
	
		printf("rooftp> ");
		fflush(stdout);

		len = read(STDIN_FILENO, user_input, sizeof(user_input));
		user_input[len] = '\0';		
		
		// Remove trailing return and newline characters
		if((user_input[len - 1] == '\n') || (user_input[len - 1] == '\r'))
		{
			user_input[len - 1] = '\0';
		}
		
		if((user_input[len - 2] == '\n') || (user_input[len - 2] == '\r'))
		{	
			user_input[len - 2] = '\0';
		}

		// cmd: exit
		if( strcmp(user_input, "exit") == 0 || 
            strcmp(user_input, "quit") == 0 || 
            strcmp(user_input, "bye") == 0
        )
		{
			sprintf(pftpcommu->msg_to_serv, "QUIT\r\n");
		
			send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, sizeof(pftpcommu->msg_from_serv), 0)) > 0)
			{
				pftpcommu->msg_from_serv[len] = '\0';
				printf("%s\n",pftpcommu->msg_from_serv);
				if( strstr(pftpcommu->msg_from_serv, "221 ") > 0 || 
                    strstr(pftpcommu->msg_from_serv, "500 ") > 0)	
					break;
			}
			break;
		}

		// cmd: Change client side directory
		if( strncmp(user_input, "!cd ", 4) == 0 || 
            strcmp(user_input, "!cd") == 0)
		{
			if(chdir(user_input + 4) == 0)
			{
				printf("Directory successfully changed\n\n");
			}			
			else
			{
				perror("Error");
			}
		}

		// cmd: Get client side current working directory 
		if( strncmp(user_input, "!pwd ", 5) == 0 || 
            strcmp(user_input, "!pwd") == 0
        )
		{
			getcwd(working_dir, sizeof(working_dir));	
			printf("%s\n\n", working_dir);
		}	

		// cmd: List files with details in current working directory on client side
		if( strncmp(user_input, "!ls -l ", 7) == 0 || 
            strcmp(user_input, "!ls -l") == 0)
		{
			getcwd(working_dir, sizeof(working_dir));	
			ls_l_dir(working_dir);
			continue;
		}
		
		// cmd: List files in current working directory on client side	
		if( strncmp(user_input, "!ls ", 4) == 0 || 
            strcmp(user_input, "!ls") == 0
        )
		{
			getcwd(working_dir, sizeof(working_dir));	
			ls_dir(working_dir);
		}
		
		// cmd: Create directory on client side
		if(strncmp(user_input, "!mkdir ", 7) == 0)
		{
			dir_check = mkdir(user_input + 7, 0755);
			if(dir_check == -1)
				perror("Error");
			else
				printf("Directory successfully created\n");
			printf("\n");
		}

		// cmd: Remove directory on client side
		if(strncmp(user_input, "!rmdir ", 7) == 0)
		{
			dir_check = rmdir(user_input + 7);
			if(dir_check == -1)
				perror("Error");
			else
				printf("Directory successfully removed\n");
			printf("\n");
		}		
	
		// cmd: Change directory on server side
		if(strncmp(user_input, "cd ", 3) == 0)
		{
			sprintf(dir, "CWD %s\r\n", user_input + 3);
			send(pftpcommu->sockfd, dir, strlen(dir), 0);
		
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, sizeof(pftpcommu->msg_from_serv), 0)) > 0)
			{
				pftpcommu->msg_from_serv[len] = '\0';
				printf("%s\n", pftpcommu->msg_from_serv);
				fflush(stdout);
				
				if(	strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "250 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "502 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "550 ") > 0
				)
				{
					break;
				}
			}
		}
	
		// cmd: List files on server side
		if(	strncmp(user_input, "ls ", 3) == 0 || strcmp(user_input, "ls")== 0)
		{
			list_content(pinput_args->ip_address, user_input, pftpcommu->sockfd);	
		}
	
		// cmd: Current working directory on server side
		if(strcmp(user_input, "pwd") == 0)
		{
			sprintf(pftpcommu->msg_to_serv, "PWD\r\n");
			send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);
		
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, sizeof(pftpcommu->msg_from_serv), 0)) > 0)
			{
				pftpcommu->msg_from_serv[len] = '\0';
				printf("%s\n", pftpcommu->msg_from_serv);
				fflush(stdout);
				
				if(	strstr(pftpcommu->msg_from_serv, "257 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "502 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "550 ") > 0
				)
				{
					break;
				}
			}
		}

		// cmd: Download file from server
		if(strncmp(user_input, "get ", 4) == 0)
		{
			clock_t start,end;
			double cpu_time;

			start = clock();
			get_content(pinput_args->ip_address, user_input, pftpcommu->sockfd);
			end = clock();
			cpu_time = ((double)(end - start))/CLOCKS_PER_SEC;
			printf("Time taken %lf\n\n", cpu_time);
		}
		
		// cmd: Upload file to server
		if(strncmp(user_input, "put ", 4) == 0)
		{
			put_content(pinput_args->ip_address, user_input, pftpcommu->sockfd);
		}

		// cmd: Upload file uniquely to server
		if(strncmp(user_input, "uniqput ", 8) == 0)
		{
			put_unique(pinput_args->ip_address, user_input, pftpcommu->sockfd);
		}
		
		// cmd: Rename file on server	
		if(strncmp(user_input, "rename ", 7) == 0)
		{
			int cnt;

			// parse user input to get old file name and new file name
			cnt = sscanf(user_input, "%s %s %s", pftpcommu->msg_to_serv, old_name, new_name);
			if(cnt != 3)
			{		
				printf("Error: rename expects two arguments\n\n");
				continue;
			}			

			sprintf(pftpcommu->msg_to_serv, "RNFR %s\r\n", old_name);
		
			send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, sizeof(pftpcommu->msg_from_serv), 0)) > 0 )
			{
				pftpcommu->msg_from_serv[len] = '\0';

				// RNFR fails
				if(strncmp(pftpcommu->msg_from_serv, "550", 3) == 0)
				{
					ftpcommu_state = 1;	
				}
					printf("%s\n", pftpcommu->msg_from_serv);
				
				fflush(stdout);
				
				if(	strstr(pftpcommu->msg_from_serv, "350 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "450 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "502 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "550 ") > 0
				)
				{
					break;
				}	
			}
			
			if(ftpcommu_state == 1)
				continue;

			sprintf(pftpcommu->msg_to_serv, "RNTO %s\r\n", new_name);

			send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0 )
			{
				pftpcommu->msg_from_serv[len] = '\0';
				if(strncmp(pftpcommu->msg_from_serv, "550", 3) == 0)/* RNTO fails*/
				{
					printf("Error: Renaming file failed.\n\n");
				}
				else
				{
					printf("%s\n", pftpcommu->msg_from_serv);
				}
				fflush(stdout);
				
				if( strstr(pftpcommu->msg_from_serv, "553 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "250 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "532 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "502 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "503 ") > 0
				)
				{
					break;
				}
			}
		}

		// cmd: Creating diectory on server
		if(strncmp(user_input, "mkdir ", 6) == 0)
		{	
			sprintf(pftpcommu->msg_to_serv, "MKD %s\r\n", user_input + 6);
			send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0 )
			{
				pftpcommu->msg_from_serv[len] = '\0';
				if(strncmp(pftpcommu->msg_from_serv, "550", 3) == 0)/* MKD fails*/
				{
					printf("Error: Creating directory failed.\n\n");
				}
				else
				{
					printf("%s\n", pftpcommu->msg_from_serv);
				}
				fflush(stdout);

				if(	strstr(pftpcommu->msg_from_serv, "257 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "502 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "550 ") > 0
				)
				{
					break;
				}
			}
		}

		// cmd: Removing directory on server
		if(strncmp(user_input, "rmdir ", 6) == 0)
		{	
			sprintf(pftpcommu->msg_to_serv, "RMD %s\r\n", user_input + 6);
			send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0 )
			{
				pftpcommu->msg_from_serv[len] = '\0';
				if(strncmp(pftpcommu->msg_from_serv, "550", 3) == 0)/* RMD fails*/
				{
					printf("Error: Removing directory failed.\n\n");
				}
				else
				{
					printf("%s\n", pftpcommu->msg_from_serv);
				}
				
				fflush(stdout);

				if(	strstr(pftpcommu->msg_from_serv, "250 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "502 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "550 ") > 0
				)
				{
					break;
				}	
			}
		}
		
		// cmd: Delete file on server
		if(strncmp(user_input, "rm ", 3) == 0)
		{	char ans[MAXSZ];
			
			get_line("Do you really want to remove this file? yes/no \n"
						, pinput_args->user
						, sizeof(ans)
					);
			
			if(strcasecmp(ans, "yes") != 0)
				continue;

			sprintf(pftpcommu->msg_to_serv, "DELE %s\r\n", user_input + 3);

			send(pftpcommu->sockfd, pftpcommu->msg_to_serv, strlen(pftpcommu->msg_to_serv), 0);
			while((len = recv(pftpcommu->sockfd, pftpcommu->msg_from_serv, MAXSZ, 0)) > 0 )
			{
				pftpcommu->msg_from_serv[len] = '\0';
				if(strncmp(pftpcommu->msg_from_serv, "550", 3) == 0)/* DEL fails*/
				{
					printf("Error: Removing file failed.\n\n");
				}
				else
				{
					printf("%s\n", pftpcommu->msg_from_serv);
				}
				fflush(stdout);

				if(	strstr(pftpcommu->msg_from_serv, "250 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "450 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "530 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "500 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "501 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "421 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "502 ") > 0 || 
					strstr(pftpcommu->msg_from_serv, "550 ") > 0
				)
				{
					break;
				}
			}
		}
	}
}

void runmode_get(ftpcommu_t *pftpcommu, input_args_t *pinput_args)
{
	clock_t start, end;
	double cpu_time;
	char user_input[MAXSZ + 4];

	sprintf(user_input, "get %s", pinput_args->filepath);

	start = clock();
	get_content(pinput_args->ip_address, user_input, pftpcommu->sockfd);
	end = clock();
	cpu_time = ((double)(end - start))/CLOCKS_PER_SEC;
	printf("Time taken %lf\n\n", cpu_time);
}

int main(int argc, char *argv[])
{
    input_args_t input_args;
    ftpcommu_t ftpcommu;

    memset(&input_args, 0, sizeof(input_args));
    input_args.runmode = RUNMODE_CLI;
    strncpy(input_args.ip_address, "0.0.0.0", sizeof(input_args.ip_address));
    strncpy(input_args.user, "napat", sizeof(input_args.user));
    strncpy(input_args.pass, "xxxxx", sizeof(input_args.pass));
    
    memset(&ftpcommu, 0, sizeof(ftpcommu));

    if(is_valid_args(&input_args, argc, argv) == false)
    {
        printf("%s(%d) ERROR!\n", __FUNCTION__, __LINE__);
        exit(1);
    }

    if(ftp_initsock(&ftpcommu, &input_args) == false)
    {
        printf("%s(%d) ERROR!\n", __FUNCTION__, __LINE__);
        exit(1);
    }
	
	switch(input_args.runmode)
	{
		case RUNMODE_CLI:
			runmode_cli(&ftpcommu, &input_args);
			break;
		case RUNMODE_GET:
			printf("Still inprogress!!!\n");
			runmode_get(&ftpcommu, &input_args);
			break;
		default:
			printf("%s(%d) Unknown error!!\n", __FUNCTION__, __LINE__);
			exit(1);
	}

	close(ftpcommu.sockfd);
	return 0;
}
