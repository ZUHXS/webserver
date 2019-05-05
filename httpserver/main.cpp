#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "http.h"


//char *USAGE = "Usage: ./webserver web/ --port 8000\n";
const char *workspace = "/Users/zuhxs/Downloads/web";

int server_port;
char *server_files_directory;
int server_fd;
#define MAX_BUF 4096


void handle_files_request(int fd) {
    HTTP *request = new HTTP(fd, workspace);
    printf("method: %s\nuri: %s\ncurrent file: %s\n", request->get_method(), request->get_uri(), server_files_directory);
    if (request->dynamic_info()){  // dynamic

    }
    else {
        FILE *file = fopen(request->get_ab_file_name(), "r");
        if (!file) {
            printf("dynamic\n");
            request->start_response(404);
        }
        else {
            printf("static\n");
            char *buf = (char *)malloc(MAX_BUF);
            request->start_response(200);
            request->http_send_header("Content-Type", request->http_get_mime_type());
            request->http_end_header();
            int readn;
            while((readn = fread(buf, 1, MAX_BUF, file)) > 0) {
                request->http_send_data(buf, readn);
            }
        }
    }
}

void serve_forever(int *socket_number) {
    struct sockaddr_in server_address, client_address;
    size_t client_address_length = sizeof(client_address);
    int client_socket_number;

    *socket_number = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == *socket_number) {
        perror("Failed to create the socket");
        exit(errno);
    }

    int socket_option = 1;
    if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) == -1) {
        perror("Failed to set socket options");
        exit(errno);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);

    if (bind(*socket_number, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to bind on socket");
        exit(errno);
    }

    if (listen(*socket_number, 1024) == -1) {
        perror("Failed to listen on socket");
        exit(errno);
    }

    std::cout << "Listening on port " << server_port << std::endl;


    while (1) {
        client_socket_number = accept(*socket_number,
                                      (struct sockaddr *) &client_address,
                                      (socklen_t *) &client_address_length);
        if (client_socket_number < 0) {
            perror("Error accepting socket");
            continue;
        }

        printf("Accepted connection from %s on port %d\n",
               inet_ntoa(client_address.sin_addr),
               client_address.sin_port);

        handle_files_request(client_socket_number);
        close(client_socket_number);

        printf("Accepted connection from %s on port %d\n",
               inet_ntoa(client_address.sin_addr),
               client_address.sin_port);
    }

    shutdown(*socket_number, SHUT_RDWR);
    close(*socket_number);

}

int main(int argc, char **argv) {
    server_port = 8000;
    server_files_directory = strdup("/Users/zuhxs/Downloads/web");
    serve_forever(&server_fd);
}
