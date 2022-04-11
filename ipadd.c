
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>




pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;


static char pnum[10] = "33333";
static char name[10] = "fun";
static char ppc[10] = "0x7ffffff";
static char pi[10] = "1.0";
static char sib[10] = "12";
static int np = 0;
static int s = 0;
static int snum = 0;
static char ip[20];





void PrintSocketAddress(const struct sockaddr *address, FILE *stream) {
  if (address == NULL || stream == NULL)
    return;
  void *numericAddress; 
  char addrBuffer[INET6_ADDRSTRLEN];
  in_port_t port; 
  switch (address->sa_family) {
  case AF_INET:
    numericAddress = &((struct sockaddr_in *) address)->sin_addr;
    port = ntohs(((struct sockaddr_in *) address)->sin_port);
    break;
  case AF_INET6:
    numericAddress = &((struct sockaddr_in6 *) address)->sin6_addr;
    port = ntohs(((struct sockaddr_in6 *) address)->sin6_port);
    break;
  default:
    fputs("[unknown type]", stream);  
    return;
  }
  if (inet_ntop(address->sa_family, numericAddress, addrBuffer,
      sizeof(addrBuffer)) == NULL)
    fputs("[invalid address]", stream); 
  else {
    fprintf(stream, "%s", addrBuffer);
    if (port != 0)              
      fprintf(stream, "-%u", port);
  }
}
bool SockAddrsEqual(const struct sockaddr *addr1, const struct sockaddr *addr2) {
  if (addr1 == NULL || addr2 == NULL)
    return addr1 == addr2;
  else if (addr1->sa_family != addr2->sa_family)
    return false;
  else if (addr1->sa_family == AF_INET) {
    struct sockaddr_in *ipv4Addr1 = (struct sockaddr_in *) addr1;
    struct sockaddr_in *ipv4Addr2 = (struct sockaddr_in *) addr2;
    return ipv4Addr1->sin_addr.s_addr == ipv4Addr2->sin_addr.s_addr
        && ipv4Addr1->sin_port == ipv4Addr2->sin_port;
  } else if (addr1->sa_family == AF_INET6) {
    struct sockaddr_in6 *ipv6Addr1 = (struct sockaddr_in6 *) addr1;
    struct sockaddr_in6 *ipv6Addr2 = (struct sockaddr_in6 *) addr2;
    return memcmp(&ipv6Addr1->sin6_addr, &ipv6Addr2->sin6_addr,
        sizeof(struct in6_addr)) == 0 && ipv6Addr1->sin6_port
        == ipv6Addr2->sin6_port;
  } else
    return false;

    //130.127.49.18 
}

void DieWithUserMessage(const char *msg, const char *detail) {
  fputs(msg, stderr);
  fputs(": ", stderr);
  fputs(detail, stderr);
  fputc('\n', stderr);
  exit(1);
}
void DieWithSystemMessage(const char *msg) {
  perror(msg);
  exit(1);
}







int UDPServer() {

  char *service = pnum; // First arg:  local port/service

    // Construct the server address structure
    struct addrinfo addrCriteria;                   // Criteria for address
    memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
    addrCriteria.ai_family = AF_UNSPEC;             // Any address family
    addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
    addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram socket
    addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP socket

    struct addrinfo *servAddr; // List of server addresses
    int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
    if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    // Create socket for incoming connections
    int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol);
    if (sock < 0)
    DieWithSystemMessage("socket() failed");


    // Bind to the local address
    if (bind(sock, servAddr->ai_addr, servAddr->ai_addrlen) < 0)
    DieWithSystemMessage("bind() failed");

    // Free address list allocated by getaddrinfo()
    freeaddrinfo(servAddr);

    for (;;) { // Run forever
    struct sockaddr_storage clntAddr; // Client address
    // Set Length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    // Block until receive message from a client
    char buffer[MAXSTRINGLENGTH]; // I/O buffer

    // Size of received message
    ssize_t numBytesRcvd = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
        (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (numBytesRcvd < 0)
      DieWithSystemMessage("recvfrom() failed");

    fputs("Handling client ", stdout);
    PrintSocketAddress((struct sockaddr *) &clntAddr, stdout);
    fputc('\n', stdout);


    // Send received datagram back to the client
    ssize_t numBytesSent = sendto(sock, buffer, numBytesRcvd, 0,
        (struct sockaddr *) &clntAddr, sizeof(clntAddr));
    if (numBytesSent < 0)
      DieWithSystemMessage("sendto() failed)");
    else if (numBytesSent != numBytesRcvd)
      DieWithUserMessage("sendto()", "sent unexpected number of bytes");
  }
  // NOT REACHED
}


void mywait(int timeInS)
{
    struct timespec ts;
    struct timeval now;
    int rt = 0;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeInS;
    pthread_mutex_lock(&fakeMutex);
    do 
    {
        rt = pthread_cond_timedwait(&fakeCond, &fakeMutex, &ts);
    }
    while (rt == 0);
    pthread_mutex_unlock(&fakeMutex);
}





