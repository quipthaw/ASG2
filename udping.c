#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>



/*
Implementation requirements:
(0) You can use the getopt() function to parse the command line arguments. 
Use online manual to look up the arguments and use. 

(1) Your program MUST create two child threads. One will send pings and 
the other will receive them.

(2) The sender() thread MUST use the pthread_cond_timedwait() function to wait 
until it is time to send the next ping. The proper time to send is: 
    start_time + (seq# - 1) x ping-interval.

(3) The receiver thread MUST compute the round trip time of the ping, 
the number of pings received, the min, max, and sum of the round trip times.

(4) Round trip times must be computed to microsecond level accuracy using the \
following function:
         int clock_gettime(clockid_t clk_id, struct timespec *tp) where 
         clk_id is a macro set to CLOCK_REALTIME 

(5) The signal() function must be used to set up handler that will print the statistics lines and the exit(0) if the
user enters CTRL-C.

(6) You can use Donahoo’s UDPEchoClient.c and UDPEchoServer.c code as starter code.








Output notes:
    (1) command line options should be echoed to the stderr stream.
    (2) other output data should go to the stdout.
    (3) in the default mode one line should be printed for every ping 
        received by the client. The server should not print anything).  
        The printed line should contain (see example below):
            - The sequence number carried by the ping packet;
            - The number of bytes of application data;
            - The round trip time in milliseconds in the format shown.
    (4) The statistics lines should be printed after the number of pings 
        specified have been sent or the user terminates the client with a Ctrl-C.


    x$ ./udping -c 10 -s 300 -i 0.1 -p 33333 <srvr_ip_addr>
    Count         10
    Size                   300     /___________________ Goes to stderr
    Interval          0.100     \
    Port              33333
    Server_ip   <ip_addr>
    (Notice the right-justified alignment)
        1      300      0.104
        2      300      0.035
        3      300      0.027
        4      300      0.035
        5      300      0.025   /___________________ Goes to stdout
        6      300      0.026   \                      |
        7      300      0.025                          |
        8      300      0.050                          |
        9      300      0.025                          |
        10     300      0.025                          V

10 packets transmitted, 10 received, 0%  packet loss, time 1004 ms
rtt min/avg/max = 0.025/0.038/0.104 msec


If the -n option is used then printing of individual responses should be suppressed and you will print 10 
asterisks instead.

tux$ ./udping -c 100 -s 300 -i 0.01   -n  <srvr_ip_addr>
Count            100
Size             300
Interval     0.010
**********
100 packets transmitted, 100 received, 0%  packet loss, time 1003 ms
rtt min/avg/max = 0.016/0.026/0.108 msec




Example of use:
    udping -S -p 50000      (starts the server and binds it to port 5000)
    udping -i 0.1 -c 100 -s 200 -p 50000 <server_ip _address> 
    (send 100 pings of size 200 bytes at a rate of 10 pings per second to a 
    server running on host with specified IP address.
    Options can be entered in any order)

*/ 








#define MAXLINE 260
//#define MSG_CONFIRM "Confirm"

void server(unsigned short port)
{
    int cli2_sockfd = -1;
    char buffer_cli2[MAXLINE] = { 0 };
    char *hello2 = "Hello from client 2";
    struct sockaddr_in cli2_addr = { 0 }, client1_addr = { 0 };
    unsigned short clis_portno = port;

    // Creating socket file descriptor 
    if ((cli2_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information 
    cli2_addr.sin_family = AF_INET; // IPv4 
    cli2_addr.sin_addr.s_addr = INADDR_ANY;
    cli2_addr.sin_port = htons(clis_portno);

    // Bind the socket with the server address 
    if (bind(cli2_sockfd, (const struct sockaddr *)&cli2_addr,
        sizeof(cli2_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        int n2;
        socklen_t len2 = sizeof(client1_addr);
        if ((n2 = recvfrom(cli2_sockfd, (char *)buffer_cli2, MAXLINE,
            0, (struct sockaddr *) &client1_addr,
            &len2)) < 0)
        {
            perror("svr recvfrom");
            exit(EXIT_FAILURE);
        }

        buffer_cli2[n2] = '\0';
        printf("Client 1: %s\n", buffer_cli2);

        if (sendto(cli2_sockfd, (const char *)hello2, strlen(hello2),
            0, (const struct sockaddr *) &client1_addr,
            len2) < 0)
        {
            perror("svr sendto");
        }
        printf("Hello message sent.\n");

    }
}

void client(const char* hostname, unsigned short port)
{
    int client1_sockfd;
    char buffer[MAXLINE];
    char *hello1 = "Hello from client 1";
    struct sockaddr_in   client2_addr = { 0 };
    struct hostent *client_2 = NULL;
    unsigned short clis_portno = port;

    // Creating socket file descriptor 
    if ((client1_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if ((client_2 = gethostbyname(hostname)) == NULL)
    {
        perror("cli gethostbyname");
        exit(EXIT_FAILURE);
    }

    // Filling Client 2 information 
    client2_addr.sin_family = AF_INET;
    client2_addr.sin_port = htons(clis_portno);
    memcpy(&client2_addr.sin_addr, client_2->h_addr, 4);

    while (1)
    {
        int n1;


        if (sendto(client1_sockfd, (const char *)hello1, strlen(hello1),
            0, (const struct sockaddr *) &client2_addr,
            sizeof(client2_addr)) < 0)
        {
            perror("cli sendto");
            exit(EXIT_FAILURE);
        }

        printf("IN THREAD: Hello1 = %s\n", hello1);

        socklen_t len1 = sizeof(client2_addr);
        if ((n1 = recvfrom(client1_sockfd, (char *)buffer, MAXLINE,
            0, (struct sockaddr *) &client2_addr,
            &len1)) < 0)
        {
            perror("cli recvfrom");
        }
        else
        {
            buffer[n1] = '\0';
            printf("IN THREAD: Client 2 : %s\n", buffer);
        }
        sleep(1);
    }
}




int main(int argc, char* argv[])
{

    if ((argc > 1) && (strcmp(argv[1], "s") == 0))
    {
        printf("Running in server mode\n");
        server(9999);
    }
    else
    {
            printf("Running in client mode\n");
            client("localhost", 9999);
    }
    return 0;
}




































/*
int opt;

while((opt = getopt(argc, argv, ":if:lrx")) != -1) 
    { 
        switch(opt) 
        { 
            case 'i': 
                printf("option: ping-interval\n");
                break;
            case 'S': 
                printf("option: Server\n");
                break;
            case 'c': 
                printf("option: ping-packet-count\n");
                break;
            case 'p':
                printf("port number\n");
                break; 
            case 'r': 
                printf("option: %c\n", opt); 
                break; 
            case 'f': 
                printf("filename: %s\n", optarg); 
                break; 
            case ':': 
                printf("option needs a value\n"); 
                break; 
            case '?': 
                printf("unknown option: %c\n", optopt);
                break; 
        } 
    } 
      
    // optind is for the extra arguments
    // which are not parsed
    for(; optind < argc; optind++){     
        printf("extra arguments: %s\n", argv[optind]); 
    }
    */  
 

    //pthread_cond_timedwait();








