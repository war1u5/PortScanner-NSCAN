#include "tools.h"

unsigned short check_sum(unsigned short *, int);
const char *dotted_quad(const struct in_addr *);
void *receive_ack(void *);
void process_packet(unsigned char *, int, char *);
void get_local_ip(char *);
void err_exit(char *, ...);
void prepare_datagram(char *, const char *, struct iphdr *, struct tcphdr *);

struct pseudo_header // Needed for checksum calculation
{
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short tcp_length;

    struct tcphdr tcp;
};

struct in_addr dest_ip;

// Initialize the datagram packet
void prepare_datagram(char *datagram, const char *source_ip, struct iphdr *iph, struct tcphdr *tcph)
{
    memset(datagram, 0, 4096);

    // Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct ip) + sizeof(struct tcphdr);
    iph->id = htons(46156);       // Id of this packet
    iph->frag_off = htons(16384); // translates a short integer from host byte order to network byte order
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;                    // Set to 0 before calculating checksum
    iph->saddr = inet_addr(source_ip); // Spoof the source ip address
    iph->daddr = dest_ip.s_addr;
    iph->check = check_sum((unsigned short *)datagram, iph->tot_len >> 1);

    // TCP Header
    tcph->source = htons(46156); // Source Port
    tcph->dest = htons(80);
    tcph->seq = htonl(1105024978);
    tcph->ack_seq = 0;
    tcph->doff = sizeof(struct tcphdr) / 4; // Size of tcp header
    tcph->fin = 0;
    tcph->syn = 1; // tcp syn
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->ack = 0;
    tcph->urg = 0;
    tcph->window = htons(14600); // Maximum allowed window size
    tcph->check = 0;             // If you set a checksum to zero, your kernel's IP stack should fill in the correct checksum during transmission
    tcph->urg_ptr = 0;
}

/*
  Format the IPv4 address in dotted quad notation, using a static buffer.
 */
const char *dotted_quad(const struct in_addr *addr)
{
    static char buf[INET_ADDRSTRLEN];

    return inet_ntop(AF_INET, addr, buf, sizeof buf); // convert IPv4 and IPv6 addresses from binary to text form
}

/*
  Exit the program with EXIT_FAILURE code
 */
void err_exit(char *fmt, ...)
{
    va_list ap;
    char buff[4096];

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);

    fflush(stdout);
    fputs(buff, stderr);
    fflush(stderr);

    exit(EXIT_FAILURE);
}

/*
  Method to sniff incoming packets and look for Ack replies
*/
int start_sniffer()
{
    int sock_raw;

    socklen_t saddr_size, data_size;
    struct sockaddr_in saddr;

    unsigned char *buffer = (unsigned char *)malloc(65536);

    // Create a raw socket that shall sniff
    sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock_raw < 0)
    {
        printf("Socket Error\n");
        fflush(stdout);
        return 1;
    }

    saddr_size = sizeof(saddr);

    // Receive a packet
    data_size = recvfrom(sock_raw, buffer, 65536, 0, (struct sockaddr *)&saddr, &saddr_size);

    if (data_size < 0)
    {
        printf("Recvfrom error, failed to get packets\n");
        fflush(stdout);
        return 1;
    }

    process_packet(buffer, data_size, inet_ntoa(saddr.sin_addr));
    close(sock_raw);

    return 0;
}

/*
  Method to sniff incoming packets and look for Ack replies
*/
void *receive_ack(void *ptr)
{
    start_sniffer();

    return NULL;
}

/*
  Method to process incoming packets and look for Ack replies
*/
void process_packet(unsigned char *buffer, int size, char *source_ip)
{
    struct iphdr *iph = (struct iphdr *)buffer; // IP Header part of this packet
    struct sockaddr_in source, dest;
    unsigned short iphdrlen;

    if (iph->protocol == 6) // iph->protocol == IPPROTO_TCP
    {
        struct iphdr *iph = (struct iphdr *)buffer;
        iphdrlen = iph->ihl * 4;

        struct tcphdr *tcph = (struct tcphdr *)(buffer + iphdrlen);

        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = iph->saddr;

        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = iph->daddr;

        if (tcph->syn == 1 && tcph->ack == 1 && source.sin_addr.s_addr == dest_ip.s_addr)
        {
            char source_host[NI_MAXHOST];

            printOpenPort(ntohs(tcph->source));

            fflush(stdout);
        }
    }
}

/*
 Checksums - IP and TCP
 */
unsigned short check_sum(unsigned short *ptr, int nbytes)
{
    register long sum;
    register short answer;
    unsigned short oddbyte;

    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1)
    {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;

    return answer;
}

/*
 Get source IP of the system running this program
 */
void get_local_ip(char *buffer)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    const char *kGoogleDnsIp = "8.8.8.8";
    int dns_port = 53;

    struct sockaddr_in serv;

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(dns_port);

    if (connect(sock, (const struct sockaddr *)&serv, sizeof(serv)) != 0)
        err_exit("Failed to get local IP\n");

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);

    if (getsockname(sock, (struct sockaddr *)&name, &namelen) != 0)
        err_exit("Failed to get local IP");

    inet_ntop(AF_INET, &name.sin_addr, buffer, INET6_ADDRSTRLEN);

    close(sock);
}