//
// Created by Brandon Rada on 10/10/24.
//

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MAX_PORT 65535
#define TEN 10

typedef char (*filter_function)(char);

// Filter functions

filter_function get_filter_function(const char *filter_name);

static char upper_filter(char c);
static char lower_filter(char c);
static char null_filter(char c);

void handle_client(int client_socket);
void handle_sigint(int sig);

// Signal flag for graceful shutdown
static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

_Noreturn void handle_sigint(int sig)
{
    (void)sig;
    exit_flag = 1;
    //    printf("\nServer is shutting down...\n");
    _exit(EXIT_SUCCESS);
}

// Main server function
int main(int argc, const char *argv[])
{
    const char        *ip_address;
    long               port;
    int                server_fd;
    struct sockaddr_in address;

    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <ip_address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ip_address = argv[1];
    port       = strtol(argv[2], NULL, TEN);
    if(port <= 0 || port > MAX_PORT)
    {
        fprintf(stderr, "Invalid port number outside of accepted range (0 - 65535)\n");

        exit(EXIT_FAILURE);
    }

    // Set up signal handling
    signal(SIGINT, handle_sigint);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port   = htons((uint16_t)port);

    if(inet_pton(AF_INET, ip_address, &address.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid address\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, TEN) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server running at %s:%ld\n", ip_address, port);

    while(!exit_flag)
    {
        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd < 0)
        {
            if(exit_flag)
            {
                break;
            }
            perror("Accept failed");
            continue;
        }

        if(fork() == 0)
        {
            close(server_fd);
            handle_client(client_fd);
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

// Handle each client request
void handle_client(int client_socket)
{
    char            buffer[BUFFER_SIZE];
    ssize_t         bytes_read;
    const char     *filter_type;
    char           *string;
    char           *saveptr;
    filter_function filter;

    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if(bytes_read <= 0)
    {
        perror("Read failed");
        return;
    }
    buffer[bytes_read] = '\0';

    filter_type = strtok_r(buffer, ":", &saveptr);
    string      = strtok_r(NULL, ":", &saveptr);
    if(!filter_type || !string)
    {
        fprintf(stderr, "Invalid request format\n");
        return;
    }

    filter = get_filter_function(filter_type);
    for(int i = 0; string[i]; i++)
    {
        string[i] = filter(string[i]);
    }
    write(client_socket, string, strlen(string));
}

static char upper_filter(char c)
{
    return (char)toupper(c);
}

static char lower_filter(char c)
{
    return (char)tolower(c);
}

static char null_filter(char c)
{
    return c;
}

filter_function get_filter_function(const char *filter_name)
{
    if (filter_name == NULL) {
        fprintf(stderr, "Error: filter_name is NULL.\n");
        return NULL; // Or handle it according to your application's logic
    }

    if(strcmp(filter_name, "upper") == 0)
    {
        return upper_filter;
    }
    if(strcmp(filter_name, "lower") == 0)
    {
        return lower_filter;
    }
    return null_filter;
}
