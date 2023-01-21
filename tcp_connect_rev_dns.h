#pragma once
#include "tools.h"

void tcp_connect_rev_dns(const char *addr, int port_no)
{
    struct in_addr address;
    struct sockaddr_in serv_addr; // structura care contine port + ip pt stabilirea conexiunii
    struct hostent *server;

    int sockfd = initSocket(); // socket descriptor

    // rev dns server
    inet_aton(addr, &address);
    server = rev_dns_convert(address);

    // initializarea structurii cu ajutorul careia verificam conexiunea
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port_no);

    // verifyConnection(sockfd, serv_addr, port_no);
   
    printf("\nPORT\tSTATE\tSERVICE\n");
    connectOnPort(port_no, sockfd, serv_addr);

    close(sockfd);
}