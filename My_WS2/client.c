#include <stdio.h>
#include <stdlib.h>


/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}


int main (int argc, char **argv) {
/*	FILE *inputFile;
	int procFileFd;
	size_t len;
	char *line = NULL;
*/
	//check arguments

  if  ((2==argc) && ('L'==*argv[1])) {
		// do list rules
		printf("Argument received is %s\n",argv[1]);
	  } else if ( (argc >=  3) && ('W'==*argv[1]) ) {
		// do input new rules (ignore any additional parameters)
		printf("Arguments received are : %s  %s\n",argv[1], argv[2]);
    } else {
		//error in input parameters
    	fprintf (stderr, "Usage: %s {L | W <input file> }\n", argv[0]);
    	exit (1);
	}

	return 0;
} //end main


