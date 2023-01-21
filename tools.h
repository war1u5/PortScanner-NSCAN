#pragma once
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>

#include <netdb.h> //defines the hostent structure, aici folosit pentru DNS

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h> //pt sockets

#include <time.h>

#include <unistd.h>

// initSock()
int initSocket()
{
    int sockfd; // socket descriptor

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //  //creare socket
                                              // domain: Specifies the communications domain in which a socket is to be created
                                              //->AF_INET: address family that is used to designate the type of addresses that your socket can communicate with (in this case, Internet Protocol v4 addresses)
    // type: Specifies the type of socket to be created.
    //-> SOCK_STREAM: Provides sequenced, reliable, bidirectional, connection-mode byte streams, and may provide a transmission mechanism for out-of-band data.
    // protocol: Specifies a particular protocol to be used with the socket
    //->Specifying a protocol of 0 causes socket() to use an unspecified default protocol appropriate for the requested socket type.

    if (sockfd < 0)
    {
        printf("ERROR opening socket");
        exit(1);
    }

    return sockfd;
}

void wrongCall()
{
    printf("Usage: nscan [Option/IP] {Port} {IP}...\nTry 'nscan --help' for more information.\n");
    exit(1);
}

// verifica daca format-ul adresei IP este corect
int isValidIpAddress(const char *ipAddress)
{
    // also check for number of '.' to be sure they are 4, then run the following code lines
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr)); // inet_pton - convert IPv4 and IPv6 addresses from text to binary form
                                                                //  returns 1 on success (network address was successfully converted).
                                                                //  0 is returned if src does not contain a character string representing a valid network address in the specified address family.
                                                                //  If af does not contain a valid address family, -1 is returned and errno is set to EAFNOSUPPORT.

    return result != 0;
}

// void verifyConnection(int sockfd, struct sockaddr_in serv_addr, int port_no)
// {
//     if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
//     {
//         printf("Port %d is closed\n", port_no);
//     }
//     else
//     {
//         printf("Port %d is active\n", port_no);
//     }
// }

// printeaza fisiere dupa caz
void myprint(const char *filename)
{
    FILE *infile;
    char *buffer;
    long numbytes;

    infile = fopen(filename, "r");

    if (infile == NULL)
        exit(1);

    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);

    fseek(infile, 0L, SEEK_SET);
    buffer = (char *)calloc(numbytes, sizeof(char));

    if (buffer == NULL)
        exit(1);

    fread(buffer, sizeof(char), numbytes, infile);
    fclose(infile);

    printf("%s", buffer);

    free(buffer);

    printf("\n");
}

// todo: ip to domain_name func() for initializing struct hostent *server
struct hostent *rev_dns_convert(struct in_addr address)
{
    struct hostent *server;
    server = gethostbyaddr(&address, sizeof(address), AF_INET);

    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    printf("Nscan scan report for %s\n", server->h_name);

    return server;
}

// domain_name to ip for initializing struct hostent *server
struct hostent *dns_convert(const char *address)
{
    struct hostent *server;
    server = gethostbyname(address);

    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    return server;
}

// domain name to ip func for printing an ip address given a domain name -> used in printing ip in tcp_connect_dns
char *dns_lookup(const char *addr_host)
{
    struct hostent *host_entity;
    char *ip = (char *)malloc(NI_MAXHOST * sizeof(char));
    int i;

    if ((host_entity = gethostbyname(addr_host)) == NULL)
    {
        printf("no such host\n");
        exit(1);
    }

    // filling up address structure
    strcpy(ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr));
    printf("IP: %s\n", ip);

    return ip;
}

void printOpenPort(int portno)
{
    struct servent *serv_name = getservbyport(htons(portno), NULL);
    if (serv_name == NULL)
    {
        printf("%d\topen\tunknown\n", portno);
    }
    else
    {
        char *name = strdup(serv_name->s_name);
        printf("%d\topen\t%s\n", portno, name);
    }
}

void connectOnPort(int portno, int sockfd, struct sockaddr_in serv_addr)
{
    fd_set fdset;
    struct timeval tv;

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    tv.tv_sec = 0;
    tv.tv_usec = 175000; // 0.175 sec timeout

    if (select(sockfd + 1, NULL, &fdset, NULL, &tv) == 1)
    {
        int so_error;
        socklen_t len = sizeof(so_error);

        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

        if (so_error == 0)
        {
            printOpenPort(portno);
        }
    }

    close(sockfd);
}

void printExecutionTime(struct timespec start_time)
{
    double program_duration;
    struct timespec finish_time;

    // clock_t end = clock();
    // double exec_time = (double)(end - start) / CLOCKS_PER_SEC;
    // printf("\nNscan done! Total scanning time: %fs\n", exec_time);

    clock_gettime(CLOCK_MONOTONIC, &finish_time);
    program_duration = (finish_time.tv_sec - start_time.tv_sec);
    program_duration += (finish_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    int hours_duration = program_duration / 3600;
    int mins_duration = (int)(program_duration / 60) % 60;
    double secs_duration = fmod(program_duration, 60);

    printf("Scan duration    : %d hour(s) %d min(s) %.05lf sec(s)\n", hours_duration, mins_duration, secs_duration);
}