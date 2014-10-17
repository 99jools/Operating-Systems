#include <stdio.h>
#include <stdlib.h>
/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

void do_gpg(int op, char passphrase[], char filename[]){
	int rc;
	char gpg_file[30];
	char command[150];

	// check whether we are in encrypt (0) or decrypt (1)
	if (0==op){
	   //check if file to be encrypted exists
	   if ( access(filename,F_OK) != -1){
		// plaintext exists
		printf("plaintext exists");
	   } else {
		printf("missing plaintext - returning 99");
		return 99;
	   }


	   //check if file is already encrypted
	   
	   if ( access(gpg_file,F_OK) != -1){
		// ciphertext exists
		printf("ciphertext already exists - returning 1");
		return 1;
	   } 
	   printf("no ciphertext - good to go");
	
	   // prepare encrypt command
	   snprintf(command, sizeof(command), "echo %s | gpg --batch -c --passphrase-fd 0 %s \n" , passphrase, filename);
	} else {
 	   // prepare decrypt command
	   snprintf(command, sizeof(command), "echo %s | gpg --batch -d --passphrase-fd 0 --output %s %s.gpg\n" , passphrase, filename, filename);
	}

	printf(command);
	rc = system(command);
	printf("%i\n",rc);
	printf("ls -l");

}

int main(int argc, char *argv[]){
	
	//check for correct number of arguements
 	if (argc < 4) {
  	     fprintf (stderr, "missing parameter\n");
  	     exit(1);
 	   }

    	do_gpg(atoi(argv[1]), argv[2], argv[3]);
	return 0;
}
