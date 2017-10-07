/*

Author: Muhammad Bilal Sadiq


client.c implements the TCP socket client
Input: server host name, server port, user name
Output: public key

*/
#include <stdio.h>
#include <string.h> /*strlen, strcpy functions*/
#include <errno.h>       /* obligatory includes */
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>

#define STRING_MAX 101 /*maximum number of characters in string, 1 for \0*/
#define MAX_KEY_BYTES 513 /*maximum number of bytes in key, 1 for \0 */
#define TERMINTATE_MSG "Terminate." /*terminate message*/

/*
Function: readString
Purpose: reads a string from standard input
out: string read in from the user (allocated in calling function)
*/
void readString(char *str)
{
	char tmpStr[STRING_MAX]; /*create an array of character to store user inpput*/
	fgets(tmpStr, sizeof(tmpStr), stdin); /*read a line*/
	tmpStr[strlen(tmpStr)-1] = '\0'; /*ignore new line character, append with \0 as null terminated character*/
	strcpy(str, tmpStr); /*copy to str, auto append \n to str*/
}

/*
Function: readInt
Purpose: reads an integer from standard input
out: integer read in from the user
*/
int readInt()
{
	int value; /*value read from standard input*/
	char tmpStr[STRING_MAX]; /*create an array of character to store user inpput*/
	readString(tmpStr);/*read a string*/
	sscanf(tmpStr, "%d", &value); /*convert to integer*/
	return value;
}

/*
Function: read_data
Purpose: read number of bytes from socket until '\0' is read
*/
int read_data(int s,     /* connected socket */
			  char *buf /* pointer to the buffer */
			  )
{ 
	int bcount; /* counts bytes read */
	int br;     /* bytes read this pass */
	int i; /*loop control variable*/
	char *first = buf; /*pointer to buf*/
	int gotData = 0; /*if got all data ending with \0*/
	bcount= 0;
	br= 0;
	while (gotData == 0 && bcount < STRING_MAX) {        /* loop until full buffer or '\0' found*/

		if ((br= read(s, buf, STRING_MAX - bcount)) > 0) {
			bcount += br;                /* increment byte counter */
			buf += br;                   /* move buffer ptr for next read */
		}
		else if (br < 0)                 /* signal an error to the caller */
			return(-1);

		/*
		printf("GOT %d\n", bcount);
		for (i = 0; i < bcount + 1; i++){
			printf("GOT %d\n", (int)first[i]);
		}
		*/
		for (i = 0; i < bcount; i++)
		{
			if (first[i] == '\0')
			{
				i = bcount; /*exit for condition*/
				gotData = 1; /*exit while condition*/
			}
		}
		
	}	
	
	return(bcount);
}

/*
Function: write_data
Purpose: write number of bytes to socket
*/
int write_data(
	int  s,                /* connected socket */
	char *buf,             /* pointer to the buffer */
	int  n)                /* number of characters (bytes) we want */
{ 
	int bcount;          /* counts bytes */
	int br;                /* bytes written  in one time */

	bcount= 0;
	br= 0;

	while (bcount < n) {             /* loop until full buffer */
		if ((br= write(s,buf,n-bcount)) > 0) {
			bcount += br;                /* increment byte counter */
			buf += br;                   /* move buffer ptr for next read */
		}
		if (br < 0) { 
			perror("write_data");        /* signal an error to the caller */
			return(-1);
		}
	}
	return(bcount);
}

/*
Function: call_socket
Purpose: connect server and return socket
*/
int call_socket(char * hostname, int portnum)
{ 
	struct sockaddr_in sa;   /*socket address*/
	struct hostent     *hp;  /*represent an entry in the hosts database*/
	int s;                   /*socket*/

	if ((hp= gethostbyname(hostname)) == NULL) { /* do we know the host's */
		errno= ECONNREFUSED;                       /* address? */
		return(-1);                                /* no */
	}

	bzero(&sa,sizeof(sa));
	bcopy(hp->h_addr,(char *)&sa.sin_addr,hp->h_length); /* set address */
	sa.sin_family= hp->h_addrtype;
	sa.sin_port= htons((u_short)portnum);

	if ((s= socket(hp->h_addrtype,SOCK_STREAM,0)) < 0)   /* get socket */
		return(-1);
	if (connect(s,(struct sockaddr *)&sa,sizeof sa) < 0) {                  /* connect */
		close(s);
		return(-1);
	}
	return(s);
}

/*main function to start application*/
int main(){
	char userID[STRING_MAX]; /*user id*/
	char publicKey[MAX_KEY_BYTES]; /*public key*/
	char hostname[STRING_MAX]; /*input host name*/
	int serverPort; /*server port*/
	int s; /*socket*/
	int n; /*bytes read*/

	/*prompt and read server host namefrom standard input*/
	printf("Enter a server host name: ");
	readString(hostname);
	//strcpy(hostname, "127.0.0.1");


	/*prompt and read server port*/
	printf("Enter server port number: ");
	serverPort =  readInt();	

	/*create socket*/
	s = call_socket(hostname, serverPort);

	strcpy(userID, "");

	/*process until user enter terminateing message*/
	while(strcmp(userID, TERMINTATE_MSG) != 0){

		/*prompt and read file name from standard input*/
		printf("Enter a user name: ");
		readString(userID);

		//printf("sending... %s\n", userID);

		/*send user id*/
		write_data(s, userID, strlen(userID) + 1);/*send \0 character too*/

		//printf("sent... %s\n", userID);

		if (strcmp(userID, TERMINTATE_MSG) != 0){
			/*read public key*/
			n = read_data(s, publicKey);
			publicKey[n] = '\0';

			/*display publicKey*/
			printf("The public key for user %s is %s\n", userID, publicKey);
		}		
	}

	return 0;
}
