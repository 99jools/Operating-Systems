#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "entry.h"

#define  BUFFERSIZE 10
#define  MYPROCFILE "/proc/jrs1"



/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

/************************************************************************/
int listrules () {
  
	int procFileFd;
	struct entry_t buffer[BUFFERSIZE];
	int count = 0;
	int currentlyRead = 0;
	int i;

	procFileFd = open (MYPROCFILE, O_RDONLY);
  
	if (procFileFd == -1) {
		fprintf (stderr, "Opening failed!\n");
		exit (1);
	}

	while (count < BUFFERSIZE * sizeof (struct entry_t)) {
		/* if (lseek (procFileFd, count, SEEK_SET) == -1) { */
		/*   fprintf (stderr, "Seek failed!\n"); */
		/*   exit (1); */
		/* }  */
		currentlyRead = read (procFileFd, buffer + count, BUFFERSIZE * sizeof (struct entry_t) - count);
		if (currentlyRead < 0) {
			fprintf (stderr, "Reading failed! \n");
			exit (1);
		}
		count = count + currentlyRead;
		if (currentlyRead == 0) { 
			/* EOF encountered */
			break;
		}
	}
	printf ("Read buffer of size %d\n", count);
	for (i = 0; i * sizeof (struct entry_t) < count; i++) {
		printf ("Next field1 element is %d\n", buffer[i].field1);
		printf ("Next field2 element is %d\n", buffer[i].field2);
	}
	close (procFileFd);
	exit (0);

}

/*********************************************************************************/
/* Reads a file and passes its content line by line to the kernel 		 */
int addrules (char* p_inFilename) {
  
  FILE *p_inputFile;
  int procFileFd;
  size_t len;
  char* line = NULL;

  p_inputFile = fopen (p_inFilename, "r");	/* open the input file for reading */
  procFileFd = open (MYPROCFILE, O_WRONLY); /* system call to open the proc-file for writing */
  
  if (!p_inputFile || (procFileFd == -1)) {
    fprintf (stderr, "Opening failed!  procFileFd = %i\n", procFileFd);
    exit (1);
  }

  while (getline (&line, &len, p_inputFile) != -1) {
    write (procFileFd, line, len); /* write line to kernel */
    free (line);
    line = NULL;
  }
  
  close (procFileFd); /* make sure data is properly written */
  fclose (p_inputFile);
  
  return 0;

}
  


/*********************************************************************/

/*********************************************************************/

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


