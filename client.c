#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include <math.h>
#include <unistd.h> // for close
#include <netdb.h>

#define h_addr h_addr_list[0]

#define PORT 54321
#define WORD 100
#define STDIN 0
#define WIN_SIZE 10
static int connected =-1 ;
int last_acked = 0;
int c_sequence= 0;
int window_size = 5;
int packets= 0;
struct packet file_cache[WIN_SIZE] = {{0}}; 
int retransmit_count =0;
int ack_goal;
int send_file(int sock,char* filename, struct sockaddr_in* server,FILE* fp ){
        int i,counter=0;
        struct header file_header;
        struct packet my_packet;
        //struct sockaddr_in rscv;
        //int rscv_size;
        memset(&file_header,0, sizeof(file_header));
        memset(file_cache,0, sizeof(file_cache));

        strncpy(file_header.filename, filename, strlen(filename));
        fseek(fp, 0, SEEK_END);
        unsigned long int length = ftell(fp);
        file_header.file_size = length;
        fseek(fp, 0, SEEK_SET);
        MYSEND(sock, &file_header, sizeof(struct header), 0, (struct sockaddr *)server, sizeof(struct sockaddr_in));

        packets = (length-1)/PACKET_SIZE +1;
        ack_goal = c_sequence + packets;
        if(packets > WIN_SIZE){
            counter = WIN_SIZE;
            packets -=  WIN_SIZE;
        }
        else{
            counter = packets;
            packets =0;
        }
        printf("packets : %d\n", packets);
        for(i =0; i < counter ; i++){
                my_packet.sequence =  c_sequence++;
                memset(my_packet.data,0,PACKET_SIZE+1);
                fread(my_packet.data , PACKET_SIZE, 1, fp) ;
                memset(file_cache[my_packet.sequence%WIN_SIZE].data,0,PACKET_SIZE+1);
                memcpy(file_cache[my_packet.sequence%WIN_SIZE].data,my_packet.data,PACKET_SIZE+1);
                MYSEND(sock, &my_packet, sizeof(struct packet), 0, (struct sockaddr *)server, sizeof(struct sockaddr_in));
                
        }
        //fclose(fp);

        return 0;
}
/************************************************
connect to remote server
************************************************/
void connect_server(int sock,char *ip, int port ,struct sockaddr_in* server){
    //struct sockaddr_in server;

    struct hostent *phe;
    server->sin_family = AF_INET;
    server->sin_port = htons( port );

    if( (phe = gethostbyname(ip)) )
        memcpy(&server->sin_addr, phe->h_addr, phe->h_length);
    else if( !inet_aton(ip,&server->sin_addr)){
        printf("bad address\n");
        return ;
    }
    //printf("%s , %d\n",inet_ntoa(server->sin_addr),ntohs(server->sin_port));

    //Connect to remote server
    /*if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        exit(errno);
    }
    printf("The server with IP \"%s\" has accepted your connection.\n" , inet_ntoa(server.sin_addr));*/
    connected = 1;

}

/************************************************
print help message
************************************************/
void help_message(){
    printf("* Use connect [ IP ] [ port ] to connect to remote server\n");
    printf("* Use upload [ filename ] to upload file to remote server\n");
    printf("* Use exit to quit\n");
    printf("* Use help to show more details\n");
}

