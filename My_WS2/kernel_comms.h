// contains the structures used for client/kernel comms via proc file
#ifndef KERNEL_COMMS_H
#define KERNEL_COMMS_H


struct ruleops {
	char op;
	int  portno;
	char prog_filename[257];
	int  prog_inode; 
};


#endif
