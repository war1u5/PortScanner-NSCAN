#pragma once
#include "tools.h"

void tcp_file(const char *filename)
{
    FILE *f = fopen(filename, "r");

    if (f == NULL)
    {
        printf("Error opening the file\n");
        exit(-1);
    }

    printf("Reading from: %s\n", filename);
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
                printf("\n%s: no such host!\n", addr);
                continue;
            }
            else
            {
                //filling up address structure
                strcpy(ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr));
                tcp_all(ip);
            }
        }
    }

    fclose(f);
}