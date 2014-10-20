/* A threaded server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include "comms.h"

#define BUFFERLENGTH 256

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
}


int active_filenames = 0;  // the number of threads currently performing encryption or decryption on a file
char *filename_ptr[30] = {NULL};
pthread_mutex_t mut; /* the lock */



/* decide if we are able to proceed with work on that filename */
int *proceed (void *args) {
	int empty =0;
  	pthread_mutex_lock (&mut); /* lock exclusive access to array filename_ptr */
	for (i=0, i<current+1, i++){
		//check if filename is in active list
		if (strcmp(request.filename, *filename_ptr[i]) {
			//filename found - we will not proceed
			pthread_mutex_unlock (&mut); /* release the lock */
			strcpy(request.msg,"File currently in use - operation terminated");
			return 99;
		}  
	if (NULL==filename_ptr[i]) empty = i;  
		/*just want to keep track of an empty slot - any one will do.  
			This is a terrible method but saves implementing linked list at this point */
	}

	// filename not found in currently active list - add to list	
	filename_ptr[empty] = &request.filename;
	pthread_mutex_unlock (&mut); /* release the lock */
	return empty;  //returns position where we just entered our filename
}



/* the procedure called for each request */
void *processRequest (void *args) {
  	int *newsockfd = (int *) args;
	struct Rqst request;
	struct Resp response;
	int i;
  	int n;
  	int tmp;
  
	/* read the data */
    n = read (newsockfd, &request, sizeof(request));
	if (n < 0) 
	 error ("ERROR reading from socket");

	printf("%i %s %s\n", request.op, request.passphrase, request.filename);

	//decide if we can operate on that filename 
	tmp = proceed();
	if (99>tmp{
		//we have exclusive use of that filename - do work
    	rc = do_gpg(request);
		printf("Error code %i\n",rc);

		//release our hold on filename
		pthread_mutex_lock (&mut); /* lock exclusive access to array filename_ptr */
		filename_ptr[tmp] = NULL;	
		pthread_mutex_unlock (&mut); /* release the lock */
	}


  n = sprintf (buffer, "I got you message, the  value of isExecuted is %d\n", isExecuted);
  /* send the reply back */
  n = write (*newsockfd, buffer, BUFFERLENGTH);
  if (n < 0) 
    error ("ERROR writing to socket");
       
  close (*newsockfd); /* important to avoid memory leak */  
  free (newsockfd);

  pthread_exit (NULL);
}



int main(int argc, char *argv[])
{
     socklen_t clilen;
     int sockfd, portno;
     char buffer[BUFFERLENGTH];
     struct sockaddr_in serv_addr, cli_addr;
     int result;



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

       pthread_t server_thread;

       int *newsockfd; /* allocate memory for each instance to avoid race condition */
       pthread_attr_t pthread_attr; /* attributes for newly created thread */

       newsockfd  = malloc (sizeof (int));
       if (!newsockfd) {
	 fprintf (stderr, "Memory allocation failed!\n");
	 exit (1);
       }

       /* waiting for connections */
       *newsockfd = accept( sockfd, 
			  (struct sockaddr *) &cli_addr, 
			  &clilen);
       if (*newsockfd < 0) 
	 error ("ERROR on accept");
       bzero (buffer, BUFFERLENGTH);

     /* create separate thread for processing */
     if (pthread_attr_init (&pthread_attr)) {
	 fprintf (stderr, "Creating initial thread attributes failed!\n");
	 exit (1);
     }

     if (pthread_attr_setdetachstate (&pthread_attr, !PTHREAD_CREATE_DETACHED)) {
       	 fprintf (stderr, "setting thread attributes failed!\n");
	 exit (1);
     }
     result = pthread_create (&server_thread, &pthread_attr, processRequest, (void *) newsockfd);
       if (result != 0) {
	 fprintf (stderr, "Thread creation failed!\n");
	 exit (1);
       }

       
     }
     return 0; 
}
