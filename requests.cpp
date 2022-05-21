#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.hpp"
#include "requests.hpp"   

char *compute_get_request(const char *host,
                        const char *url,
						const char *cookies,
                        const char *auth,
                        const char *bookId)
{
    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(LINELEN, sizeof(char));

    if (bookId)
    {
        sprintf(line, "GET %s/%s HTTP/1.1", url, bookId);
    }
    else
    {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    

    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    if (cookies != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: %s", cookies);
        compute_message(message, line);
    }
    if (auth != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "%s%s", AUTH, auth);
        compute_message(message, line);
    }
    
    // Step 4: add final new line
    compute_message(message, "");
    free(line);
    return message;
}

char *compute_delete_request(const char *host,
                            const char *url,
                            const char *cookies,
                            const char *auth,
                            const char *bookId)
{
    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(LINELEN, sizeof(char));

    if (bookId)
    {
        sprintf(line, "DELETE %s/%s HTTP/1.1", url, bookId);
    }
    else
    {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }
    

    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    if (cookies != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: %s", cookies);
        compute_message(message, line);
    }
    if (auth != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "%s%s", AUTH, auth);
        compute_message(message, line);
    }
    
    // Step 4: add final new line
    compute_message(message, "");
    free(line);
    return message;
}


char *compute_post_request(const char *host,
                        const char *url,
                        const char* content_type,
                        const char *body_data,
						char* cookies,
                        const char *auth)                       
{
    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Content-type: %s", content_type);
    compute_message(message, line);
    sprintf(line, "Content-length: %u", (int) (strlen(body_data)));
    compute_message(message, line);
    // Step 4 (optional): add cookies
    if (cookies != NULL) {
       
    }
    // Step 5: add new line at end of header
    if (auth != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "%s%s", AUTH, auth);
        compute_message(message, line);
    }
    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, "");

    compute_message(message, body_data);
    free(line);
    return message;
}
