
/*
    C ECHO client example using sockets
*/
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <time.h>
 
uint64_t eal_tsc_resolution_hz = 0;


static uint64_t
get_tsc()
{
        union {
                uint64_t tsc_64;
                struct {
                        uint32_t lo_32;
                        uint32_t hi_32;
                };
        } tsc;


        asm volatile("rdtsc" :
                     "=a" (tsc.lo_32),
                     "=d" (tsc.hi_32));
        return tsc.tsc_64;
}

static int
set_tsc_freq_from_clock(void)
{
#define NS_PER_SEC 1E9

	struct timespec sleeptime = {.tv_nsec = 5E8 }; /* 1/2 second */

	struct timespec t_start, t_end;

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &t_start) == 0) {
			uint64_t ns, end, start = get_tsc();
			nanosleep(&sleeptime,NULL);
			clock_gettime(CLOCK_MONOTONIC_RAW, &t_end);
			end = get_tsc();
			ns = ((t_end.tv_sec - t_start.tv_sec) * NS_PER_SEC);
			ns += (t_end.tv_nsec - t_start.tv_nsec);

			double secs = (double)ns/NS_PER_SEC;
			eal_tsc_resolution_hz = (uint64_t)((end - start)/secs);
			return 0;
	}

	return -1;
}


int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    unsigned char buffer[1400] , server_reply[2000];
    unsigned long long total_rcvd = 0, rcvd;
	memset(buffer, 'A', 1400);

	printf("Clock measure:%d\n\n",set_tsc_freq_from_clock());

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("10.132.251.197");
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
    


    //keep communicating with server
    while(1)
    {      
        //Send some data
        if( send(sock , buffer , 1400 , 0) < 0)
        {
            puts("Send failed");
            return 1;
        }
         
        //puts("Server reply :");
        //puts(server_reply);
    }
     
    close(sock);
    return 0;
}
