/*

Author: Muhammad Bilal Sadiq


server.c implements the TCP socket server (not multi thread)
Input: file name, read file ((userID, public key))
Output: the server accepts connections, read, search userID and return public key

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

/*server table should be able to store up to 1,024 pairs (userID, public key)*/
#define SERVER_TABLE 1024

#define MAX_QUEUED_CONNECTS 10 /* max # of queued connects */ 

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
Function: loadTable
Purpose: reads user ids, public keys from file
out: number of pairs, user ids, public keys
*/
int loadTable(char filename[], char userIDs[][STRING_MAX], char publicKeys[][MAX_KEY_BYTES]){
	int numPairs = 0;/*number of pairs*/

	FILE * fp; /*file input*/

	/*close file for reading*/
	fp = fopen(filename, "r");

	if (fp){
		/*read a pair one by one*/
		while	 (fscanf(fp, "%s %s", userIDs[numPairs], publicKeys[numPairs]) == 2){
			numPairs++;/*next index*/
		}

		fclose(fp);/*close file*/
	}else{
		printf("Could not open file %s\n", filename);
	}

	return numPairs;
}

/*
Function: getPublicKeyByID
Purpose: get public key by user id
out: public key by user id (public key allocated by calling function)
*/
void getPublicKeyByID(int numPairs, char userID[], char publicKey[], char userIDs[][STRING_MAX], char publicKeys[][MAX_KEY_BYTES]){

	int i; /*loop control variable*/

	/*set empty key*/
	strcpy(publicKey, "");

	/*sequential search on array*/
	for (i = 0; i < numPairs; i++)
	{
		/*compare user id*/
		if (strcmp(userID, userIDs[i]) == 0)
		{
			strcpy(publicKey, publicKeys[i]); /*copy string*/
			i = numPairs; /*exit loop*/
		}
	}
}

/* 
Function: establish
Purpose: code to establish a socket; originally from bzs@bu-cs.bu.edu
return the server socket
*/

int establish(int portnum)
{ 
	char   myname[STRING_MAX]; /*server name*/
	int    s;                  /*socket*/ 
	struct sockaddr_in sa;     /*socket address*/ 
	struct hostent *hp;        /*represent an entry in the hosts database*/

	memset(&sa, 0, sizeof(struct sockaddr_in));  /* clear our address */
	gethostname(myname, STRING_MAX);            /* who are we? */
	hp= gethostbyname(myname);                  /* get our address info */
	if (hp == NULL)                             /* we don't exist !? */
		return(-1);
	sa.sin_family= hp->h_addrtype;              /* this is our host address */
	sa.sin_port= htons(portnum);                /* this is our port number */
	if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0) /* create socket */
		return(-1);
	if (bind(s,(struct sockaddr *)&sa,sizeof(struct sockaddr_in)) < 0) {
		close(s);
		return(-1);                               /* bind address to socket */
	}
	listen(s, MAX_QUEUED_CONNECTS);                               /* max # of queued connects */
	return(s);
}

/* 
Function: get_connection
Purpose: wait for a connection to occur on a socket created with establish()
return socket of connection
*/
int get_connection(int s)
{ 
	int t;                  /* socket of connection */

	if ((t = accept(s,NULL,NULL)) < 0)   /* accept connection if there is one */
		return(-1);

	return(t);
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


/*main function to start application*/
int main(){

	char filename[STRING_MAX]; /*input file name*/
	int serverPort; /*server port*/
	int numPairs; /*number of pairs (user id, public key)*/
	/*server table stores pairs (userID, public key)*/
	char userIDs[SERVER_TABLE][STRING_MAX]; 
	char publicKeys[SERVER_TABLE][MAX_KEY_BYTES]; 
	int s; /*server socket*/
	int t; /* socket of connection */
	char userID[STRING_MAX]; /*user id*/
	char publicKey[MAX_KEY_BYTES]; /*public key*/

	/*prompt and read file name from standard input*/
	printf("Enter a file name: ");
	readString(filename);
	//strcpy(filename, "keys21.txt");

	/*prompt and read server port*/
	printf("Enter server port number: ");
	serverPort = readInt();
	//serverPort = 1234;

	/*load pairs (userID, public key) from file*/
	numPairs = loadTable(filename, userIDs, publicKeys);

	/*create server socket*/
	if ((s= establish(serverPort)) < 0) {
		perror("establish");
		exit(1);
	}

	for (;;) {                          /* loop for client requests, exit when CTRL + C */
		if ((t= get_connection(s)) < 0) { /* get a connection */
			if (errno == EINTR)             /* EINTR might happen on accept(), */
				continue;                     /* try again */
			perror("accept");               /* bad */
			exit(1);
		}

		/*process request*/
		/*read user id*/
		int n = read_data(t, userID);
		userID[n] = '\0';

		/*process until user enter terminateing message*/
		while (strcmp(userID, TERMINTATE_MSG) != 0){

			/*find public key*/
			getPublicKeyByID(numPairs, userID, publicKey, userIDs, publicKeys);

			/*send public key*/
			write_data(t, publicKey, strlen(publicKey) + 1);/*send \0 character too*/

			/*read user id*/
			int n = read_data(t, userID);
			userID[n] = '\0';
		}/*end while*/
	}

	return 0;
}
