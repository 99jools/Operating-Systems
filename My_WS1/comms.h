// contains the structures used for client/server comms
#ifndef COMMS_H
#define COMMS_H


struct Rqst {
	int op;
	char passphrase[151];
	char filename[151];
};


struct Resp {
	char msg[81];
};

#endif
