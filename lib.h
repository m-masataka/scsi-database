#ifndef C11CODEREVIEW_LIB_H
#define C11CODEREVIEW_LIB_H

typedef enum Method {UNSUPPORTED, GET, POST, PUT, DELETE, HEAD} Method;

typedef struct Header {
    char *name;
    char *value;
    struct Header *next;
} Header;

typedef struct Request {
    enum Method method;
    char *url;
    char *version;
    struct Header *headers;
    char *body;
    size_t body_len;
} Request;


struct Request *parse_request(const char *raw);
void free_header(struct Header *h);
void free_request(struct Request *req);

#endif //C11CODEREVIEW_LIB_H
