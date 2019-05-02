//
// Created by zuhxs on 2019/4/30.
//

#include "http.h"

HTTP::HTTP(int des_fd) {
    this->fd = des_fd;
    char *read_buffer = (char *)malloc(LIBHTTP_REQUEST_MAX_SIZE + 1);
    if (!read_buffer)
        this->fatal_error("malloc failed");
    int bytes_read = read(fd, read_buffer, LIBHTTP_REQUEST_MAX_SIZE);
    read_buffer[bytes_read] = 0;

    char *start_p, *end_p;
    int read_size;

    // method: [A-z]*
    start_p = end_p = read_buffer;
    while (*end_p >= 'A' && *end_p <= 'Z')
        end_p++;
    read_size = end_p - start_p;
    if (read_size == 0)
        this->fatal_error("Error parsing when reading method");
    this->method = (char *)malloc(read_size + 1);
    memcpy(this->method, start_p, read_size);
    this->method[read_size] = 0;

    // space character
    start_p = end_p;
    if (*end_p != ' ')
        this->fatal_error("Error parsing when getting space");
    end_p++;

    // path: [^ \n]*
    start_p = end_p;
    while (*end_p != 0 && *end_p != ' ' && *end_p != '\n')
        end_p++;
    read_size = end_p - start_p;
    if (read_size == 0)
        this->fatal_error("Error parsing when getting path");
    this->path = (char *)malloc(read_size+1);
    memcpy(this->path, start_p, read_size);
    this->path[read_size] = 0;
    if (!strcmp(this->path, "/")) {  // use default path
        free(this->path);
        this->path = strdup("/index.html");
    }

    // HTTP version & the request line .*
    start_p = end_p;
    while (*end_p != '\0' && *end_p != '\n')
        end_p++;
    if (*end_p != '\n')
        this->fatal_error("Error parsing when reading the line");

    free(read_buffer);
}

char* HTTP::get_method() {
    return this->method;
}

char *HTTP::get_path() {
    return this->path;
}

void HTTP::fatal_error(const char *message) const {
    fprintf(stderr, "%s\n", message);
    exit(ENOBUFS);
}

void HTTP::http_send_data(char *data, int size) {
    int bytes_send;
    while (size > 0){
        bytes_send = write(this->fd, data, size);  //  may use several segment to send the data
        if (bytes_send < 0) {
            this->fatal_error("Error when sending data.");
        }
        size -= bytes_send;
        data += bytes_send;
    }
}

void HTTP::http_end_header() {
    dprintf(fd, "\r\n");
}

const char * HTTP::http_get_mime_type() {
    char *file_extension = strrchr(this->path, '.');
    if (!file_extension) {
        return "text/plain";
    }
    if (strcmp(file_extension, ".html") == 0 || strcmp(file_extension, ".htm") == 0) {
        return "text/html";
    } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(file_extension, ".png") == 0) {
        return "image/png";
    } else if (strcmp(file_extension, ".css") == 0) {
        return "text/css";
    } else if (strcmp(file_extension, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(file_extension, ".pdf") == 0) {
        return "application/pdf";
    } else {
        return "text/plain";
    }
}

const char* HTTP::get_response_message(int status_code) {
    switch (status_code) {
        case 100:
            return "Continue";
        case 200:
            return "OK";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 304:
            return "Not Modified";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        default:
            return "Internal Server Error";
    }
}

void HTTP::start_response(int status_code) {
    dprintf(this->fd, "HTTP/1.0 %d %s\r\n", status_code, this->get_response_message(status_code));
}

void HTTP::http_send_header(const char *key, const char *value) {
    dprintf(this->fd, "%s: %s\r\n", key, value);
}