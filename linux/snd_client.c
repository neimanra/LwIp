
/*
    C ECHO client example using sockets
*/
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <time.h>
#include <math.h>
 
uint64_t eal_tsc_resolution_hz = 0;
double log_1024;
const char * suffix_arr[] = {"","K","M","G"};

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

void
print_statistics(uint64_t bytes_received)
{
	double val, bits_received = bytes_received * 8;

	uint8_t order;
	uint32_t denominator;

	order = log(bits_received) / log_1024;
	denominator = 1 << (10 * order);
	val = bits_received / denominator;
	printf("Recv rate: %.3lf %sb/s \n\n", val, suffix_arr[order]);
}

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    unsigned char buffer[1400] , server_reply[2000];
    int rcvd;
    unsigned long long total_rcvd = 0;
	memset(buffer, 'A', 1400);

    log_1024 = log(1024);
    set_tsc_freq_from_clock();
    printf("CPU Hz:%llu\n\n", (unsigned long long)eal_tsc_resolution_hz);

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("192.168.56.102");
    server.sin_family = AF_INET;
    server.sin_port = htons( 5001 );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
    

    uint64_t  end = get_tsc() + eal_tsc_resolution_hz;

    //keep communicating with server
    while(1)
    {      
        //Send some data
        if( send(sock , buffer , 1300 , 0) < 0)
        {
            puts("Send failed");
            return 1;
        }
         
        rcvd = recv(sock , buffer , 1300 , MSG_DONTWAIT);

        if (rcvd > 0)
        {
            total_rcvd += rcvd;
        }

        else
        {
//        	puts("Disconnected from server\n");
//        	return 0;
        }

        if(get_tsc() >= end)
		{
        	print_statistics(total_rcvd);
			total_rcvd = 0;
			end = get_tsc() + eal_tsc_resolution_hz;
		}

    }
     
    close(sock);
    return 0;
}
