//
// Created by zuhxs on 2019/5/3.
//

#include "fastcgi.h"

int FASTCGI::request_id = 0;

FASTCGI::FASTCGI() {
    this->current_request_id = FASTCGI::request_id;
    FASTCGI::request_id += 1;
    // open a socket
    struct sockaddr_in serv_addr;

    this->CGI_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (this->CGI_sock == -1) {
        this->fatal_error("socket creation error.");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(FCGI_HOST);
    serv_addr.sin_port = htons(FCGI_PORT);

    if (connect(this->CGI_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
        this->fatal_error("error connection, please start php-fpm at port 9000.");
    }
}



void FASTCGI::fatal_error(const char *message) const {
    fprintf(stderr, "%s\n", message);
    exit(ENOBUFS);
}

void FASTCGI::request_cgi(HTTP *hp) {
    char *paname[] = {
            "SCRIPT_FILENAME",
            "SCRIPT_NAME",
            "REQUEST_METHOD",
            "REQUEST_URI",
            "QUERY_STRING",
            "CONTENT_TYPE",
            "CONTENT_LENGTH"
    };

    // send the header
    FCGI_BeginRequestRecord beginRecord;
    FCGI_Header *header = make_header(FCGI_BEGIN_REQUEST, sizeof(beginRecord.body), 0);
    beginRecord.header = *header;
    free(header);

    FCGI_BeginRequestBody *body = make_BeginRequestBody(FCGI_RESPONDER, 0);
    beginRecord.body = *body;
    free(body);

    // write the data to socket
    write(CGI_sock, &beginRecord, sizeof(beginRecord));

    // send the arguments
    if (hp->get_ab_file_name() && strlen(hp->get_ab_file_name()) > 0)
        this->sendParamsRecord(paname[0], strlen(paname[0]), hp->get_ab_file_name(), strlen(hp->get_ab_file_name()));
    if (hp->get_file_name() && strlen(hp->get_file_name()) > 0)
        this->sendParamsRecord(paname[1], strlen(paname[1]), hp->get_file_name(), strlen(hp->get_file_name()));
    if (hp->get_method() && strlen(hp->get_method()) > 0)
        this->sendParamsRecord(paname[2], strlen(paname[2]), hp->get_method(), strlen(hp->get_method()));
    if (hp->get_uri() && strlen(hp->get_uri()) > 0)
        this->sendParamsRecord(paname[3], strlen(paname[3]), hp->get_uri(), strlen(hp->get_uri()));
    if (hp->get_cgiargs() && strlen(hp->get_cgiargs()) > 0)
        this->sendParamsRecord(paname[4], strlen(paname[4]), hp->get_cgiargs(), strlen(hp->get_cgiargs()));
    if (hp->get_contype() && strlen(hp->get_contype()) > 0)
        this->sendParamsRecord(paname[5], strlen(paname[5]), hp->get_contype(), strlen(hp->get_contype()));
    if (hp->get_conlength() && strlen(hp->get_conlength()) > 0)
        this->sendParamsRecord(paname[6], strlen(paname[6]), hp->get_conlength(), strlen(hp->get_conlength()));

    // send an empty params
    this->sendEmptyParamsRecord();

    // send the buffer
    int l = atoi(hp->get_conlength());
    if (l > 0) {
        char *buf = (char *)malloc(l + 1);
        char *old = buf;
        int length = strlen(hp->get_cgi_content());
        memcpy(buf, hp->get_cgi_content(), length);
        buf[length] = '\0';

        int cl = l;
        int pl;
        while (l > 0) {
            char padding[8] = {0};
            if (l > FCGI_MAX_LENGTH) {
                cl = FCGI_MAX_LENGTH;
            }
            pl = (cl % 8) == 0 ? 0 : 8 - (cl % 8);

            // send header
            FCGI_Header *sinHeader = make_header(FCGI_STDIN, cl, pl);
            write(this->CGI_sock, (char *)sinHeader, FCGI_HEADER_LEN);
            //free(sinHeader);

            // send buffer
            write(this->CGI_sock, buf, cl);
            // send padding
            write(this->CGI_sock, padding, pl);

            l -= cl;
            buf += cl;
        }
        free(old);
    }

    // send empty params
    this->sendEmptyParamsRecord();

}

FCGI_Header* FASTCGI::make_header(unsigned char type, int contentLength, unsigned char paddingLength) {
    FCGI_Header *header = (FCGI_Header *)malloc(sizeof(FCGI_Header));
    header->version = FCGI_VERSION_1;
    header->type = type;
    header->requestIdB1 = (unsigned char ) ((this->current_request_id >> 8) & 0xff);
    header->requestIdB0 = (unsigned char) ((this->current_request_id) & 0xff);
    header->contentLengthB1 = (unsigned char) ((contentLength >> 8) & 0xff);
    header->contentLengthB0 = (unsigned char) ((contentLength) & 0xff);
    header->paddingLength = (unsigned char) paddingLength;
    header->reserved = 0;
    return header;
}

FCGI_BeginRequestBody* FASTCGI::make_BeginRequestBody(int role, int keepConn) {
    FCGI_BeginRequestBody *requestbody = (FCGI_BeginRequestBody *)malloc(sizeof(FCGI_BeginRequestBody));
    requestbody->roleB1 = (unsigned char) ((role >> 8) & 0xff);
    requestbody->roleB0 = (unsigned char) (role & 0xff);
    requestbody->flags = (unsigned char) ((keepConn) ? 1 : 0); // 1 for long connection
    for (int i = 0; i < 5; i++){
        requestbody->reserved[i] = 0;
    }
    return requestbody;
}

void FASTCGI::sendParamsRecord(char *name, int nlen, char *value, int vlen) {
    unsigned char *buf, *old;
    int ret, pl, cl = nlen + vlen;
    cl = (nlen < 128) ? ++cl : cl + 4;
    cl = (vlen < 128) ? ++cl : cl + 4;
    pl = (cl % 8) == 0 ? 0 : 8 - (cl % 8);
    old = buf = (unsigned char *)malloc(FCGI_HEADER_LEN + cl + pl);

    FCGI_Header *nvHeader = make_header(FCGI_PARAMS, cl, pl);
    memcpy(buf, (char *)nvHeader, FCGI_HEADER_LEN);
    free(nvHeader);
    buf = buf + FCGI_HEADER_LEN;

    if (nlen < 128) { // name长度小于128字节，用一个字节保存长度
        *buf++ = (unsigned char)nlen;
    } else { // 大于等于128用4个字节保存长度
        *buf++ = (unsigned char)((nlen >> 24) | 0x80);
        *buf++ = (unsigned char)(nlen >> 16);
        *buf++ = (unsigned char)(nlen >> 8);
        *buf++ = (unsigned char)nlen;
    }

    if (vlen < 128) { // value长度小于128字节，用一个字节保存长度
        *buf++ = (unsigned char)vlen;
    } else { // 大于等于128用4个字节保存长度
        *buf++ = (unsigned char)((vlen >> 24) | 0x80);
        *buf++ = (unsigned char)(vlen >> 16);
        *buf++ = (unsigned char)(vlen >> 8);
        *buf++ = (unsigned char)vlen;
    }

    memcpy(buf, name, nlen);
    buf = buf + nlen;
    memcpy(buf, value, vlen);

    write(this->CGI_sock, old, FCGI_HEADER_LEN + cl + pl);
    free(old);
}

void FASTCGI::sendEmptyParamsRecord() {
    FCGI_Header *emHeader = make_header(FCGI_PARAMS, 0, 0);
    write(this->CGI_sock, (char *)emHeader, FCGI_HEADER_LEN);
    free(emHeader);
}

char *FASTCGI::recvRecord() {
    FCGI_Header responHeader;
    FCGI_EndRequestBody endr;
    char *conBuf = nullptr;
    int buf[8], cl;
    int recv_id;  // compare with the request id
    int outlen = 0;

    while(read(this->CGI_sock, &responHeader, FCGI_HEADER_LEN)) {
        recv_id = (int)(responHeader.requestIdB1 << 8) + (int)(responHeader.requestIdB0);
        if (recv_id != this->current_request_id)
            continue;
        if (responHeader.type == FCGI_STDOUT) {
            cl = (int)(responHeader.contentLengthB1 << 8) + (int)(responHeader.contentLengthB0);
            int old_outlen = outlen;
            outlen += cl;
            conBuf = (char *)realloc(conBuf, outlen+1);
            memset(conBuf, '\0', outlen+1);
            read(CGI_sock, &conBuf[old_outlen], cl);
            if (responHeader.paddingLength > 0) {
                read(CGI_sock, buf, responHeader.paddingLength);
            }
        }
        else if (responHeader.type == FCGI_STDERR) {
            cl = (int)(responHeader.contentLengthB1 << 8) + (int)(responHeader.contentLengthB0);
            char *errlog = (char *)malloc(cl);
            read(CGI_sock, errlog, cl);
            if (responHeader.paddingLength > 0) {
                read(CGI_sock, buf, responHeader.paddingLength); // read buffer
            }
            fprintf(stderr, "%s", errlog);
            free(errlog);
        }
        else if (responHeader.type == FCGI_END_REQUEST) {
            read(CGI_sock, &endr, sizeof(FCGI_EndRequestBody));

            // send the data to client
            return conBuf;
        }
    }
    return conBuf;

}