void* sender(void* arg)
{

    //int newTime = (start_time + snum) * pi;
    mywait(3);

    char *server = ip;     // First arg: server address/name
    char *echoString = name; 
    // Second arg: word to echo
    size_t echoStringLen = strlen(echoString);
    if (echoStringLen > MAXSTRINGLENGTH) 
    // Check input length
    DieWithUserMessage(echoString, "string too long");

    // Third arg (optional): server port/service
    //char *servPort = (argc == 4) ? pnum : "echo";
  
    char *servPort =  pnum;
  
    // Tell the system what kind(s) of address info we want
    struct addrinfo addrCriteria;                   // Criteria for address match
    memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
    addrCriteria.ai_family = AF_UNSPEC;             // Any address family
    // For the following fields, a zero value means "don't care"
    addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
    addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

    // Get address(es)
    struct addrinfo *servAddr; // List of server addresses
    int rtnVal = getaddrinfo(server, servPort, &addrCriteria, &servAddr);
    if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    // Create a datagram/UDP socket
    int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol); // Socket descriptor for client
    if (sock < 0)
    DieWithSystemMessage("socket() failed");

    // Send the string to the server
    ssize_t numBytes = sendto(sock, echoString, echoStringLen, 0,
      servAddr->ai_addr, servAddr->ai_addrlen);
    if (numBytes < 0)
    DieWithSystemMessage("sendto() failed");
    else if (numBytes != echoStringLen)
    DieWithUserMessage("sendto() error", "sent unexpected number of bytes");

    freeaddrinfo(servAddr);

    //buffer[echoStringLen] = '\0';     // Null-terminate received data
    //printf("Received: %s\n", buffer); // Print the echoed string

    close(sock);
    exit(0);
}






void* reciever(void* arg)
{
    char *server = ip;     // First arg: server address/name
    char *echoString = name; 
    // Second arg: word to echo
    size_t echoStringLen = strlen(echoString);
    if (echoStringLen > MAXSTRINGLENGTH) 
    // Check input length
    DieWithUserMessage(echoString, "string too long");

    // Third arg (optional): server port/service
    //char *servPort = (argc == 4) ? pnum : "echo";
  
    char *servPort =  pnum;
  
    // Tell the system what kind(s) of address info we want
    struct addrinfo addrCriteria;                   // Criteria for address match
    memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
    addrCriteria.ai_family = AF_UNSPEC;             // Any address family
    // For the following fields, a zero value means "don't care"
    addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
    addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

    // Get address(es)
    struct addrinfo *servAddr; // List of server addresses
    int rtnVal = getaddrinfo(server, servPort, &addrCriteria, &servAddr);
    if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));


    // Create a datagram/UDP socket
    int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol); // Socket descriptor for client
    if (sock < 0)
    DieWithSystemMessage("socket() failed");


    // Receive a response
    struct sockaddr_storage fromAddr; // Source address of server
    // Set length of from address structure (in-out parameter)
    socklen_t fromAddrLen = sizeof(fromAddr);
    char buffer[MAXSTRINGLENGTH + 1]; // I/O buffer
    ssize_t  numBytes = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
      (struct sockaddr *) &fromAddr, &fromAddrLen);
      printf("%ld\n",numBytes);
    if (numBytes < 0)
        DieWithSystemMessage("recvfrom() failed");
    else if (numBytes != echoStringLen)
        DieWithUserMessage("recvfrom() error", "received unexpected number of bytes");

    // Verify reception from expected source
    if (!SockAddrsEqual(servAddr->ai_addr, (struct sockaddr *) &fromAddr))
    DieWithUserMessage("recvfrom()", "received a packet from unknown source");

    freeaddrinfo(servAddr);

    buffer[echoStringLen] = '\0';     // Null-terminate received data
    printf("Received: %s\n", buffer); // Print the echoed string

    close(sock);
    exit(0);
}



int main(int argc, char* argv[])
{

    int opt;
      
    // put ':' in the starting of the
    // string so that program can 
    //distinguish between '?' and ':' 
    while((opt = getopt(argc, argv, "c:i:p:s:n:S")) != -1) 
    { 
        switch(opt) 
        { 
            case 'c':
                printf("ping-packet-count: %s\n",optarg);
                strcpy(ppc, optarg);
                break; 
            case 'i':
                printf("ping-interval: %s\n",optarg); 
                strcpy(pi,optarg);
                break;
            case 'p': 
                printf("port number: %s\n", optarg); 
                strcpy(pnum, optarg);
                break; 
            case 's': 
                printf("size in bytes: %s\n", optarg); 
                strcpy(sib,optarg);
                break; 
            case 'n': 
                printf("no print\n"); 
                np = 1;
                break; 
            case 'S': 
                s = 1;
                break; 
            case ':':
                printf("option needs a value\n");
                break;
            case '?': //used for some unknown options
                printf("unknown option: %c\n", optopt);
                break;
        } 
    } 
    // optind is for the extra arguments
    // which are not parsed
    for(; optind < argc; optind++)
    {     
        printf("IP: %s\n", argv[optind]); 
        strcpy(ip, argv[optind]);
    }


    if(s == 1){ UDPServer(pnum);}

    else
    {
        pthread_t send;
        pthread_t recv;
        void *ret;

        for(int i = 0; i < atoi(ppc); i++)
        {
            pthread_create(&send, NULL, sender, NULL);
            snum++;
        }
    

         
    }
    return 0;
}