// contains the structures used for client/kernel comms via proc file
#ifndef KERNEL_COMMS_H
#define KERNEL_COMMS_H


struct rule {
	char op;
	int  portno;
	char str_program[257];
};


#endif
