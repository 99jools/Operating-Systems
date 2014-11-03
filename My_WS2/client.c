#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "kernel_comms.h"

#define  BUFFERSIZE 10
#define  MYPROCFILE "/proc/kernelReadWrite"

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}


/********************************************************************************/
/* listrules - instructs the kernel to list the currently loaded rules 		*/ 
/********************************************************************************/
int listrules () {
	
	struct rule ruleToKernel;
  	int procFileFd;

	procFileFd = open (MYPROCFILE, O_WRONLY); /* system call to open the proc-file for writing */
  
	if (procFileFd == -1) {
		fprintf (stderr, "Opening failed!  procFileFd = %i\n", procFileFd);
		exit (1);
	}

	ruleToKernel.op = 'L';
 	/* write rule to kernel - return value equals number of bytes writen
	(if negative, this indicates an error */	
	if (write (procFileFd, &ruleToKernel, sizeof(ruleToKernel) ) != sizeof(ruleToKernel) ){
		fprintf (stderr, "Error writing to kernel  procFileFd = %i\n", procFileFd);
		exit (1);
	}
	
	exit (0);

}

/*********************************************************************************/
/* addrules - reads a file and passes its content line by line to the kernel 	 */
/*********************************************************************************/
int addrules (char* ptr_inFilename) {
	
//	int statrc;
//	struct stat sb;
	struct rule ruleToKernel;
  	FILE *ptr_inputFile;
  	int procFileFd;

  	size_t len;
  	char* ptr_line = NULL;
	const char str_delim[2] = " ";
	
	ptr_inputFile = fopen (ptr_inFilename, "r");	/* open the input file for reading */	
	procFileFd = open (MYPROCFILE, O_WRONLY); 	/* system call to open the proc-file for writing */
  
	if (!ptr_inputFile || (procFileFd == -1)) {
		fprintf (stderr, "Opening failed!  procFileFd = %i\n", procFileFd);
		exit (1);
	}

	ruleToKernel.op = 'N';

	/* read the input file containing the rules and write structure to kernel - one write per rule */
	while (getline (&ptr_line, &len, ptr_inputFile) != -1) {

		//extract portno
		ruleToKernel.portno = atoi(strtok(ptr_line, str_delim));

		//extract filename
		strncpy(ruleToKernel.str_program, strtok(NULL, str_delim),256);

		//make sure this has a terminator
		ruleToKernel.str_program[256] = '\0';

		//check that file exists and is executable

/*
		  statrc = stat(ruleToKernel.str_program, &sb);
 printf("checking file status for %s - stat rc is %i  \n",ruleToKernel.str_program, statrc);

           if ((sb.st_mode & S_IFMT) == S_IFREG) {
                printf("is regular file\n");
           }
		      
*/	



 		/* write rule to kernel - return value equals number of bytes writen
		(if negative, this indicates an error */	
		if (write (procFileFd, &ruleToKernel, sizeof(ruleToKernel) ) != sizeof(ruleToKernel) ){
			fprintf (stderr, "Error writing to kernel  procFileFd = %i\n", procFileFd);
			free (ptr_line);
			exit (1);
		} 

		free (ptr_line); 
		ptr_line = NULL;
		ruleToKernel.op = 'A';
	} //end while
  
  	close (procFileFd); /* make sure data is properly writen */
  	fclose (ptr_inputFile);
  
  	return 0;
} // end addrules
  

/*********************************************************************/
int main (int argc, char **argv) {
	int rc = 0;

	//check arguments

	if  ((2==argc) && ('L'==*argv[1])) {
		// do list rules
		printf("Argument received is %s\n",argv[1]);
		rc = listrules();

	} else if ( (argc >=  3) && ('W'==*argv[1]) ) {
		// do input new rules (ignore any additional parameters)
		printf("Arguments received are : %s  %s\n",argv[1], argv[2]);
		rc = addrules(argv[2]);

	} else {
		//error in input parameters
		fprintf (stderr, "Usage: %s {L | W <input file> }\n", argv[0]);
		exit (1);
	}

	return rc;
} //end main


