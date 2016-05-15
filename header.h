#ifndef HEADER_H
#define HEADER_H

#include "list.h"
#define PACKET_SIZE 1024
#define WORD 100
#define MYSEND(a,b,c,d,e,f)  do{ \
	if (sendto(a, b, c, d, e, f) < 0) { \
                            perror("sendto failed");\
                            return 0;\
             }\
}while(0)

#define MYRSCV(a,b,c,d,e,f)   do{ \
	if (recvfrom(a, b, c, d, e, f) < 0) { \
                            perror("rescfrom failed");\
                            return 0;\
             }\
}while(0)
struct packet
{
	int sequence;
	int ack;
	int finish;
	char  data[PACKET_SIZE+1];
	

};

struct header{
	long int file_size;
	char filename[WORD];
};

struct packet_list{
        struct packet my_packet;
        LIST_ENTRY(packet_list) packet_lists;
};
LIST_HEAD(packet_listhead, packet_list) head; 
#endif