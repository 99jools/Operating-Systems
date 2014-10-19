#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "comms.h"

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

int do_gpg(struct Rqst request){
	int rc;
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
		default: return (cipherflag + plainflag + request.op); //error so don't proceed
	}

	// go ahead and execute command
	rc = system(command);
	printf("%i\n",rc);
	return rc;  // should be zero but return value just in case
} //end do_gpg



int main(){
	int rc;
	struct Rqst reqa;
	struct Rqst reqb;

	
	//initialise request
	reqa.op = 0;
	strcpy(reqa.passphrase, "mypassphrase");
	strcpy(reqa.filename, "myfile");

	reqb.op = 0;
	strcpy(reqa.passphrase, "mypassphraseb");
	strcpy(reqa.filename, "myfile");


	rc = do_gpg(reqa);
	printf("Error code %i\n",rc);

	rc = do_gpg(reqb);
	printf("Error code %i\n",rc);

	return 0;
} //end main
