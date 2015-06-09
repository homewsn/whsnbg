/*
* Copyright (c) 2013-2015 Vladimir Alemasov
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

#include "tls.h"


//--------------------------------------------
SSL_CTX *tls_ctx_new(void)
{
	SSL_CTX *ssl_ctx = NULL;

#ifdef OPENSSL_LIBRARY /* OpenSSL */
	int res;

	SSL_library_init();
	SSL_load_error_strings();
	if ((ssl_ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
		return NULL;
	res = SSL_CTX_use_certificate_file(ssl_ctx, "ssl_cert.pem", 1);
	res = SSL_CTX_use_PrivateKey_file(ssl_ctx, "ssl_cert.pem", 1);
	res = SSL_CTX_use_certificate_chain_file(ssl_ctx, "ssl_cert.pem");
#elif defined AXTLS_LIBRARY /* axTLS */
#if 0
	int res;

	ssl_ctx = ssl_ctx_new(SSL_DISPLAY_CERTS | SSL_DISPLAY_STATES, 2); // 2 sessions only yet
	res = ssl_obj_load(ssl_ctx, SSL_OBJ_X509_CERT, "whsnbg.pem", NULL);
#endif
#if 1
	ssl_ctx = ssl_ctx_new(0, 2); // 2 sessions only yet
#endif

#endif
	return ssl_ctx;
}

//--------------------------------------------
SSL *tls_server_new(SSL_CTX *ssl_ctx, int fd)
{
	SSL *ssl = NULL;

#ifdef OPENSSL_LIBRARY /* OpenSSL */
	if ((ssl = SSL_new(ssl_ctx)) == NULL)
		return NULL;
	if (SSL_set_fd(ssl, fd) == 0)
		goto error;
	if (SSL_accept(ssl) <= 0)
		goto error;
	return ssl;

error:
//		printf("Error: %s\n", ERR_reason_error_string(ERR_get_error()));
	SSL_shutdown(ssl);
	SSL_free(ssl);
	return NULL;

#elif defined AXTLS_LIBRARY /* axTLS */
	ssl = ssl_server_new(ssl_ctx, fd);

#endif
	return ssl;
}

//--------------------------------------------
int tls_recv(SSL *ssl, unsigned char **buf_addr, size_t size)
{
#ifdef OPENSSL_LIBRARY /* OpenSSL */
	int recv_cnt = SSL_read(ssl, *buf_addr, (int)size);
	if (recv_cnt == 0)
		return -1;
	return recv_cnt;

#elif defined AXTLS_LIBRARY /* axTLS */
	return ssl_read(ssl, buf_addr);

#endif
}

//--------------------------------------------
int tls_send(SSL *ssl, const unsigned char *buf, size_t size)
{
	int send_cnt = 0;

#ifdef OPENSSL_LIBRARY /* OpenSSL */
	send_cnt = SSL_write(ssl, buf, (int)size);

#elif defined AXTLS_LIBRARY /* axTLS */
	send_cnt = ssl_write(ssl, buf, (int)size);

#endif
	return send_cnt;
}

//--------------------------------------------
void tls_free(SSL *ssl)
{
#ifdef OPENSSL_LIBRARY /* OpenSSL */
	SSL_shutdown(ssl);
	SSL_free(ssl);

#elif defined AXTLS_LIBRARY /* axTLS */
	ssl_free(ssl);

#endif
}

//--------------------------------------------
void tls_ctx_free(SSL_CTX * ssl_ctx)
{
#ifdef OPENSSL_LIBRARY /* OpenSSL */
	// Is something needed here?

#elif defined AXTLS_LIBRARY /* axTLS */
	ssl_ctx_free(ssl_ctx);

#endif
}
