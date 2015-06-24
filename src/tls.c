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

#include <string.h>		/* strcpy, strcat, memcpy */
#include "config.h"
#include "tls.h"

#ifdef USE_TLS_LIBRARY

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
	// external private key and certificate in the *.pem file
	if ((res = SSL_CTX_use_PrivateKey_file(ssl_ctx, PEM_FILE, 1)) == 0)
		return NULL;
	if ((res = SSL_CTX_use_certificate_file(ssl_ctx, PEM_FILE, 1)) == 0)
		return NULL;
//	if ((res = SSL_CTX_use_certificate_chain_file(ssl_ctx, PEM_FILE)) == 0)
//		return NULL;
#elif defined AXTLS_LIBRARY /* axTLS */
#if 1
	// external private key and certificate in the *.pem file
	int res;

	ssl_ctx = ssl_ctx_new(0, 2); // 2 sessions only yet
//	ssl_ctx = ssl_ctx_new(SSL_DISPLAY_CERTS | SSL_DISPLAY_STATES | SSL_DISPLAY_RSA | SSL_DISPLAY_BYTES, 2); // 2 sessions only yet

	if ((res = ssl_obj_load(ssl_ctx, SSL_OBJ_RSA_KEY | SSL_OBJ_X509_CERT, PEM_FILE, NULL)) != SSL_OK)
		return NULL;
//	if ((res = ssl_obj_load(ssl_ctx, SSL_OBJ_RSA_KEY, PEM_FILE, NULL)) != SSL_OK)
//		return NULL;
#endif
#if 0
	// built-in axTLS library certificate
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

#else

/* SHA-1 code Copyright(c) by Steve Reid <steve@edmweb.com> */
//--------------------------------------------
static int is_big_endian(void)
{
	static const int n = 1;
	return ((char *)&n)[0] == 0;
}

//--------------------------------------------
union char64long16
{
	unsigned char c[64];
	uint32_t l[16];
};

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

//--------------------------------------------
static uint32_t blk0(union char64long16 *block, int i)
{
	/* Forrest: SHA expect BIG_ENDIAN, swap if LITTLE_ENDIAN */
	if (!is_big_endian()) {
		block->l[i] = (rol(block->l[i], 24) & 0xFF00FF00) |
			(rol(block->l[i], 8) & 0x00FF00FF);
	}
	return block->l[i];
}

#define blk(i)                                                                 \
	(block->l[i & 15] = rol(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ \
	block->l[(i + 2) & 15] ^ block->l[i & 15],     \
	1))
#define R0(v, w, x, y, z, i)                                                   \
	z += ((w & (x ^ y)) ^ y) + blk0(block, i) + 0x5A827999 + rol(v, 5);        \
	w = rol(w, 30);
#define R1(v, w, x, y, z, i)                                                   \
	z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5);                \
	w = rol(w, 30);
#define R2(v, w, x, y, z, i)                                                   \
	z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5);                        \
	w = rol(w, 30);
#define R3(v, w, x, y, z, i)                                                   \
	z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5);          \
	w = rol(w, 30);
#define R4(v, w, x, y, z, i)                                                   \
	z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5);                        \
	w = rol(w, 30);

