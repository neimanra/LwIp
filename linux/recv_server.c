
#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
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
    int socket_desc , client_sock , c , read_size;
    unsigned long long total_rcvd = 0;
    struct sockaddr_in server , client;
    char client_message[2000];
     
    printf("Clock measure:%d\n\n",set_tsc_freq_from_clock());

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("10.132.251.197");
    server.sin_port = htons( 80 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
    //accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    puts("Connection accepted");
     
    uint64_t  end = get_tsc() + eal_tsc_resolution_hz;

    //Receive a message from client
    while( (read_size = recv(client_sock , client_message , 2000 , MSG_DONTWAIT )) > 0 )
    {
        if (read_size > 0) 
        {
            total_rcvd += read_size;
        }

        if(get_tsc() >= end)
		{
			printf("%llu \n", total_rcvd);
			total_rcvd = 0;
			end = get_tsc() + eal_tsc_resolution_hz;
		}
    }
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
     
    return 0;
}
