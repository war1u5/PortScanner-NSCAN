#define CRT_SECURE_NO_WARNINGS
#include "tools.h"
#include "tcp_connect_rev_dns.h"
#include "tcp_connect_dns.h"
#include "tcp_all.h"
#include "syn_scan.h"
#include "myping.h"

// todo:
// check if command is wrong for argv[1]

int main(int argc, char *argv[])
{

    // program side

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    if (argc == 1) // apel incorect al programului => mini descriere a modului in care trebuie apelat programul
    {
        myprint("_ascii_art.txt");
        printf("\n\nPress any key to continue...");
        getchar();
    }
    else if (argc == 2) // doar pentru cazul cu --help
    {
        if (strcmp(argv[1], "--help") == 0)
        {
            system("man ./_man.1");
            exit(0);
        }
        else
        {
            wrongCall();
            exit(-1);
        }
    }
    else if (argc == 3) // to do scan all ports for given domain name
    {
        // tcp_all pt given ip
        if (strcmp(argv[1], "--scan") == 0)
        {
            if (isValidIpAddress(argv[2]))
            {
                tcp_all(argv[2]);
            }
            else
            {
                // struct hostent *host_entity;
                char *ip = (char *)malloc(NI_MAXHOST * sizeof(char));
                ip = dns_lookup(argv[2]);
                tcp_all(ip);
            }

            printExecutionTime(start_time);
            exit(0);
        }
        else if (strcmp(argv[1], "--file") == 0)
        {
            // put --file into a header
            FILE *f = fopen(argv[2], "r");

            if (f == NULL)
            {
                printf("Error opening the file\n");
                exit(-1);
            }

            printf("Reading from: %s\n", argv[2]);
            char addr[60];

            while (fgets(addr, 60, f))
            {
                if (strchr(addr, '\n') != 0)
                {
                    addr[strlen(addr) - 1] = '\0';
                }

                if (isValidIpAddress(addr))
                {
                    tcp_all(addr);
                }
                else
                {
                    struct hostent *host_entity;
                    char *ip = (char *)malloc(NI_MAXHOST * sizeof(char));
                    int i;

                    if ((host_entity = gethostbyname(addr)) == NULL)
                    {
                        printf("no such host\n");
                        exit(-1);
                    }

                    // filling up address structure
                    strcpy(ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr));
                    tcp_all(ip);
                }
            }

            fclose(f);

            printExecutionTime(start_time);

            exit(0);
        }
        else if (strcmp(argv[1], "--ping") == 0)
        {
            // put --ping in a header file
            int sockfd;
            char *ip_addr, *reverse_hostname;
            struct sockaddr_in addr_con;
            int addrlen = sizeof(addr_con);
            char net_buf[NI_MAXHOST];

            ip_addr = dns_lookup_ping(argv[2], &addr_con);
            if (ip_addr == NULL)
            {
                printf("\nDNS lookup failed! Could not resolve hostname!\n");
                exit(-1);
            }

            reverse_hostname = reverse_dns_lookup(ip_addr);
            printf("\nTrying to connect to '%s' IP: %s\n", argv[2], ip_addr);
            printf("\nReverse Lookup domain: %s", reverse_hostname);

            // socket()
            sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

            if (sockfd < 0)
            {
                printf("\nSocket file descriptor not received!!\n");
                exit(-1);
            }
            else
            {
                printf("\nSocket file descriptor %d received\n", sockfd);
            }

            signal(SIGINT, intHandler);

            // send pings continuously
            send_ping(sockfd, &addr_con, reverse_hostname, ip_addr, argv[2]);

            if (isReachable == 1)
            {
                printf("\nThe destination is reachable!\n\n");
            }

            printExecutionTime(start_time);

            exit(0);
        }
        else if (strcmp(argv[1], "--syn") == 0)
        {
            printf("Nscan SYN scan report for %s\n", argv[2]);

            char *addr;

            if (!isValidIpAddress(argv[2])) // syn scan used with domain name
            {
                struct hostent *get_addr = gethostbyname(argv[2]);

                if (get_addr)
                {
                    addr = strdup(inet_ntoa(*(struct in_addr *)get_addr->h_addr));
                    printf("IP address for %s is %s\n\n", argv[2], addr);
                }
                else
                {
                    err_exit("Could not resolve domain name to ip!");
                }
            }
            else // syn scan used with ip address
            {
                addr = strdup(argv[2]);

                struct in_addr address;
                inet_aton(addr, &address);

                struct hostent *domain;
                domain = gethostbyaddr(&address, sizeof(address), AF_INET);

                if (domain == NULL)
                {
                    fprintf(stderr, "ERROR, no such host\n");
                    exit(1);
                }

                printf("Domain name for IP address %s is %s\n\n", addr, domain->h_name);
            }

            struct in_addr target_in_addr;
            inet_aton(argv[2], &target_in_addr);

            char source_ip[INET6_ADDRSTRLEN];
            get_local_ip(source_ip); // Get machine's local IP for IP header information in datagram packet

            int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP); // This is the main socket to send the SYN packet
            if (sockfd < 0)
                err_exit("Error creating socket. Error number: %d. Error message: %s\n", errno, strerror(errno));

            // Set IP_HDRINCL socket option to tell the kernel that headers are included in the packet
            int oneVal = 1;
            if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &oneVal, sizeof(oneVal)) < 0)
                err_exit("Error setting IP_HDRINCL. Error number: %d. Error message: %s\n", errno, strerror(errno));

            dest_ip.s_addr = inet_addr(dotted_quad(&target_in_addr)); // Current iteration's target host

            if (dest_ip.s_addr == -1)
                err_exit("Invalid address\n");

            char datagram[4096];
            struct iphdr *iph = (struct iphdr *)datagram;                          // IP header
            struct tcphdr *tcph = (struct tcphdr *)(datagram + sizeof(struct ip)); // TCP header

            prepare_datagram(datagram, source_ip, iph, tcph);

            pthread_t sniffer_thread;
            if (pthread_create(&sniffer_thread, NULL, receive_ack, NULL) < 0) // Thread to listen for just one SYN-ACK packet from any of the selected ports
                err_exit("Could not create sniffer thread. Error number: %d. Error message: %s\n", errno, strerror(errno));

            printf("\nPORT\tSTATE\tSERVICE\n");
            for (int port = 0; port <= 1024; port++) // Iterate all selected ports and send SYN packet all at once
            {
                struct sockaddr_in dest;
                struct pseudo_header psh;

                dest.sin_family = AF_INET;
                dest.sin_addr.s_addr = dest_ip.s_addr;

                tcph->dest = htons(port);
                tcph->check = 0;

                psh.source_address = inet_addr(source_ip);
                psh.dest_address = dest.sin_addr.s_addr;
                psh.placeholder = 0;
                psh.protocol = IPPROTO_TCP;
                psh.tcp_length = htons(sizeof(struct tcphdr));

                memcpy(&psh.tcp, tcph, sizeof(struct tcphdr));

                tcph->check = check_sum((unsigned short *)&psh, sizeof(struct pseudo_header));

                if (sendto(sockfd, datagram, sizeof(struct iphdr) + sizeof(struct tcphdr), 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
                    err_exit("Error sending syn packet. Error number: %d. Error message: %s\n", errno, strerror(errno));
            }

            pthread_join(sniffer_thread, NULL); // Will wait for the sniffer to receive a reply, host is considered closed if there aren't any

            target_in_addr.s_addr = htonl(ntohl(target_in_addr.s_addr) + 1);

            close(sockfd);

            printExecutionTime(start_time);
        }
        else
        {
            wrongCall();
            exit(-1);
        }
    }
    else if (argc == 4) // argv[2] = port si argv[3] = ip
    {
        clock_t start;
        if (strcmp(argv[1], "--port") == 0)
        {
            printf("nscan --port %d %s\n", atoi(argv[2]), argv[3]);

            if (atoi(argv[2]) == 0)
            {
                exit(-1);
            }

            if (isValidIpAddress(argv[3]))
            {
                tcp_connect_rev_dns(argv[3], atoi(argv[2]));
            }
            else
            {
                tcp_connect_dns(argv[3], atoi(argv[2]));
            }

            printExecutionTime(start_time);

            exit(0);
        }
        else
        {
            wrongCall();
            exit(-1);
        }
    }

    return 0;
}