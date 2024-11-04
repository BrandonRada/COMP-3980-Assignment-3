//
// Created by Brandon Rada on 10/10/24.
//

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define PORT 8080
#define MIN_ARGS 5

void usage(const char *program_name, int exit_code, const char *message) __attribute__((noreturn));
void parse_arguments(int argc, char *argv[], const char **ip_address, char **filter, char **string);

int main(int argc, char *argv[])
{
    char              *filter = NULL;
    char              *string = NULL;
    const char        *ip_address;
    int                sock;
    char               request[BUFFER_SIZE];
    struct sockaddr_in serv_addr;
    char               response[BUFFER_SIZE];
    ssize_t            bytes_read;

    parse_arguments(argc, argv, &ip_address, &filter, &string);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Socket created successfully.\n");
    }
    // Assign Ip and port
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip_address);
    serv_addr.sin_port        = htons(PORT);

    // connect

    if(inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Connected to sever...\n");
    }

    snprintf(request, sizeof(request), "%s:%s", filter, string);
    send(sock, request, strlen(request), 0);

    bytes_read = read(sock, response, sizeof(response) - 1);
    if(bytes_read > 0)
    {
        response[bytes_read] = '\0';
        printf("Response from server: %s\n", response);
    }
    else
    {
        perror("Failed to read response");
    }

    close(sock);
    return 0;
}

_Noreturn void usage(const char *program_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s <ip_address> -f <upper|lower|null> -s <string>\n", program_name);
    exit(exit_code);
}

void parse_arguments(int argc, char *argv[], const char **ip_address, char **filter, char **string)
{
    int opt;
    opterr = 0;

    if(argc < MIN_ARGS)    // Minimum argument check: program, ip, -f, filter, -s, string
    {
        usage(argv[0], EXIT_FAILURE, "Incorrect number of arguments.");
    }

    *ip_address = argv[1];    // Set the IP address as the first positional argument

    *filter = NULL;
    *string = NULL;

    while((opt = getopt(argc, argv, "f:s:")) != -1)
    {
        switch(opt)
        {
            case 'f':
                *filter = optarg;
                break;
            case 's':
                *string = optarg;
                break;
            default:
                usage(argv[0], EXIT_FAILURE, "Invalid option. Use -f for filter and -s for string.");
        }
    }

    // Check if filter and string arguments are provided
    if(*filter == NULL)
    {
        usage(argv[0], EXIT_FAILURE, "Error: Filter cannot be empty. Please specify -f <upper|lower|null>.");
    }

    if(*string == NULL)
    {
        usage(argv[0], EXIT_FAILURE, "Error: String input cannot be empty. Please specify -s <string>.");
    }

    // Validate the filter value
    if(strcmp(*filter, "upper") != 0 && strcmp(*filter, "lower") != 0 && strcmp(*filter, "null") != 0)
    {
        usage(argv[0], EXIT_FAILURE, "Invalid filter type. Please specify -f <upper|lower|null>.");
    }
}
