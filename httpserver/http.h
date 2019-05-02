//
// Created by zuhxs on 2019/4/30.
//

#ifndef HTTPSERVER_HTTP_H
#define HTTPSERVER_HTTP_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LIBHTTP_REQUEST_MAX_SIZE 8192


class HTTP {
private:
    char *method;
    char *path;
    int fd;
public:
    HTTP(int des_fd);
    void fatal_error(const char *message) const;
    char *get_method();
    char *get_path();
    void http_send_header(const char *key, const char *value);
    void http_end_header();
    void http_send_string(char *data);
    void http_send_data(char *data, int size);
    const char *http_get_mime_type();
    void start_response(int status_code);
    const char *get_response_message(int status_code);
};



#endif //HTTPSERVER_HTTP_H
