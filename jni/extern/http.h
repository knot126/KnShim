/*
------------------------------------------------------------------------------
          Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

http.h v2.0 - Basic HTTP protocol implementation over sockets, with optional
mbedTLS https support.

Modified by Knot126 for KnShim :3

Do this:
    #define HTTP_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.
*/

#ifndef http_hpp
#define http_hpp

#define _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS
#include <stddef.h> // for size_t
#include <stdint.h> // for uintptr_t

typedef enum http_status_t {
    HTTP_STATUS_PENDING,
    HTTP_STATUS_COMPLETED,
    HTTP_STATUS_FAILED,
} http_status_t;

typedef struct http_header_t {
    const char *name;
    const char *value;
} http_header_t;

typedef struct http_t {
    http_status_t status;
    int status_code;
    char const* reason_phrase;
    size_t num_headers;
    http_header_t *headers;
    size_t response_size;
    void* response_data;
} http_t;

#ifndef HTTP_ENABLE_MBEDTLS
http_t *http_request(const char *method, char const *url, const void *data, size_t size, const http_header_t *headers, size_t num_headers, void *memctx);
#else
http_t *http_request(const char *method, char const *url, const void *data, size_t size, const http_header_t *headers, size_t num_headers, const unsigned char *cert_data, size_t cert_data_size, void *memctx);
#endif

http_status_t http_process(http_t *http);

const char *http_get_header(http_t *http, const char *name, size_t nth);

void http_release(http_t *http);

#endif /* http_hpp */

/******************
 * IMPLEMENTATION *
 ******************/

#ifdef HTTP_IMPLEMENTATION

#ifdef _WIN32
    #define _CRT_NONSTDC_NO_DEPRECATE 
    #define _CRT_SECURE_NO_WARNINGS
    #pragma warning( push )
    #pragma warning( disable: 4127 ) // conditional expression is constant
    #pragma warning( disable: 4255 ) // 'function' : no function prototype given: converting '()' to '(void)'
    #pragma warning( disable: 4365 ) // 'action' : conversion from 'type_1' to 'type_2', signed/unsigned mismatch
    #pragma warning( disable: 4574 ) // 'Identifier' is defined to be '0': did you mean to use '#if identifier'?
    #pragma warning( disable: 4668 ) // 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directive'
    #pragma warning( disable: 4706 ) // assignment within conditional expression
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma warning( pop )
    #pragma comment (lib, "Ws2_32.lib") 
    #include <string.h>
    #include <stdio.h>
    #define HTTP_SOCKET SOCKET
    #define HTTP_INVALID_SOCKET INVALID_SOCKET
#else
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <netdb.h>
    #define HTTP_SOCKET int
    #define HTTP_INVALID_SOCKET -1
#endif

#ifndef HTTP_MALLOC
    #define _CRT_NONSTDC_NO_DEPRECATE 
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define HTTP_MALLOC( ctx, size ) ( malloc( size ) )
    #define HTTP_FREE( ctx, ptr ) ( free( ptr ) )
#endif

#ifndef HTTP_LOG
    #define HTTP_LOG(...)
#endif

#ifdef HTTP_ENABLE_MBEDTLS
    #include "mbedtls/ctr_drbg.h"
    #include "mbedtls/entropy.h"
    #include "mbedtls/net_sockets.h"
    #include "mbedtls/ssl.h"
    
    typedef struct http_tls_context_t {
        mbedtls_net_context net;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context drbg;
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;
        mbedtls_x509_crt cert;
    } http_tls_context_t;
#endif

typedef enum http_state_t {
    HTTP_STATE_UNKNOWN = 0,
    HTTP_STATE_CONNECT_PENDING,
    HTTP_STATE_SETTING_UP_TLS,
    HTTP_STATE_SENDING_REQUEST,
    HTTP_STATE_RECIEVING_RESPONSE,
    HTTP_STATE_COMPLETE,
} http_state_t;

