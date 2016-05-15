#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>
#include <errno.h>
#include "header.h"
#include "list.h"
#include <math.h>

#define DIE(x) perror(x),exit(1)
#define WORD 100
#define PORT 55123
#define WINDOW 10

int expected = 0;
int window_size = 10;
int win[WINDOW] ={0};


int search(FILE* fp){
     struct packet_list *loop ;
     struct packet_list *loop_temp ;
     LIST_FOREACH_SAFE(loop, &head, packet_lists, loop_temp){
        if(loop->my_packet.sequence == expected){
            fwrite(loop->my_packet.data, strlen(loop->my_packet.data), 1, fp);
            expected++;
            LIST_REMOVE(loop, packet_lists);   
            free(loop);
            return 1;
        }
     }
     return 0;
}
int main(int argc, char **argv) {
    LIST_INIT(&head);
    struct sockaddr_in server , client;
    struct header fileheader;
    int sd,n;
    int length;
    float p;

    memset(&fileheader,0, sizeof(fileheader));
    /* Setup socket.*/
    if((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
        perror("socket");
        exit(errno);
    }
    /* Initialize address. */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons( PORT );

    /* Name and activate the socket.*/
    if( bind(sd, (struct sockaddr*)&server, sizeof(server) ) == -1 )
    {
        perror("bind");
        exit(errno);
    }
    length = sizeof(client);
    while(1){

    	if ((n = recvfrom(sd, &fileheader, sizeof(struct header), 0, (struct sockaddr*)&client, (socklen_t *)&length)) > 0) {
                          printf("%s\n", fileheader.filename);
                          int packets = (fileheader.file_size-1)/PACKET_SIZE +1;
                          struct packet my_packet;
                          FILE * test ;

                          test = fopen("server_test.txt","w");
                          if (!test){
                                    perror("server fopen ");
                                    
                          }
                          while(packets>0){
                                MYRSCV(sd, &my_packet, sizeof(my_packet), 0, (struct sockaddr*)&client, (socklen_t *)&length);
                                printf("sequence : %d\n", my_packet.sequence);
                                //printf("data : %s\n", my_packet.data);

                                int temp = my_packet.sequence;
                                //memset(&my_packet,0, sizeof(my_packet));
                                //my_packet.ack = temp +1;
                                p   =   (random()%100)/100.0; 
                                if  (p  >=   0.1){
                                    if(temp == expected){
                                        
                                        my_packet.ack = ++expected;
                                        packets--;
                                        fwrite(my_packet.data, strlen(my_packet.data), 1, test);
                                        while(search(test) == 1){
                                                packets--;
                                        }
                                        my_packet.ack = expected;
                                    }
                                    else{
                                        my_packet.ack = expected;
                                        struct packet_list *np;
                                        np =  malloc(sizeof(struct packet_list));
                                        np->my_packet.sequence = temp;
                                        memcpy(np->my_packet.data, my_packet.data, PACKET_SIZE+1);
                                        LIST_INSERT_HEAD(&head, np, packet_lists); 
                                        
                                    }
                                }
                                else{
                                    printf("Drop packet : %d\n",temp);
                                    my_packet.ack = expected;
                                }
                                if( packets == 0)
                                    my_packet.finish = 1;
                                else
                                    my_packet.finish = 0;
                                MYSEND(sd, &my_packet, sizeof(my_packet), 0, (struct sockaddr*)&client, sizeof(client));
                          } 
                          fclose(test);
                          memset(&fileheader,0, sizeof(fileheader));
                          //memset(buf,0, WORD);
	}
 
    }/* end while loop */
	
    close(sd);
    
    return 0;	
    
} /* end main */