int main(int argc , char *argv[])
{
    int sock;
    int n;
    FILE *fp;
    char message[WORD]  = {0};
    char command[WORD] = {0};
    struct header file_header;
    struct sockaddr_in server;
    struct packet my_packet;
    struct sockaddr_in rscv;
    int rscv_size = sizeof(rscv);
    fd_set read_fds;
    fd_set master;printf("%s\n", "haha");
    FD_ZERO(&master); 
    FD_ZERO(&read_fds);

    FD_SET(STDIN, &master);
   
    memset(&file_header,0,sizeof(struct header));

    //Create socket
    sock = socket(AF_INET , SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        perror("sock");
        exit(errno);
    }
    FD_SET(sock, &master);
    help_message();
    while(1){
        read_fds = master;
        if (select(sock+1, &read_fds, NULL, NULL, NULL) == -1) {
                        perror("select");
                        exit(errno); 
        }
        if( FD_ISSET(STDIN, &read_fds) ){
                fgets(message , WORD , stdin);
                char *delim = " \n";
                char * pch ;
                pch = strtok(message,delim);
                
                // connect command
                if(strcmp(pch,"connect")==0){
                    char *ip = strtok(NULL,delim);
                    char *port = strtok(NULL,delim);
                    if( ip && port)
                        connect_server(sock,ip,atoi(port),&server);
                    else 
                        printf("Error ! Usage : connect [ IP ] [ PORT ]\n" );

                    
                }
                //upload command
                else if( strcmp(pch,"upload")==0 ){
                        strncpy ( command, pch, WORD);
                        command[WORD - 1] = '\0';
                        
                        char* filename = strtok(NULL,delim);
                        if(  connected == -1 ){
                            printf("ERROR! You are not connected to a remote server.\n");
                        }
                        else{
                            if( filename ){
                                fp = fopen(filename, "r");
                                if (!fp){
                                    perror("send_file fopen fp");
                                    
                                }
                                else{
                                    send_file(sock,filename,&server,fp);
                                }
                                
                                //strncpy(file_header.filename, filename, strlen(filename));
                                //MYSEND(sock, &file_header, sizeof(struct header), 0, (struct sockaddr *)&server, sizeof(server));
                                /*if (sendto(sock, &file_header, sizeof(struct header), 0, (struct sockaddr *)&server, sizeof(server)) < 0) {
                                    perror("sendto failed");
                                    return 0;
                                }*/
                            }
                            else{
                                printf("Error ! Usage : upload [ filename ]\n" );
                            }
                        }
                        
                }
                //exit command
                else if( strcmp(pch,"exit")==0 ){
                        printf("Goodbye.\n");
                        if(  (n = write(sock , pch, WORD) ) == -1){
                            perror("write");
                            exit(errno);
                        }
                        write(sock , pch, WORD);
                        
                        break;
                }
                // help command
                else if( strcmp(pch,"help")==0 ){
                        help_message();
                }
                else{
                    printf("No such command!\n" );
                }
                memset(message,0, WORD);
                memset(command,0, WORD);
                memset(&file_header,0, sizeof(struct header));
        }
        else if( FD_ISSET(sock, &read_fds) ){
                MYRSCV(sock, &my_packet, sizeof(struct packet), 0, (struct sockaddr *)&rscv, (socklen_t *)&rscv_size);
                printf("ACK : %d\n" ,my_packet.ack);
                //if( ack_goal > my_packet.ack){
                    //printf("ack_goal : %d\n", ack_goal );
                if(my_packet.ack <= last_acked && my_packet.ack >0){
                    //if(retransmit_count == 0){

                        retransmit_count++;
                        printf("packet loss ! Retransmit packet %d\n", my_packet.ack );
                        struct packet temp_packet;
                        temp_packet.sequence =  my_packet.ack;
                        memcpy(temp_packet.data, file_cache[my_packet.ack%WIN_SIZE].data, PACKET_SIZE+1);
                        //printf("packet:%d  data: %s\n" ,temp_packet.sequence,file_cache[my_packet.ack%WIN_SIZE].data);
                        MYSEND(sock, &temp_packet, sizeof(struct packet), 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
                    //}
                
                }
                else if(my_packet.ack > last_acked ){
                    //printf("%d %d\n",c_sequence , packets);
                    retransmit_count = 0;
                    last_acked = my_packet.ack ;
                    
                    if(packets > 0){
                        struct packet temp_packet;
                        temp_packet.sequence =  c_sequence++;
                        memset(temp_packet.data,0,PACKET_SIZE+1);
                        fread(temp_packet.data , PACKET_SIZE, 1, fp) ;
                        memset(file_cache[temp_packet.sequence%WIN_SIZE].data,0,PACKET_SIZE+1);
                        memcpy(file_cache[temp_packet.sequence%WIN_SIZE].data,temp_packet.data,PACKET_SIZE+1);
                        MYSEND(sock, &temp_packet, sizeof(struct packet), 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
                        packets--;
                        //printf("packet:%d  data: %s\n" ,temp_packet.sequence,temp_packet.data);
                    }
                    /*else  {
                        struct packet temp_packet;
                        temp_packet.sequence =  last_acked+1;
                        memcpy(temp_packet.data, file_cache[temp_packet.sequence %WIN_SIZE].data, PACKET_SIZE+1);
                        //printf("packet:%d  data: %s\n" ,temp_packet.sequence,file_cache[my_packet.ack%WIN_SIZE].data);
                        MYSEND(sock, &temp_packet, sizeof(struct packet), 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
                    }*/
                }
                /*else if( last_acked != (c_sequence-1) && my_packet.ack < 0){
                    struct packet temp_packet;
                    temp_packet.sequence =  last_acked+1;
                    memcpy(temp_packet.data, file_cache[temp_packet.sequence %WIN_SIZE].data, PACKET_SIZE+1);
                    //printf("packet:%d  data: %s\n" ,temp_packet.sequence,file_cache[my_packet.ack%WIN_SIZE].data);
                    MYSEND(sock, &temp_packet, sizeof(struct packet), 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
                }*/
                //}
        }

        
    }

    close(sock);
    return 0;
        
}


