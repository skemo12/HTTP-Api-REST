#ifndef _REQUESTS_
#define _REQUESTS_

#define AUTH "Authorization: Bearer "
// computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_get_request(const char *host,
                        const char *url,
						const char *cookies,
                        const char *auth,
                        const char *bookId);

// computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request(const char *host,
                        const char *url,
                        const char* content_type,
                        const char *body_data,
						char* cookies,
                        const char *auth);

char *compute_delete_request(const char *host,
                            const char *url,
                            const char *cookies,
                            const char *auth,
                            const char *bookId);
#endif
