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
/* listrules - instructs the kernel to list the currently loaded rules 			*/ 
/********************************************************************************/
int listrules () {
	
	struct ruleops ruleToKernel;
  	int procFileFd;

	procFileFd = open (MYPROCFILE, O_WRONLY); /* system call to open the proc-file for writing */
  
	if (procFileFd == -1) {
		fprintf (stderr, "Opening failed!  procFileFd = %i\n", procFileFd);
		exit (1);
	}

	ruleToKernel.op = 'L';
printf("%c\n",ruleToKernel.op);
	
	exit (0);

}

/*********************************************************************************/
/* addrules - reads a file and passes its content line by line to the kernel 	 */
/*********************************************************************************/
int addrules (char* p_inFilename) {
	
	struct ruleops ruleToKernel;
  	FILE *p_inputFile;
  	int procFileFd;

  	size_t len;
  	char* line = NULL;
	const char delim[2] = " ";
	
	p_inputFile = fopen (p_inFilename, "r");	/* open the input file for reading */	
	procFileFd = open (MYPROCFILE, O_WRONLY); 	/* system call to open the proc-file for writing */
  
	if (!p_inputFile || (procFileFd == -1)) {
		fprintf (stderr, "Opening failed!  procFileFd = %i\n", procFileFd);
		exit (1);
	}

	ruleToKernel.op = 'N';

	/* read the input file containing the rules and write structure to kernel - one write per rule */
	while (getline (&line, &len, p_inputFile) != -1) {

		//extract portno
		ruleToKernel.portno = atoi(strtok(line, delim));

		//extract filename
		strncpy(ruleToKernel.prog_filename, strtok(NULL, delim),256);

		//make sure this has a terminator
		ruleToKernel.prog_filename[256] = '\0';

//need to deal with problems if there are errors in input file

 		
if (		write (procFileFd, &ruleToKernel, sizeof(ruleToKernel) ) != sizeof(ruleToKernel) ); /* write rule to kernel */






		free (line); 
		line = NULL;
		ruleToKernel.op = 'A';
	}
  
  close (procFileFd); /* make sure data is properly written */
  fclose (p_inputFile);
  
  return 0;

}
  

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