//--------------------------------------------
static void SHA1Transform(uint32_t state[5], const unsigned char buffer[64])
{
	uint32_t a, b, c, d, e;
	union char64long16 block[1];

	memcpy(block, buffer, 64);
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	R0(a, b, c, d, e, 0);
	R0(e, a, b, c, d, 1);
	R0(d, e, a, b, c, 2);
	R0(c, d, e, a, b, 3);
	R0(b, c, d, e, a, 4);
	R0(a, b, c, d, e, 5);
	R0(e, a, b, c, d, 6);
	R0(d, e, a, b, c, 7);
	R0(c, d, e, a, b, 8);
	R0(b, c, d, e, a, 9);
	R0(a, b, c, d, e, 10);
	R0(e, a, b, c, d, 11);
	R0(d, e, a, b, c, 12);
	R0(c, d, e, a, b, 13);
	R0(b, c, d, e, a, 14);
	R0(a, b, c, d, e, 15);
	R1(e, a, b, c, d, 16);
	R1(d, e, a, b, c, 17);
	R1(c, d, e, a, b, 18);
	R1(b, c, d, e, a, 19);
	R2(a, b, c, d, e, 20);
	R2(e, a, b, c, d, 21);
	R2(d, e, a, b, c, 22);
	R2(c, d, e, a, b, 23);
	R2(b, c, d, e, a, 24);
	R2(a, b, c, d, e, 25);
	R2(e, a, b, c, d, 26);
	R2(d, e, a, b, c, 27);
	R2(c, d, e, a, b, 28);
	R2(b, c, d, e, a, 29);
	R2(a, b, c, d, e, 30);
	R2(e, a, b, c, d, 31);
	R2(d, e, a, b, c, 32);
	R2(c, d, e, a, b, 33);
	R2(b, c, d, e, a, 34);
	R2(a, b, c, d, e, 35);
	R2(e, a, b, c, d, 36);
	R2(d, e, a, b, c, 37);
	R2(c, d, e, a, b, 38);
	R2(b, c, d, e, a, 39);
	R3(a, b, c, d, e, 40);
	R3(e, a, b, c, d, 41);
	R3(d, e, a, b, c, 42);
	R3(c, d, e, a, b, 43);
	R3(b, c, d, e, a, 44);
	R3(a, b, c, d, e, 45);
	R3(e, a, b, c, d, 46);
	R3(d, e, a, b, c, 47);
	R3(c, d, e, a, b, 48);
	R3(b, c, d, e, a, 49);
	R3(a, b, c, d, e, 50);
	R3(e, a, b, c, d, 51);
	R3(d, e, a, b, c, 52);
	R3(c, d, e, a, b, 53);
	R3(b, c, d, e, a, 54);
	R3(a, b, c, d, e, 55);
	R3(e, a, b, c, d, 56);
	R3(d, e, a, b, c, 57);
	R3(c, d, e, a, b, 58);
	R3(b, c, d, e, a, 59);
	R4(a, b, c, d, e, 60);
	R4(e, a, b, c, d, 61);
	R4(d, e, a, b, c, 62);
	R4(c, d, e, a, b, 63);
	R4(b, c, d, e, a, 64);
	R4(a, b, c, d, e, 65);
	R4(e, a, b, c, d, 66);
	R4(d, e, a, b, c, 67);
	R4(c, d, e, a, b, 68);
	R4(b, c, d, e, a, 69);
	R4(a, b, c, d, e, 70);
	R4(e, a, b, c, d, 71);
	R4(d, e, a, b, c, 72);
	R4(c, d, e, a, b, 73);
	R4(b, c, d, e, a, 74);
	R4(a, b, c, d, e, 75);
	R4(e, a, b, c, d, 76);
	R4(d, e, a, b, c, 77);
	R4(c, d, e, a, b, 78);
	R4(b, c, d, e, a, 79);
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	a = b = c = d = e = 0;
	memset(block, '\0', sizeof(block));
}

//--------------------------------------------
void SHA1_Init(SHA_CTX *context)
{
	context->state[0] = 0x67452301;
	context->state[1] = 0xEFCDAB89;
	context->state[2] = 0x98BADCFE;
	context->state[3] = 0x10325476;
	context->state[4] = 0xC3D2E1F0;
	context->count[0] = context->count[1] = 0;
}

//--------------------------------------------
void SHA1_Update(SHA_CTX *context, const uint8_t *data, uint32_t len)
{
	uint32_t i, j;

	j = context->count[0];
	if ((context->count[0] += len << 3) < j)
		context->count[1]++;
	context->count[1] += (len >> 29);
	j = (j >> 3) & 63;
	if ((j + len) > 63) {
		memcpy(&context->buffer[j], data, (i = 64 - j));
		SHA1Transform(context->state, context->buffer);
		for (; i + 63 < len; i += 64) {
			SHA1Transform(context->state, &data[i]);
		}
		j = 0;
	} else
		i = 0;
	memcpy(&context->buffer[j], &data[i], len - i);
}

//--------------------------------------------
void SHA1_Final(uint8_t *digest, SHA_CTX *context)
{
	unsigned i;
	unsigned char finalcount[8], c;

	for (i = 0; i < 8; i++) {
		finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)] >>
			((3 - (i & 3)) * 8)) &
			255);
	}
	c = 0200;
	SHA1_Update(context, &c, 1);
	while ((context->count[0] & 504) != 448) {
		c = 0000;
		SHA1_Update(context, &c, 1);
	}
	SHA1_Update(context, finalcount, 8);
	for (i = 0; i < 20; i++) {
		digest[i] =
			(unsigned char)((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) &
			255);
	}
	memset(context, '\0', sizeof(*context));
	memset(&finalcount, '\0', sizeof(finalcount));
}

#endif /* USE_TLS_LIBRARY */
