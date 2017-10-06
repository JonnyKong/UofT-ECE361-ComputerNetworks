typedef struct tagPacket {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char filename[50];
	char filedata[1000]; 
} Packet;
