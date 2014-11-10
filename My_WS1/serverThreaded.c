/* A threaded server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "comms.h"

#define CONCURRENT 10

int count = 0; // the number of threads currently performing encryption or decryption on a file
char* pfiles[CONCURRENT];

pthread_mutex_t mut; /* the lock */


/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
} //end error

/* function to decide if we are able to proceed with work on that filename 
   manages locks and call local function to do work
*/
int proceed (struct Rqst* prequest) {
	int ret;  //the return code
	pthread_mutex_lock (&mut); /* lock exclusive access to array filename_ptr */
	ret = proceed_local(prequest);
	pthread_mutex_unlock (&mut); /* release the lock */
	return ret;   //returns 99 or the position of our entry in array
} // end proceed


/* local function to do decision work */
int proceed_local (struct Rqst* prequest) {
	int i;

printf("I am about to sleep - active files = %i\n",count);
sleep(10);
printf("I have woken up - active files = %i\n",count);
	if (CONCURRENT ==count) return 99;  // array is full - give up
	for (i=0; i<count; i++){
		//check if filename is in active list
		if (0==strcmp( prequest->filename, pfiles[i])) {
			//zero == filename found - we will not proceed
			return 99;
		}  
	}

	// filename not found in currently active list - add to list 	
	pfiles[count++] = prequest->filename;
	return (count - 1);  //returns the position where we just inserted our entry

} //end proceed_local

/*function that does encryption work */
int do_gpg(struct Rqst* prequest){
	char gpg_file[35];
	char command[151];
	int plainflag;
	int cipherflag;

	//check if plaintext exists
	if (access(prequest->filename,F_OK) != -1){
		plainflag = 2;
	} else plainflag = 0;

	//check if ciphertext exists
	memset(gpg_file, '\0', sizeof(gpg_file));
	strcpy(gpg_file, prequest->filename);
	strcat(gpg_file, ".gpg");

 	if ( access(gpg_file,F_OK) != -1){
		cipherflag = 4;
	} else cipherflag = 0;	
	printf("%i  %i   %i\n", prequest->op, plainflag, cipherflag);

	//sort out which case we are in
	switch (cipherflag + plainflag + prequest->op){
		case (0 + 2 + 0):
		   	// prepare for encrypt
	   		snprintf(command, sizeof(command), "echo %s | gpg --batch -c --passphrase-fd 0 %s \n" , 
				prequest->passphrase, prequest->filename);
			break;
		case (4 + 0 + 1):
			// prepare for decrypt
 		   	snprintf(command, sizeof(command), "echo %s | gpg --batch -d --passphrase-fd 0 --output %s %s.gpg\n" ,
				 prequest->passphrase, prequest->filename, prequest->filename);
			break;
		default: return (cipherflag + plainflag + prequest->op + 16); //error so don't proceed - 16 added to show it is an error code
	}

	// go ahead and execute command

	return system(command); // should be zero but return value just in case
} //end do_gpg

/* function called for each request */
void *processRequest (void *args) {
  	int *newsockfd = (int *) args;
	struct Rqst request;
	struct Resp response;
  	int n;
	int rc;
  	int position_inserted;
  

	/* read the data into request*/
    	n = read (*newsockfd, &request, sizeof(request));
	if (n < 0) 
	 error ("ERROR reading from socket");

	//decide if we can operate on that filename 
	position_inserted = proceed(&request);

	if (99==position_inserted ) {
		strcpy(response.msg,"File currently in use or concurrent limit exceeded - operation terminated");
	} else {
		//we have exclusive use of that filename - do work
    		rc = do_gpg(&request);
	
		//construct the response
		printf("Error code %i\n",rc);
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

		//update array to remove filename and release our hold on it
		pthread_mutex_lock (&mut); 	/* lock exclusive access to array filename_ptr */
		if ((count - position_inserted) > 1) {
			//we are not the last element in the array
			pfiles[position_inserted] = pfiles[count - 1];	/* remove entry by overwriting with last entry */

		}


		--count; 			/* decrease count to move end marker */
		pthread_mutex_unlock (&mut); 	/* release the lock */
	} 

    /* send the reply back */
	n = write (*newsockfd, &response, sizeof(response));
	if (n < 0) 
		error ("ERROR writing to socket");

	close (*newsockfd); /* important to avoid memory leak */  
	free (newsockfd);

  	pthread_exit (NULL);
} //end processRequest



int main(int argc, char *argv[])
{
     socklen_t clilen;
     int sockfd, portno;
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
