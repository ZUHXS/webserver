//
// Created by zuhxs on 2019/4/30.
//

#include "http.h"

HTTP::HTTP(int des_fd, const char *workspace) {
    this->method = nullptr;
    this->uri = nullptr;
    this->version = nullptr;
    this->ab_file_name = nullptr;
    this->file_name = nullptr;
    this->cgiargs = nullptr;
    this->cgi_content = nullptr;
    this->contype = nullptr;
    this->conlength = nullptr;
    this->if_dynamic = 0;
    this->workspace = strdup(workspace);

    this->fd = des_fd;
    char *read_buffer = (char *)malloc(LIBHTTP_REQUEST_MAX_SIZE + 1);
    if (!read_buffer)
        this->fatal_error("malloc failed");
    int bytes_read = read(fd, read_buffer, LIBHTTP_REQUEST_MAX_SIZE);
    read_buffer[bytes_read] = 0;
    this->cgi_content = read_buffer;

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
    if (read_size == 1 && *start_p == '/') {  // jump to index.html
        this->uri = strdup("/index.html");
    }
    else {
        this->uri = (char *) malloc(read_size + 1);
        memcpy(this->uri, start_p, read_size);
        this->uri[read_size] = 0;
    }
    //printf("uri: %s\n", this->uri);

    // parse the path
    this->parse_uri();

    // HTTP version & the request line .*
    start_p = end_p;
    while (*end_p != '\0' && *end_p != '\r')
        end_p++;
    if (*end_p != '\r')
        this->fatal_error("Error parsing when reading the line");
    read_size = end_p - start_p;
    if (read_size == 0){
        this->fatal_error("Error parsing when getting version");
    }
    this->version = (char *)malloc(read_size+1);
    memcpy(this->version, start_p, read_size);
    this->version[read_size] = 0;

    // parse conlength and contype
    if (this->if_dynamic) {
        end_p += 2;  // -> '\n', -> next line
        for ( ; ; ) {
            start_p = end_p;
            while (*end_p != '\r' && *end_p != '\0') {
                end_p++;
            }
            if (*end_p != '\r')
                this->fatal_error("Error parsing arguments in the packet");
            read_size = end_p - start_p;
            if (read_size == 0)
                break;   // end of header
            char *temp = (char *)malloc(read_size+1);
            memcpy(temp, start_p, read_size);
            temp[read_size] = 0;
            //printf("everyline: %s\n", temp);
            if (strstr(temp, "Content-Type: ") == temp) {
                this->contype = strdup(&temp[strlen("Content-Type: ")]);
                //printf("Con type: %s\n", this->contype);
            }
            if (strstr(temp, "Content-Length: ") == temp) {
                this->conlength = strdup(&temp[strlen("Content-Length: ")]);
                //printf("Con length: %s\n", this->contype);
            }
            end_p += 2;
        }
        //printf("the overall buf is: %s\n", read_buffer);

        // points to next line
        end_p += 2;
        start_p = end_p;

        //printf("remaining: %s\n", start_p);
        this->cgi_content = strdup(start_p);
        //printf("cgi content is %s\n", this->cgi_content);
    }
}

char* HTTP::get_method() {
    return this->method;
}

char *HTTP::get_uri() {
    return this->uri;
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
    char *file_extension = strrchr(this->uri, '.');
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

// test case: login.php?name=aaa, login.php.text, login.php
void HTTP::parse_uri() {
    int length = strlen(this->uri);
    char *temp_uri = (char *)malloc(length);
    strcpy(temp_uri, this->uri);

    char *temp_test = strstr(temp_uri, ".php");
    if (!temp_test) {  // static
        this->if_dynamic = 0;
        this->file_name = strdup(temp_uri);
        int ab_file_name_length = strlen(this->file_name) + strlen(this->workspace) + 1;
        this->ab_file_name = (char *)malloc(ab_file_name_length);
        strcpy(this->ab_file_name, workspace);
        strcat(this->ab_file_name, file_name);
    }
    else {
        this->if_dynamic = 1;
        char *p = strchr(temp_uri, '?');
        if (p) {  // have the argument in the uri
            strcpy(this->cgiargs, p+1);
            *p = '\0';
        }
        this->file_name = strdup(temp_uri);
        int ab_file_name_length = strlen(this->file_name) + strlen(this->workspace) + 1;
        this->ab_file_name = (char *)malloc(ab_file_name_length);
        strcpy(this->ab_file_name, workspace);
        strcat(this->ab_file_name, file_name);
    }
    free(temp_uri);
}

bool HTTP::dynamic_info() {
    return this->if_dynamic;
}

char* HTTP::get_ab_file_name() {
    return this->ab_file_name;
}

char* HTTP::get_file_name() {
    return this->file_name;
}

char* HTTP::get_cgiargs() {
    return this->cgiargs;
}

char* HTTP::get_contype() {
    return this->contype;
}

char *HTTP::get_conlength() {
    return this->conlength;
}

char *HTTP::get_cgi_content(){
    return this->cgi_content;
}