typedef struct http_internal_t {
    /* keep this at the top because http_internal_t* can be cast to http_t* */ 
    http_t http;
    
    void* memctx;
    HTTP_SOCKET socket;
#ifdef HTTP_ENABLE_MBEDTLS
    http_tls_context_t *tls_context;
#endif
    http_state_t state;
    char address[ 256 ];
    char request_header[ 256 ];
    char* request_header_large;
    size_t request_header_sent;
    void* request_data;
    size_t request_data_size;
    size_t request_data_sent;
    char reason_phrase[ 1024 ];
    size_t data_size;
    size_t data_capacity;
    void* data;
} http_internal_t;


static int http_internal_parse_url(char const* url, int *secure, char* address, size_t address_capacity, char* port, size_t port_capacity, char const** resource ) {
    // make sure url starts with http:// or https://
    // if( strncmp( url, "http://", 7 ) != 0 ) return 0;
    // url += 7; // skip http:// part of url
    if (!strncmp(url, "http://", 7)) { *secure = 0; url += 7; }
    else if (!strncmp(url, "https://", 8)) { *secure = 1; url += 8; }
    else { return 0; }
    
    size_t url_len = strlen( url );

    // find end of address part of url
    char const* address_end = strchr( url, ':' );
    if( !address_end ) address_end = strchr( url, '/' );
    if( !address_end ) address_end = url + url_len;

    // extract address
    size_t address_len = (size_t)( address_end - url );
    if( address_len >= address_capacity ) return 0;
    memcpy( address, url, address_len );
    address[ address_len ] = 0;

    // check if there's a port defined
    char const* port_end = address_end;
    
    if (*address_end == ':') {
        ++address_end;
        port_end = strchr( address_end, '/' );
        if( !port_end ) port_end = address_end + strlen( address_end );
        size_t port_len = (size_t)( port_end - address_end );
        if( port_len >= port_capacity ) return 0;
        memcpy( port, address_end, port_len );
        port[ port_len ] = 0;
    }
    else {
        // use default port; 80 for http, 433 for https
        if (*secure) {
            if (port_capacity <= 3) return 0;
            strcpy(port, "433");
        }
        else {
            if (port_capacity <= 2) return 0;
            strcpy(port, "80");
        }
    }


    *resource = port_end;

    return 1;
    }


HTTP_SOCKET http_internal_connect( char const* address, char const* port )
    {   
    // set up hints for getaddrinfo
    struct addrinfo hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC; // the Internet Protocol version 4 (IPv4) address family.
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;    // Use Transmission Control Protocol (TCP).

    // resolve the server address and port
    struct addrinfo* addri = 0;
    int error = getaddrinfo( address, port, &hints, &addri) ;
    if( error != 0 ) return HTTP_INVALID_SOCKET;

    // create the socket
    HTTP_SOCKET sock = socket( addri->ai_family, addri->ai_socktype, addri->ai_protocol );
    if( sock == -1) 
        {
        freeaddrinfo( addri );
        return HTTP_INVALID_SOCKET;
        }

    // set socket to nonblocking mode
    u_long nonblocking = 1;
    #ifdef _WIN32
        int res = ioctlsocket( sock, FIONBIO, &nonblocking );
    #else
        int flags = fcntl( sock, F_GETFL, 0 );
        int res = fcntl( sock, F_SETFL, flags | O_NONBLOCK ); 
    #endif
    if( res == -1 )
        {
        freeaddrinfo( addri );
        #ifdef _WIN32
            closesocket( sock );
        #else
            close( sock );
        #endif
        return HTTP_INVALID_SOCKET;
        }

    // connect to server
    if( connect( sock, addri->ai_addr, (int)addri->ai_addrlen ) == -1 )
        {
        #ifdef _WIN32
            if( WSAGetLastError() != WSAEWOULDBLOCK && WSAGetLastError() != WSAEINPROGRESS )
                {
                freeaddrinfo( addri );
                closesocket( sock );
                return HTTP_INVALID_SOCKET;
                }
        #else
            if( errno != EWOULDBLOCK && errno != EINPROGRESS && errno != EAGAIN )
                {
                freeaddrinfo( addri );
                close( sock );
                return HTTP_INVALID_SOCKET;
                }
        #endif
    }

    freeaddrinfo( addri );
    return sock;
}

