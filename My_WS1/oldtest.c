#include <stdio.h>


/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}




int main(){

	//check for correct number of arguments
 	if (argc < 3) {
  	     fprintf (stderr, "missing parameter\n");
  	     exit(1);
	}
	



	printf("Arguements received are : %s  %s\n",argv[1], argv[2]);
	return 0;
} //end main
