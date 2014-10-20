/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "comms.h"

#define BUFFERLENGTH 256

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
}


int do_gpg(struct Rqst request){
	char gpg_file[35];
	char command[151];
	int plainflag;
	int cipherflag;


	//check if plaintext exists
	if (access(request.filename,F_OK) != -1){
		plainflag = 2;
	} else plainflag = 0;


	//check if ciphertext exists
	memset(gpg_file, '\0', sizeof(gpg_file));
	strcpy(gpg_file, request.filename);
	strcat(gpg_file, ".gpg");

 	if ( access(gpg_file,F_OK) != -1){
		cipherflag = 4;
	} else cipherflag = 0;	
printf("%i  %i   %i\n", request.op, plainflag, cipherflag);

	//sort out which case we are in
	switch (cipherflag + plainflag + request.op){
		case (0 + 2 + 0):
		   	// prepare for encrypt
	   		snprintf(command, sizeof(command), "echo %s | gpg --batch -c --passphrase-fd 0 %s \n" , request.passphrase, request.filename);
			break;
		case (4 + 0 + 1):
			// prepare for decrypt
 		   	snprintf(command, sizeof(command), "echo %s | gpg --batch -d --passphrase-fd 0 --output %s %s.gpg\n" , request.passphrase, request.filename, request.filename);
			break;
		default: return (cipherflag + plainflag + request.op + 16); //error so don't proceed - 16 added to show it is an error code
	}

	// go ahead and execute command

	return system(command); // should be zero but return value just in case
} //end do_gpg



int main(int argc, char *argv[])
{
	socklen_t clilen;
	int sockfd, newsockfd, portno;
	char buffer[BUFFERLENGTH];
	int rc;
	struct sockaddr_in serv_addr, cli_addr;
	struct Rqst request;
	struct Resp response;

     int n;
     if (argc < 2) {
         fprintf (stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
     /* create socket */
     sockfd = socket (AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero ((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons (portno);

     /* bind it */
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     /* ready to accept connections */
     listen (sockfd,5);
     clilen = sizeof (cli_addr);
     
     /* now wait in an endless loop for connections and process them */
     while (1) {
       
       /* waiting for connections */
       newsockfd = accept( sockfd, 
			  (struct sockaddr *) &cli_addr, 
			  &clilen);
       if (newsockfd < 0) 
	 error ("ERROR on accept");
       bzero (buffer, BUFFERLENGTH);
       
       /* read the data */
      	n = read (newsockfd, &request, sizeof(request));
       if (n < 0) 
	 error ("ERROR reading from socket");


printf("%i %s %s\n", request.op, request.passphrase, request.filename);

//now do the actual work
      rc = do_gpg(request);
	printf("Error code %i\n",rc);

//construct the response
	switch (rc){
		case 16:
		case 20:
			strcpy(response.msg, "Encrypt failed - Plaintext is missing"); 
			break;
		case 17:
		case 19:
			strcpy(response.msg, "Decrypt failed - Ciphertext is missing"); 
			break;
		case 22:
			strcpy(response.msg, "Encrypt Failed - Ciphertext already exists"); 
			break;
		case 23:
			strcpy(response.msg, "Decrypt Failed - Plaintext already exists"); 
			break;

		case 0:
			strcpy(response.msg, "Operation completed successfully"); 
			break;
			
		default: strcpy(response.msg, "GPG error - consult server log for more information"); 
	}



       /* send the reply back */
         n = write (newsockfd, &response, sizeof(response));
       if (n < 0) 
	 error ("ERROR writing to socket");
       
       close (newsockfd); /* important to avoid memory leak */
     }
     return 0; 
}