#ifdef HTTP_ENABLE_MBEDTLS

#define FREE_CTX(self) {\
    mbedtls_net_free(&self->net);\
    mbedtls_ssl_free(&self->ssl);\
    mbedtls_ssl_config_free(&self->conf);\
    mbedtls_entropy_free(&self->entropy);\
    mbedtls_ctr_drbg_free(&self->drbg);\
    mbedtls_x509_crt_free(&self->cert);\
    HTTP_FREE(memctx, self);\
}\

#define CHECK(EXPR) { int result = (EXPR); if (result) { HTTP_LOG("MbedTLS error: %s result=%d", #EXPR, result); FREE_CTX(self); return NULL; } }

http_tls_context_t *http_internal_create_tls_context(const char * const address, HTTP_SOCKET socket, const unsigned char *cert_data, size_t cert_data_size, void *memctx) {
    // Seems helpful: https://x509errors.org/guides/mbedtls
    http_tls_context_t *self = HTTP_MALLOC(memctx, sizeof *self);
    
    if (!self) {
        return NULL;
    }
    
    // TLS setup
    mbedtls_net_init(&self->net);
    mbedtls_ssl_config_init(&self->conf);
    mbedtls_ssl_init(&self->ssl);
    mbedtls_entropy_init(&self->entropy);
    mbedtls_ctr_drbg_init(&self->drbg);
    mbedtls_x509_crt_init(&self->cert);
    
    // Seed RNG
    CHECK(mbedtls_ctr_drbg_seed(&self->drbg, mbedtls_entropy_func, &self->entropy, NULL, 0));
    
    // Set socket fd
    self->net.fd = socket;
    
    // Setup config
    CHECK(mbedtls_ssl_config_defaults(&self->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));
    
    mbedtls_ssl_conf_rng(&self->conf, mbedtls_ctr_drbg_random, &self->drbg);
    mbedtls_ssl_conf_min_version(&self->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    
    if (cert_data) {
        CHECK(mbedtls_x509_crt_parse(&self->cert, cert_data, cert_data_size));
        mbedtls_ssl_conf_ca_chain(&self->conf, &self->cert, NULL);
        mbedtls_ssl_conf_authmode(&self->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    else {
        mbedtls_ssl_conf_authmode(&self->conf, MBEDTLS_SSL_VERIFY_NONE);
    }
    
    CHECK(mbedtls_ssl_setup(&self->ssl, &self->conf));
    
    if (cert_data) {
        CHECK(mbedtls_ssl_set_hostname(&self->ssl, address));
    }
    
    CHECK(mbedtls_net_set_nonblock(&self->net));
    mbedtls_ssl_set_bio(&self->ssl, &self->net, mbedtls_net_send, mbedtls_net_recv, NULL);
    
    return self;
}

void http_internal_release_tls_context(http_tls_context_t *self, void *memctx) {
    mbedtls_ssl_close_notify(&self->ssl);
    FREE_CTX(self);
}
#endif

    
static http_internal_t *http_internal_create(size_t request_data_size, void* memctx) {
    http_internal_t *internal = (http_internal_t *) HTTP_MALLOC(memctx, sizeof (http_internal_t) + request_data_size);
    
    if (!internal) {
        return NULL;
    }
    
    memset(internal, 0, sizeof *internal);
    
    internal->http.status = HTTP_STATUS_PENDING;
    
    internal->memctx = memctx;
    internal->state = HTTP_STATE_CONNECT_PENDING;
    
    internal->http.reason_phrase = internal->reason_phrase;
    
    internal->data_capacity = 64 * 1024;
    internal->data = HTTP_MALLOC( memctx, internal->data_capacity );
    
    if (!internal->data) {
        return NULL;
    }
    
    internal->request_data = NULL;
    internal->request_data_size = 0;
    
    return internal;
}

static size_t http_approximate_headers_size(const http_header_t *headers, size_t num_headers) {
    /**
     * Approximate the resulting length of the given headers
     */
    
    size_t length = 0;
    
    if (headers) {
        for (size_t i = 0; i < num_headers; i++) {
            length += strlen(headers[i].name);
            length += strlen(headers[i].value);
            length += 4; /** Extra \r\n and the ": " seperator **/
        }
    }
    
    return length;
}

#define TEMP_LINE_LENGTH 4096

#ifdef HTTP_ENABLE_MBEDTLS
http_t *http_request(const char *method, char const *url, const void *data, size_t size, const http_header_t *headers, size_t num_headers, const unsigned char *cert_data, size_t cert_data_size, void *memctx) {
#else
http_t *http_request(const char *method, char const *url, const void *data, size_t size, const http_header_t *headers, size_t num_headers, void *memctx) {
#endif
    #ifdef _WIN32
        WSADATA wsa_data;
        if( WSAStartup( MAKEWORD( 1, 0 ), &wsa_data ) != 0 ) return 0;
    #endif
    
    int secure;
    char address[ 256 ];
    char port[ 16 ];
    char const* path;
    
    if (http_internal_parse_url(url, &secure, address, sizeof( address ), port, sizeof( port ), &path ) == 0) {
        return NULL;
    }
    
#ifndef HTTP_ENABLE_MBEDTLS
    if (secure) {
        return NULL;
    }
#endif

    HTTP_SOCKET socket = http_internal_connect(address, port);
    
    if (socket == HTTP_INVALID_SOCKET) {
        return NULL;
    }
    
    http_internal_t* internal = http_internal_create(size, memctx);
    internal->socket = socket;
    
#ifdef HTTP_ENABLE_MBEDTLS
    if (secure) {
        internal->tls_context = http_internal_create_tls_context(address, socket, cert_data, cert_data_size, memctx);
        
        if (!internal->tls_context) {
            close(internal->socket);
            free(internal->data);
            free(internal);
            return NULL;
        }
    }
#endif

    char* request_header = NULL;
    size_t request_header_len = 64 + strlen(method) + strlen(path) + strlen(address) + strlen(port) + http_approximate_headers_size(headers, num_headers);
    
    if (request_header_len < sizeof( internal->request_header )) {
        internal->request_header_large = NULL;
        request_header = internal->request_header;
    }
    else {
        internal->request_header_large = (char*) HTTP_MALLOC( memctx, request_header_len + 1 );
        request_header = internal->request_header_large;
    }
    
    request_header[0] = '\0';
    
    int default_http_port = secure ? (strcmp(port, "433") == 0) : (strcmp(port, "80") == 0);
    
    char temp_line[TEMP_LINE_LENGTH] = {};
    
    // Request line
    snprintf(temp_line, TEMP_LINE_LENGTH, "%s %s HTTP/1.0\r\n", method, path);
    strcat(request_header, temp_line);
    
    // Host header
    snprintf(temp_line, TEMP_LINE_LENGTH, "Host: %s%s%s\r\n", address, default_http_port ? "" : ":", default_http_port ? "" : port);
    strcat(request_header, temp_line);
    
    // Content length, if this request has a body
    if (data) {
        snprintf(temp_line, TEMP_LINE_LENGTH, "Content-Length: %zu\r\n", size);
        strcat(request_header, temp_line);
    }
    
    // Headers, if there are any
    if (headers) {
        for (size_t i = 0; i < num_headers; i++) {
            snprintf(temp_line, TEMP_LINE_LENGTH, "%s: %s\r\n", headers[i].name, headers[i].value);
            strcat(request_header, temp_line);
        }
    }
    
    // End of headers
    strcat(request_header, "\r\n");
    
    if (request_header_len <= strlen(request_header)) {
        // We're overwriting the blasted heap! You idiot! You know you shouldn't
        // have used strcat! Just abort now so we don't loose our minds later.
        abort();
    }
    
    // Body
    if (data) {
        internal->request_data_size = size;
        internal->request_data = ( internal + 1 );
        memcpy( internal->request_data, data, size );
    }
    
    return &internal->http;
}


http_status_t http_process(http_t *http) {
    http_internal_t* internal = (http_internal_t*) http;    
    
    if (http->status != HTTP_STATUS_PENDING) {
        return http->status;
    }
    
    // If we're pending connect(), update status for it
    if (internal->state == HTTP_STATE_CONNECT_PENDING) {   
        fd_set sockets_to_check; 
        FD_ZERO( &sockets_to_check );
        #pragma warning( push )
        #pragma warning( disable: 4548 ) // expression before comma has no effect; expected expression with side-effect
        FD_SET( internal->socket, &sockets_to_check );
        #pragma warning( pop )
        struct timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 0;
        
        // check if socket is ready for send; if it is, we're connected
        if (select((int)(internal->socket + 1), NULL, &sockets_to_check, NULL, &timeout) == 1 ) {
            int opt = -1;
            socklen_t len = sizeof(opt);
            
            if (getsockopt( internal->socket, SOL_SOCKET, SO_ERROR, (char*)( &opt ), &len) >= 0 && opt == 0) {
#ifdef HTTP_ENABLE_MBEDTLS
                if (internal->tls_context) {
                    internal->state = HTTP_STATE_SETTING_UP_TLS;
                }
                else {
#endif
                    internal->state = HTTP_STATE_SENDING_REQUEST;
#ifdef HTTP_ENABLE_MBEDTLS
                }
#endif
            }
        }
    }
    
    // Still pending?
    if (internal->state == HTTP_STATE_CONNECT_PENDING) {
        return http->status;
    }
    
#ifdef HTTP_ENABLE_MBEDTLS
    if (internal->state == HTTP_STATE_SETTING_UP_TLS) {
        int result = mbedtls_ssl_handshake(&internal->tls_context->ssl);
        
        if (result == 0) {
            internal->state = HTTP_STATE_SENDING_REQUEST;
        }
        else if (result == MBEDTLS_ERR_SSL_WANT_READ ||
            result == MBEDTLS_ERR_SSL_WANT_WRITE ||
            result == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
            result == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
            // Just waiting...
            return http->status;
        }
        else {
            http->status = HTTP_STATUS_FAILED;
            return http->status;
        }
    }
#endif

    if (internal->state == HTTP_STATE_SENDING_REQUEST) {
        char const* request_header = internal->request_header_large ? internal->request_header_large : internal->request_header;
        const size_t request_header_len = strlen(request_header);
        
#ifdef HTTP_ENABLE_MBEDTLS
        if (internal->tls_context) {
            if (internal->request_header_sent < request_header_len) {
                int status = mbedtls_ssl_write(&internal->tls_context->ssl, (const unsigned char *) request_header + internal->request_header_sent, request_header_len - internal->request_header_sent);
                
                if (status < 0) {
                    if (status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE && status != MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS && status != MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
                        http->status = HTTP_STATUS_FAILED;
                        return http->status;
                    }
                    else {
                        return http->status;
                    }
                }
                else {
                    internal->request_header_sent += status;
                    
                    if (internal->request_header_sent < request_header_len) {
                        return http->status;
                    }
                }
            }
        }
        else {
#endif
            if (send(internal->socket, request_header, request_header_len, 0 ) == -1) {
                http->status = HTTP_STATUS_FAILED;
                return http->status;
            }
#ifdef HTTP_ENABLE_MBEDTLS
        }
#endif
        
        if (internal->request_data_size) {
#ifdef HTTP_ENABLE_MBEDTLS
            if (internal->tls_context) {
                if (internal->request_data_sent < internal->request_data_size) {
                    int status = mbedtls_ssl_write(&internal->tls_context->ssl, internal->request_data + internal->request_data_sent, internal->request_data_size - internal->request_data_sent);
                    
                    if (status < 0) {
                        if (status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE && status != MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS && status != MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
                            http->status = HTTP_STATUS_FAILED;
                            return http->status;
                        }
                        else {
                            return http->status;
                        }
                    }
                    else {
                        internal->request_data_sent += status;
                        
                        if (internal->request_data_sent < internal->request_data_size) {
                            return http->status;
                        }
                    }
                }
            }
            else {
#endif
                int res = send(internal->socket, (char const*)internal->request_data, (int) internal->request_data_size, 0);
                
                if (res == -1) {
                    http->status = HTTP_STATUS_FAILED;
                    return http->status;
                }
#ifdef HTTP_ENABLE_MBEDTLS
            }
#endif
        }
        
        internal->state = HTTP_STATE_RECIEVING_RESPONSE;
        return http->status;
    }

    // check if socket is ready for recv
    fd_set sockets_to_check; 
    FD_ZERO( &sockets_to_check );
    #pragma warning( push )
    #pragma warning( disable: 4548 ) // expression before comma has no effect; expected expression with side-effect
    FD_SET( internal->socket, &sockets_to_check );
    #pragma warning( pop )
    struct timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 0;
    
    while (1) {
#ifdef HTTP_ENABLE_MBEDTLS
        if (internal->tls_context) {
            if (mbedtls_net_poll(&internal->tls_context->net, MBEDTLS_NET_POLL_READ, 0) != MBEDTLS_NET_POLL_READ) {
                break;
            }
        }
        else {
#endif
            if (select( (int)( internal->socket + 1 ), &sockets_to_check, NULL, NULL, &timeout ) != 1) {
                break;
            }
#ifdef HTTP_ENABLE_MBEDTLS
        }
#endif
        
        char buffer[4096];
        int size;
        
#ifdef HTTP_ENABLE_MBEDTLS
        if (internal->tls_context) {
            size = mbedtls_ssl_read(&internal->tls_context->ssl, (unsigned char *) buffer, sizeof buffer);
            
            if (size < 0) {
                size = -1;
            }
        }
        else {
#endif
            size = recv(internal->socket, buffer, sizeof buffer, 0);
#ifdef HTTP_ENABLE_MBEDTLS
        }
#endif
    
        if (size == -1) {
            http->status = HTTP_STATUS_FAILED;
            return http->status;
        }
        else if (size > 0) {
            size_t min_size = internal->data_size + size + 1;
            
            if (internal->data_capacity < min_size) {
                internal->data_capacity *= 2; 
                if( internal->data_capacity < min_size ) internal->data_capacity = min_size;
                void* new_data = HTTP_MALLOC( memctx, internal->data_capacity );
                memcpy( new_data, internal->data, internal->data_size );
                HTTP_FREE( memctx, internal->data );
                internal->data = new_data;
            }
            
            memcpy( (void*)( ( (uintptr_t) internal->data ) + internal->data_size ), buffer, (size_t) size );
            internal->data_size += size;
        }
        else if (size == 0) {
            char *status_line = (char *) internal->data;

            int header_size = 0;
            char *header_end = strstr( status_line, "\r\n\r\n" );
            
            if (header_end) {
                header_end += 4;
                header_size = (int)( header_end - status_line );
            }
            else {
                http->status = HTTP_STATUS_FAILED;
                return http->status;
            }

            // skip http version
            status_line = strchr( status_line, ' ' );
            
            if (!status_line) {
                http->status = HTTP_STATUS_FAILED;
                return http->status;
            }
            
            ++status_line;
            
            // extract status code
            char status_code[ 16 ];
            char *status_code_end = strchr( status_line, ' ' );
            
            if (!status_code_end) {
                http->status = HTTP_STATUS_FAILED;
                return http->status;
            }
            
            memcpy( status_code, status_line, (size_t)( status_code_end - status_line ) );
            status_code[ status_code_end - status_line ] = 0;
            status_line = status_code_end + 1;
            http->status_code = atoi( status_code );
            
            // extract reason phrase
            char *reason_phrase_end = strstr( status_line, "\r\n" );
            
            if (!reason_phrase_end) {
                http->status = HTTP_STATUS_FAILED;
                return http->status;
            }
            
            size_t reason_phrase_len = (size_t)( reason_phrase_end - status_line );
            
            if (reason_phrase_len >= sizeof( internal->reason_phrase )) {
                reason_phrase_len = sizeof( internal->reason_phrase ) - 1;
            }
            
            memcpy( internal->reason_phrase, status_line, reason_phrase_len );
            internal->reason_phrase[ reason_phrase_len ] = 0;
            status_line = reason_phrase_end + 2;
            
            // Process headers
            size_t cap_headers = 0;
            
            while (status_line[0] != '\r' && status_line[0] != '\n' && status_line[0] != '\0') {
                // Increase capacity if we are out of room for more headers
                if (http->num_headers >= cap_headers) {
                    cap_headers = 2 * cap_headers + 3;
                    http_header_t *new_headers = HTTP_MALLOC(memctx, sizeof *new_headers * cap_headers);
                    memcpy(new_headers, http->headers, sizeof *new_headers * http->num_headers);
                    HTTP_FREE(memctx, http->headers);
                    http->headers = new_headers;
                }
                
                char *name = status_line;
                char *value = strchr(status_line, ':');
                status_line = strstr(status_line, "\r\n");
                
                // Add NUL terminators and increment where appropriate
                *value = '\0'; value++; while (*value == ' ') { value++; }
                *status_line = '\0'; status_line += 2;
                
                http->headers[http->num_headers].name = name;
                http->headers[http->num_headers].value = value;
                http->num_headers++;
            }

            // XXX: I didn't really like that non-200 status codes return
            // failures so they no longer count as failed.
            // http->status =  http->status_code < 300 ? HTTP_STATUS_COMPLETED : HTTP_STATUS_FAILED;
            http->status = HTTP_STATUS_COMPLETED;
            http->response_data = (void*)( ( (uintptr_t) internal->data ) + header_size );
            http->response_size = internal->data_size - header_size;

            // add an extra zero after the received data, but don't modify the
            // size, so ascii results can be used as a zero terminated string.
            // the size returned will be the string without this extra zero
            // terminator.
            ( (char*)http->response_data )[ http->response_size ] = 0;
            
            internal->state = HTTP_STATE_COMPLETE;
            
            return http->status;
        }
    }
    
    return http->status;
}


#define HTTP_TOLOWER(ch) ((ch >= 'A' && ch <= 'Z') ? (ch - ('a' - 'A')) : (ch))

static int http_strieq(const char *first, const char *second) {
    size_t first_len = strlen(first);
    size_t second_len = strlen(second);
    
    if (first_len != second_len) {
        return 0;
    }
    
    for (size_t i = 0; i < first_len; i++) {
        if (HTTP_TOLOWER(first[i]) != HTTP_TOLOWER(second[i])) {
            return 0;
        }
    }
    
    return 1;
}


const char *http_get_header(http_t *http, const char *name, size_t nth) {
    /**
     * Return the value of the nth header with the given case-insensitive name
     */
    
    if (http->status != HTTP_STATUS_COMPLETED) { return NULL; }
    
    for (size_t i = 0; i < http->num_headers; i++) {
        if (http_strieq(http->headers[i].name, name) && !(nth--)) {
            return http->headers[i].value;
        }
    }
    
    return NULL;
}


void http_release(http_t *http) {
    http_internal_t* internal = (http_internal_t*) http;
    void *memctx = internal->memctx;
    
#ifdef HTTP_ENABLE_MBEDTLS
    if (internal->tls_context) {
        http_internal_release_tls_context(internal->tls_context, memctx);
    }
    else {
#endif
    #ifdef _WIN32
        closesocket( internal->socket );
    #else
        close( internal->socket );
    #endif
#ifdef HTTP_ENABLE_MBEDTLS
    }
#endif

    if (internal->request_header_large) { HTTP_FREE( memctx, internal->request_header_large ); }
    if (http->headers) { HTTP_FREE(memctx, http->headers); }
    HTTP_FREE( memctx, internal->data );
    HTTP_FREE( memctx, internal );
    
    #ifdef _WIN32
        WSACleanup();
    #endif
}


#endif /* HTTP_IMPLEMENTATION */

/*
revision history:
    1.0     first released version  
*/

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2016 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/
