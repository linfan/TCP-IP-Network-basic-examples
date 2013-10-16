#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    struct servent* server_info;
    int i, j;
    for(i=0; i < 65536; i++)
    {
        server_info = getservbyport(i, NULL);
        // Print server_info
        if(server_info != NULL)
        {
            printf("server_info->s_name: %s\n", server_info->s_name);
            for(j=0; server_info->s_aliases[j] != NULL; j++)
            {
                printf("server_info->s_aliases: %s\n", server_info->s_aliases[j]);
            }
            printf("server_info->s_port: %u\n", ntohs((unsigned short) server_info->s_port));
            printf("server_info->s_prot: %s\n", server_info->s_proto);
            printf("\n");
        }

    }
}
