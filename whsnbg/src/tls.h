/*
* Copyright (c) 2013-2014 Vladimir Alemasov
* All rights reserved
*
* This program and the accompanying materials are distributed under 
* the terms of GNU General Public License version 2 
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef __TLS_H__
#define __TLS_H__

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#endif

#include "config.h"
#include "os_port.h"

#if defined OPENSSL_LIBRARY && defined AXTLS_LIBRARY
#error "You must select one ssl/tls library"
#endif

#ifdef OPENSSL_LIBRARY /* OpenSSL */
#ifdef WIN32 /* Windows */
#pragma comment(lib,"libeay32MD.lib")
#pragma comment(lib,"ssleay32MD.lib")
#endif

#ifdef SSL_LIBRARY_HEADERS /* ssl library headers */
#include <openssl/ssl.h>
#include <openssl/err.h>

#else /* no ssl library headers */
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

int SSL_library_init(void);
void SSL_load_error_strings(void);
const SSL_METHOD *SSLv23_server_method(void);
int	SSL_CTX_use_certificate_file(SSL_CTX *ssl_ctx, const char *file, int type);
int	SSL_CTX_use_PrivateKey_file(SSL_CTX *ssl_ctx, const char *file, int type);
int	SSL_CTX_use_certificate_chain_file(SSL_CTX *ssl_ctx, const char *file); /* PEM type */
SSL *SSL_new(SSL_CTX *ssl_ctx);
int	SSL_set_fd(SSL *ssl, int fd);
int SSL_accept(SSL *ssl);
int SSL_shutdown(SSL *ssl);
void SSL_free(SSL *ssl);
int SSL_read(SSL *ssl,void *buf,int num);
int SSL_write(SSL *ssl,const void *buf,int num);
SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth);

typedef struct SHA_CTX
{
	uint32_t h0,h1,h2,h3,h4;
	uint32_t Nl,Nh;
	uint32_t data[16];
	unsigned int num;
} SHA_CTX;
int SHA1_Init(SHA_CTX *c);
int SHA1_Update(SHA_CTX *c, const void *data, size_t len);
int SHA1_Final(unsigned char *md, SHA_CTX *c);
#endif


#elif defined AXTLS_LIBRARY /* axTLS */
#ifdef WIN32 /* Windows */
#pragma comment(lib,"axtls.lib")
#endif

#ifdef SSL_LIBRARY_HEADERS /* ssl library headers */
#include "ssl.h"
#define SHA_CTX	SHA1_CTX

#else /* no ssl library headers */
typedef struct _SSL SSL;
typedef struct _SSL_CTX SSL_CTX;

/* The optional parameters that can be given to the client/server SSL engine */
#define SSL_CLIENT_AUTHENTICATION               0x00010000
#define SSL_SERVER_VERIFY_LATER                 0x00020000
#define SSL_NO_DEFAULT_KEY                      0x00040000
#define SSL_DISPLAY_STATES                      0x00080000
#define SSL_DISPLAY_BYTES                       0x00100000
#define SSL_DISPLAY_CERTS                       0x00200000
#define SSL_DISPLAY_RSA                         0x00400000
#define SSL_CONNECT_IN_PARTS                    0x00800000

#define SSL_OBJ_X509_CERT                       1

SSL_CTX * STDCALL ssl_ctx_new(uint32_t options, int num_sessions);
void STDCALL ssl_ctx_free(SSL_CTX *ssl_ctx);
SSL * STDCALL ssl_server_new(SSL_CTX *ssl_ctx, int client_fd);
void STDCALL ssl_free(SSL *ssl);
int STDCALL ssl_read(SSL *ssl, uint8_t **in_data);
int STDCALL ssl_write(SSL *ssl, const uint8_t *out_data, int out_len);
int STDCALL ssl_obj_load(SSL_CTX *ssl_ctx, int obj_type, const char *filename, const char *password);

typedef struct SHA_CTX
{
	uint32_t Intermediate_Hash[5];  /* Message Digest */
	uint32_t Length_Low;            /* Message length in bits */
	uint32_t Length_High;           /* Message length in bits */
	uint16_t Message_Block_Index;   /* Index into message block array */
	uint8_t Message_Block[64];      /* 512-bit message blocks */
} SHA_CTX;
void STDCALL SHA1_Init(SHA_CTX *);
void STDCALL SHA1_Update(SHA_CTX *, const uint8_t *msg, int len);
void STDCALL SHA1_Final(uint8_t *digest, SHA_CTX *);
#endif


#else /* nothing */
#error "You must select the ssl/tls library"
#endif

SSL_CTX *tls_ctx_new(void);
SSL *tls_server_new(SSL_CTX *ssl_ctx, int fd);
int tls_recv(SSL *ssl, unsigned char **buf_addr, size_t size);
int tls_send(SSL *ssl, const unsigned char *buf, size_t size);
void tls_free(SSL *ssl);
void tls_ctx_free(SSL_CTX * ssl_ctx);

#endif /* __TLS_H__ */
