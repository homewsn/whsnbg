/*
* Copyright (c) 2013-2015, 2018, 2019 Vladimir Alemasov
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

#include "config.h"
#ifdef LINUX_DAEMON_VERSION
#include <unistd.h>
#include <syslog.h>
#endif

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#endif

#include <stdio.h>		/* sprintf, sscanf */
#include <string.h>		/* strcpy, strcat, memcpy */
#include <assert.h>		/* assert */
#include "os_port.h"
#include "tls.h"
#include "list_tcp_conn.h"
#include "msg_tcp_mqtt.h"
#include "msg_mqtt_tcp.h"
#include "mqtt.h"
#include "thread_state.h"
#include "thread_tcp.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#include <stdio.h>
#ifdef LINUX_DAEMON_VERSION
#include <errno.h>		/* errno */
#include <string.h>		/* strerror */
#define dprintf(...) syslog(LOG_DEBUG, __VA_ARGS__)
#define print_error_socket(line) syslog(LOG_DEBUG, "Socket Error on line %d: %s\n", line, strerror(errno))
#else
#define dprintf(...) printf(__VA_ARGS__)
#ifdef WIN32
#define print_error_socket(line) \
	do { \
	char output[1024]; \
	LPTSTR s = NULL; \
	FormatMessageA(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
	NULL, \
	WSAGetLastError(), \
	0, \
	(LPTSTR)&s, \
	0, \
	NULL); \
	fprintf(stderr, "Socket Error on line %d: %s\n", line, s); \
	sprintf((char *)&output, "Socket Error on line %d: %s\n", line, s); \
	LocalFree(s); \
	OutputDebugStringA(output); \
	} while (0)
#else
#include <errno.h>		/* errno */
#include <string.h>		/* strerror */
#define print_error_socket(line) fprintf(stderr, "Socket Error on line %d: %s\n", line, strerror(errno))
#endif
#endif
#else
#define dprintf(...)
#define print_error_socket(...)
#endif



//--------------------------------------------
typedef enum
{
	MODE_MQTT,	// MQTT over plain TCP
	MODE_WS		// MQTT over WebSockets
} mqtt_mode_t;

typedef struct serv_tcp
{
	int is_tls;
	unsigned short port;
	SOCKET sock;
#ifdef USE_TLS_LIBRARY
	SSL_CTX *ssl_ctx;
#endif
	mqtt_mode_t mode;
} serv_tcp_t;

#ifdef USE_TLS_LIBRARY
#define MQTT_TCP_SERVERS		4
#else
#define MQTT_TCP_SERVERS		2
#endif
#define MAX_HTTP_HEADER_LENGTH	2048
#define ZIGBEE2MQTT_BUF_LENGTH	32768
#define SOCK_BUF_LENGTH			ZIGBEE2MQTT_BUF_LENGTH

//--------------------------------------------
static serv_tcp_t servs[MQTT_TCP_SERVERS] = { 0 };
static thread_tcp_options_t *thread_options;
static list_tcp_conn_t *conns = NULL;
static unsigned char sock_buf[SOCK_BUF_LENGTH];
static volatile thread_state_t thread_state;


//--------------------------------------------
//** encode

//--------------------------------------------
void base64_encode(const unsigned char *src, int src_len, char *dst)
{
	static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i, j, a, b, c;

	for (i = j = 0; i < src_len; i += 3)
	{
		a = src[i];
		b = i + 1 >= src_len ? 0 : src[i + 1];
		c = i + 2 >= src_len ? 0 : src[i + 2];

		dst[j++] = b64[a >> 2];
		dst[j++] = b64[((a & 3) << 4) | (b >> 4)];
		if (i + 1 < src_len)
			dst[j++] = b64[(b & 15) << 2 | (c >> 6)];
		if (i + 2 < src_len)
			dst[j++] = b64[c & 63];
	}
	while (j % 4 != 0)
		dst[j++] = '=';
	dst[j++] = '\0';
}

//--------------------------------------------
void ws_handshake_encode(const char *key, char *hash)
{
	char buf[100], sha[20], b64_sha[sizeof(sha) * 2];
	SHA_CTX sha_ctx;

	strcpy((char *)buf, key);
	strcat((char *)buf, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	SHA1_Init(&sha_ctx);
	SHA1_Update(&sha_ctx, (unsigned char *) buf, (uint32_t)strlen(buf));
	SHA1_Final((unsigned char *) sha, &sha_ctx);

	base64_encode((unsigned char *) sha, sizeof(sha), b64_sha);

	strcpy(hash, (char *)b64_sha);
}



//--------------------------------------------
//** sockets

//--------------------------------------------
static int sock_recv(list_tcp_conn_t *conn, unsigned char **buf_addr, size_t size)
{
#ifdef USE_TLS_LIBRARY
	if (conn->ssl != NULL)
		return tls_recv(conn->ssl, buf_addr, size);
	else
#endif
		return recv(conn->sock, *buf_addr, (int)size, 0);
}

//--------------------------------------------
static int sock_send(list_tcp_conn_t *conn, const unsigned char *buf, size_t size)
{
#ifdef USE_TLS_LIBRARY
	if (conn->ssl != NULL)
		return tls_send(conn->ssl, buf, size);
	else
#endif
		return send(conn->sock, buf, (int)size, 0);
}

//--------------------------------------------
static int sock_timeout(SOCKET sock, int milliseconds)
{
#ifdef WIN32
	DWORD t = milliseconds;
#else
	struct timeval t;
	t.tv_sec = milliseconds / 1000;
	t.tv_usec = (milliseconds * 1000) % 1000000;
#endif
#if 0
	// https://www.google.com/#q=resource%20temporarily%20unavailable%20linux%20tcp%20socket
	// http://stackoverflow.com/questions/10318191/reading-socket-eagain-resource-temporarily-unavailable
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &t, sizeof(t)) ||
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *) &t, sizeof(t));
#endif
	return 	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *) &t, sizeof(t));
}

//--------------------------------------------
static void sock_close(SOCKET sock)
{
	if (sock != INVALID_SOCKET)
	{
		shutdown(sock, SHUT_RDWR);
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}


//--------------------------------------------
//** connections
#define conn_add_new(a) list_tcp_conn_add_new(&conns, a)
#define conn_find_addr(a) list_tcp_conn_find_addr(&conns, a)
#define conn_remove(a) list_tcp_conn_remove(&conns, a)
#define conn_remove_all() list_tcp_conn_remove_all(&conns)

//--------------------------------------------
static void recv_buf_free(list_tcp_conn_t *conn)
{
	if (conn->recv_buf != NULL)
	{
		free(conn->recv_buf);
		conn->recv_buf = NULL;
		conn->recv_cnt = 0;
		memset(&conn->ws_frame, 0, sizeof(ws_frame_t));
	}
}

//--------------------------------------------
static list_tcp_conn_t *conn_close(list_tcp_conn_t *conn)
{
	if (thread_state == THREAD_RUNNING)
		msg_tcp_mqtt_add_close_conn(&conn->addr);

#ifdef USE_TLS_LIBRARY
	if (conn->ssl != NULL)
	{
		tls_free(conn->ssl);
		conn->ssl = NULL;
	}
#endif

	sock_close(conn->sock);
	recv_buf_free(conn);
	return conn_remove(conn);
}

//--------------------------------------------
static int conn_check(list_tcp_conn_t *conn, int sock_recv_result)
{
#ifdef USE_TLS_LIBRARY
	if (conn->ssl != NULL)
	{
		// axTLS
		// > 0, then the handshaking is complete and we are returning the number of decrypted bytes.
		// = 0 if the handshaking stage is successful (but not yet complete).
		// < 0 if an error.
		// OpenSSL
		// > 0 the read operation was successful. The return value is the number of bytes
		// actually read from the TLS/SSL connection.
		// = 0 does not happen
		// < 0 the read operation was not successful, because either the connection was closed,
		// an error occurred or action must be taken by the calling process.
		if (sock_recv_result <= 0)
		{
			if (sock_recv_result < 0)
			{
				print_error_socket(__LINE__);
				conn_close(conn);
			}
			return -1;
		}
	}
	else
#endif
	{
		// > 0 the number of bytes received.
		// = 0 when a stream socket peer has performed an orderly shutdown.
		// < 0 if an error.
		if (sock_recv_result <= 0)
		{
			if (sock_recv_result < 0)
				print_error_socket(__LINE__);
			conn_close(conn);
			return -1;
		}
	}
	return 0;
}

//--------------------------------------------
static void conn_new(size_t num)
{
	int optval;
	int len;
	SOCKET sock;
	struct sockaddr_in addr;
	list_tcp_conn_t *conn;

	optval = 1;
	len = sizeof(struct sockaddr_in);

	if ((sock = accept(servs[num].sock, (struct sockaddr *)&addr, &len)) == INVALID_SOCKET)
	{
		print_error_socket(__LINE__);
		return;
	}

	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&optval, sizeof(optval));
	sock_timeout(sock, 10000); // 10 seconds


	if ((conn = conn_add_new(&addr)) == NULL)
	{
		// max connections exceeded
		sock_close(sock);
		return;
	}

#ifdef USE_TLS_LIBRARY
	if (servs[num].is_tls == 1)
	{
		SSL *ssl;

		if ((ssl = tls_server_new(servs[num].ssl_ctx, (int)sock)) == NULL)
		{
			// ssl error
			conn_remove(conn);
			sock_close(sock);
			return;
		}
		conn->ssl = ssl;
	}
#endif

	conn->sock = sock;
	if (servs[num].mode == MODE_MQTT)
		conn->state = HANDLE_MQTT_MESSAGE;
	else
		conn->state = HANDLE_HTTP_HEADER;
}



//--------------------------------------------
//** servers

//--------------------------------------------
static int serv_init(struct in_addr *in_addr)
{
	size_t cnt;
	struct sockaddr_in addr;

	for (cnt = 0; cnt < MQTT_TCP_SERVERS; ++cnt)
	{
		if (servs[cnt].port != 0)
		{
			if ((servs[cnt].sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			{
				print_error_socket(__LINE__);
				return -1;
			}

			addr.sin_family = AF_INET;
			addr.sin_port = htons(servs[cnt].port);
			addr.sin_addr = *in_addr;

			if (bind(servs[cnt].sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			{
				print_error_socket(__LINE__);
				return -1;
			}

			if (listen(servs[cnt].sock, SOMAXCONN) < 0)
			{
				print_error_socket(__LINE__);
				return -1;
			}

#ifdef USE_TLS_LIBRARY
			if (servs[cnt].is_tls == 1)
			{
				servs[cnt].ssl_ctx = tls_ctx_new();
				if (servs[cnt].ssl_ctx == NULL)
					dprintf("%s\n", "Failed to initialize TSL library.");
			}
#endif
		}
	}
	return 0;
}

//--------------------------------------------
static void serv_close(void)
{
	size_t cnt;
	list_tcp_conn_t *conn;

	conn = conns;
	while (conn != NULL)
		conn = conn_close(conn);

	for (cnt = 0; cnt < MQTT_TCP_SERVERS; ++cnt)
	{
#ifdef USE_TLS_LIBRARY
		if (servs[cnt].ssl_ctx != NULL)
			tls_ctx_free(servs[cnt].ssl_ctx);
#endif
		if (servs[cnt].sock != INVALID_SOCKET)
			closesocket(servs[cnt].sock);
	}
}


//--------------------------------------------
//** http header

#define MAX_HTTP_HEADER_NAME_SIZE	64
#define MAX_HTTP_HEADER_VALUE_SIZE	64

//--------------------------------------------
typedef struct
{
	char upgrade[MAX_HTTP_HEADER_VALUE_SIZE + 1];
	char connection[MAX_HTTP_HEADER_VALUE_SIZE + 1];
	char version[MAX_HTTP_HEADER_VALUE_SIZE + 1];
	char key[MAX_HTTP_HEADER_VALUE_SIZE + 1];
	char protocol[MAX_HTTP_HEADER_VALUE_SIZE + 1];
} ws_headers_t;

//--------------------------------------------
static void http_header_parse_token(const char *token, ws_headers_t *headers)
{
	char name[MAX_HTTP_HEADER_NAME_SIZE + 1];
	char value[MAX_HTTP_HEADER_VALUE_SIZE + 1];
	if (sscanf(token, "%"STRINGIFY(MAX_HTTP_HEADER_NAME_SIZE)"[^:]: %"STRINGIFY(MAX_HTTP_HEADER_VALUE_SIZE)"[^\r\n]", (char *)&name, (char *)&value) == 2)
	{
		if (strcmp(name, "Sec-WebSocket-Key") == 0)
			strcpy((char *)&headers->key, value);
		else if (strcmp(name, "Upgrade") == 0)
			strcpy((char *)&headers->upgrade, value);
		else if (strcmp(name, "Connection") == 0)
			strcpy((char *)&headers->connection, value);
		else if (strcmp(name, "Sec-WebSocket-Protocol") == 0)
			strcpy((char *)&headers->protocol, value);
		else if (strcmp(name, "Sec-WebSocket-Version") == 0)
			strcpy((char *)&headers->version, value);
	}
}

//--------------------------------------------
static int http_header_parse(char *buf, char *key)
{
	char seps[] = "\r\n";
	char *token;
	ws_headers_t headers = { 0 };

	token = strtok(buf, seps);
	while (token != NULL)
	{
		//	printf(" %s\n", token);
		http_header_parse_token(token, &headers);
		token = strtok(NULL, seps);
	}

	if (strstr(headers.connection, "Upgrade") != NULL &&
		(strcmp(headers.upgrade, "websocket") == 0 || strcmp(headers.upgrade, "Websocket") == 0) &&
		strcmp(headers.version, "13") == 0 &&
		strcmp(headers.protocol, "mqttv3.1") == 0 &&
		strlen(headers.key) != 0)
	{
		strcpy(key, headers.key);
		return 1;
	}
	return 0;
}

//--------------------------------------------
static int http_ws_response_send(list_tcp_conn_t *conn)
{
	char key[64];// = { 0 };
	char hash[64];
	char sbuf[256];
	if (http_header_parse(conn->recv_buf, (char *)key) == 1)
	{
		// this is websocket response
		ws_handshake_encode((const char *)key, (char *)hash);
		sprintf(sbuf,
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Protocol: mqttv3.1\r\n"
			"Sec-WebSocket-Accept: %s\r\n\r\n",
			hash);
		return sock_send(conn, sbuf, strlen(sbuf));
	}
	return 0;
}

//--------------------------------------------
static int http_header_detect(list_tcp_conn_t *conn)
{
	unsigned char *buf_end = strstr(conn->recv_buf, "\r\n\r\n");
	if (buf_end != NULL && (size_t)(buf_end - conn->recv_buf) < conn->recv_cnt)
		return 0;
	return -1;
}

//--------------------------------------------
static void http_header_handle(list_tcp_conn_t *conn)
{
	int res;
	int recv_cnt;
	unsigned char *recv_buf = sock_buf;

	recv_cnt = sock_recv(conn, &recv_buf, sizeof(sock_buf));
	// here recv_buf may or may not be equal to &sock_buf - it depends on the ssl library
	// recv_buf buffer may be overwritten, but only in the range from 0 to recv_cnt-1 bytes
	// don't free recv_buf - it's static or it's freed inside the ssl library

	// check the connection to close
	if (conn_check(conn, recv_cnt) == -1)
		return;

	if (conn->recv_cnt + recv_cnt > sizeof(sock_buf))
	{
		// too long header
		conn_close(conn);
		return;
	}

	if (conn->recv_buf == NULL)
		conn->recv_buf = (unsigned char *)malloc(sizeof(sock_buf));

	memcpy(conn->recv_buf + conn->recv_cnt, recv_buf, recv_cnt);
	conn->recv_cnt += recv_cnt;

	res = http_header_detect(conn);

	if (res == -1)
		return;

	if (http_ws_response_send(conn) == 0)
		conn_close(conn);
	else
	{
		recv_buf_free(conn);
		conn->state = HANDLE_WS_FRAME;
	}
}


//--------------------------------------------
//** websockets frame

//--------------------------------------------
static int ws_frame_send(list_tcp_conn_t *conn, unsigned char opcode, const unsigned char *pld_buf, size_t pld_size)
{
	int res;
	unsigned char hdr_buf[10];
	size_t hdr_size;

	hdr_buf[0] = 0x80 + (opcode & 0x0F);

	if (pld_size <= 125)
	{
		hdr_buf[1] = (unsigned char)pld_size;
		hdr_size = 2;
	}
	else if (pld_size <= 0xFFFF)
	{
		hdr_buf[1] = 126;
		*(uint16_t*)(hdr_buf + 2) = htons((uint16_t)pld_size);
		hdr_size = 4;
	}
	else
	{
		hdr_buf[1] = 127;
		*(uint32_t*)(hdr_buf + 2) = htonl((uint32_t)((uint64_t)pld_size >> 32));
		*(uint32_t*)(hdr_buf + 6) = htonl((uint32_t)(pld_size & 0xFFFFFFFF));
		hdr_size = 10;
	}
	res = sock_send(conn, hdr_buf, hdr_size);
	res = sock_send(conn, pld_buf, pld_size);
	return res;
}

//--------------------------------------------
static void ws_frame_decode(list_tcp_conn_t *conn)
{
	if (conn->ws_frame.mask)
	{
		size_t cnt;
		for (cnt = 0; cnt < conn->ws_frame.pld_size; ++cnt)
			*(conn->recv_buf + cnt) ^= conn->ws_frame.key[cnt & 3];
	}
}

//--------------------------------------------
static int ws_frame_process(list_tcp_conn_t *conn)
{
	if (conn->ws_frame.opcode == WS_OPCODE_PONG)
	{
		recv_buf_free(conn);
		return 0;
	}
	if (conn->ws_frame.opcode == WS_OPCODE_PING)
	{
		recv_buf_free(conn);
		ws_frame_send(conn, WS_OPCODE_PONG, NULL, 0);
		return 0;
	}
	if (conn->ws_frame.opcode == WS_OPCODE_BINARY)
	{
		msg_tcp_mqtt_add_packet(&conn->addr, conn->recv_buf, conn->recv_cnt);
		recv_buf_free(conn);
		return 0;
	}
	// WS_OPCODE_CONTINUATION, WS_OPCODE_TEXT, WS_OPCODE_CONNECTION_CLOSE are not processed
	conn_close(conn); // recv_buf_free() inside
	return -1;
}

//--------------------------------------------
static int ws_frame_parse(list_tcp_conn_t *conn, unsigned char *recv_buf, size_t *recv_size)
{
	size_t cnt;
	size_t offset;
	size_t add;
	size_t pld_size;
	unsigned char *pld_buf;

	ws_frame_t *ws_frame = &conn->ws_frame;

	if (ws_frame->hdr_cnt == 0 && *recv_size == 1)
	{
		ws_frame->hdr_cnt = 1;
		ws_frame->hdr_buf[0] = recv_buf[0];
		// it's only the part of the header
		return -1;
	}

	add = sizeof(ws_frame->hdr_buf) - ws_frame->hdr_cnt;
	cnt = add < *recv_size ? add : *recv_size;
	memcpy(ws_frame->hdr_buf + ws_frame->hdr_cnt, recv_buf, cnt);

	ws_frame->fin = ws_frame->hdr_buf[0] & 0x80 ? 1 : 0;
	ws_frame->opcode = ws_frame->hdr_buf[0] & 0x0F;
	ws_frame->mask = ws_frame->hdr_buf[1] & 0x80 ? 1 : 0;
	ws_frame->pld_size = ws_frame->hdr_buf[1] & 0x7F;

	if (ws_frame->pld_size > 0)
	{
		if (ws_frame->pld_size <= 125)
			offset = 2;
		else if (ws_frame->pld_size == 126)
			offset = 4;
		else
			offset = 10;

		ws_frame->hdr_size = offset + (ws_frame->mask ? 4 : 0);

		if (ws_frame->hdr_size > ws_frame->hdr_cnt + cnt)
		{
			ws_frame->hdr_cnt += cnt;
			// it's only the part of the header
			return -1;
		}

		pld_buf = recv_buf + (ws_frame->hdr_size - ws_frame->hdr_cnt); // обозвать hdr_cnt
		pld_size = *recv_size - (size_t)(pld_buf - recv_buf);
		ws_frame->hdr_cnt = ws_frame->hdr_size;

		// the entire header is read
		if (ws_frame->pld_size == 126)
			ws_frame->pld_size = (size_t)((((uint16_t)ws_frame->hdr_buf[2]) << 8) + ws_frame->hdr_buf[3]);
		if (ws_frame->pld_size == 127)
			ws_frame->pld_size = (size_t)(((uint64_t)ntohl(*(uint32_t *)&ws_frame->hdr_buf[2])) << 32) + 
			ntohl(*(uint32_t*)&ws_frame->hdr_buf[6]);

		if (ws_frame->mask)
			memcpy(&ws_frame->key, ws_frame->hdr_buf + offset, sizeof(ws_frame->key));

		conn->recv_buf = (unsigned char *)malloc(ws_frame->pld_size);
		conn->recv_cnt = pld_size < ws_frame->pld_size? pld_size : ws_frame->pld_size;
		memcpy(conn->recv_buf, pld_buf, conn->recv_cnt);

		if (ws_frame->pld_size > pld_size)
			// it's only the first part of the frame
			return -2;
		if (ws_frame->pld_size < pld_size)
		{
			// at least a part of one more frame
			memmove(recv_buf, recv_buf + ws_frame->hdr_size + ws_frame->pld_size, pld_size - ws_frame->pld_size);
			*recv_size = pld_size - ws_frame->pld_size;
			return 1;
		}
	}
	else
	{
		if (*recv_size < 6)
			// it's only the part of the header
			return -1;
	}

	// it's a whole frame
	return 0;
}

//--------------------------------------------
static void ws_frame_parse_loop(list_tcp_conn_t *conn, unsigned char *recv_buf, size_t recv_size)
{
	int res;
	size_t recv_cnt = recv_size;

	for (;;)
	{
		res = ws_frame_parse(conn, recv_buf, &recv_cnt);
		if (res < 0)
			// it's only a part of the frame
			return;
		// ready to decode the frame payload
		ws_frame_decode(conn);
		if (ws_frame_process(conn) == -1)
			// connection has been closed
			return;
		if (res == 0)
			// all frames have been processed
			return;
	}
}

//--------------------------------------------
static void ws_frame_handle(list_tcp_conn_t *conn)
{
	int recv_cnt;
	unsigned char *recv_buf = sock_buf;

	recv_cnt = sock_recv(conn, &recv_buf, sizeof(sock_buf));
	// here recv_buf may or may not be equal to &sock_buf - it depends on the ssl library
	// recv_buf buffer may be overwritten, but only in the range from 0 to recv_cnt-1 bytes
	// don't free recv_buf - it's static or it's freed inside the ssl library

	// check the connection to close
	if (conn_check(conn, recv_cnt) == -1)
		return;

	if (conn->recv_buf != NULL)
	{
		size_t next_cnt;
		size_t rem_cnt;

		next_cnt = (size_t)recv_cnt < conn->ws_frame.pld_size - conn->recv_cnt ?
			(size_t)recv_cnt : conn->ws_frame.pld_size - conn->recv_cnt;
		rem_cnt = (size_t)recv_cnt - next_cnt;

		// adding the next part of the frame
		memcpy(conn->recv_buf + conn->recv_cnt, recv_buf, next_cnt);
		conn->recv_cnt += next_cnt;
		if (conn->recv_cnt < conn->ws_frame.pld_size)
			// it's only a part of the frame
			return;
		// ready to decode the frame payload
		ws_frame_decode(conn);
		if (ws_frame_process(conn) == -1)
			// connection has been closed
			return;
		if (rem_cnt > 0)
		{
			// at least one more frame
			memmove(recv_buf, recv_buf + next_cnt, rem_cnt);
			ws_frame_parse_loop(conn, recv_buf, recv_cnt);
		}
	}
	else
		ws_frame_parse_loop(conn, recv_buf, recv_cnt);
}


//--------------------------------------------
//** mqtt message

//--------------------------------------------
static void mqtt_message_handle(list_tcp_conn_t *conn)
{
	int recv_cnt;
	unsigned char *recv_buf = sock_buf;

	recv_cnt = sock_recv(conn, &recv_buf, sizeof(sock_buf));
	// here recv_buf may or may not be equal to &sock_buf - it depends on the ssl library
	// recv_buf buffer may be overwritten, but only in the range from 0 to recv_cnt-1 bytes
	// don't free recv_buf - it's static or it's freed inside the ssl library

	// check the connection to close
	if (conn_check(conn, recv_cnt) == -1)
		return;

	if (conn->recv_cnt + recv_cnt > sizeof(sock_buf))
	{
		// too long header
		conn_close(conn);
		return;
	}

	if (conn->recv_buf == NULL)
		conn->recv_buf = (unsigned char *)malloc(sizeof(sock_buf));

	memcpy(conn->recv_buf + conn->recv_cnt, recv_buf, recv_cnt);
	conn->recv_cnt += recv_cnt;

	if (mqtt_packets_buffer_check(conn->recv_buf, conn->recv_cnt) < 0)
		return;

	msg_tcp_mqtt_add_packet(&conn->addr, conn->recv_buf, conn->recv_cnt);
	recv_buf_free(conn);
}


//--------------------------------------------
//** main thread

//--------------------------------------------
static void thread_run(void *param)
{
	for (;;)
	{
		int res;
		fd_set rd;
		size_t cnt;
		int rnum = INVALID_SOCKET;
		struct timeval tv = { 0, 10000 }; // 0.01 sec
		list_tcp_conn_t *conn;
		list_tcp_conn_t *next;

		if (thread_state == THREAD_STAYING)
			break;

		FD_ZERO(&rd);

		for (cnt = 0; cnt < MQTT_TCP_SERVERS; ++cnt)
		{
			if (servs[cnt].sock != INVALID_SOCKET)
			{
				FD_SET(servs[cnt].sock, &rd);
				rnum = (int)servs[cnt].sock > rnum ? (int)servs[cnt].sock : rnum;
			}
		}

		conn = conns;
		while (conn != NULL)
		{
			FD_SET(conn->sock, &rd);
			rnum = (int)conn->sock > rnum ? (int)conn->sock : rnum;
			conn = list_tcp_conn_next(conn);
		}

		res = select(rnum + 1, &rd, NULL, NULL, &tv);

		if (res < 0)
		{
			print_error_socket(__LINE__);
			break;
		}

		if (res == 0)
		{
			// timeout
			msg_mqtt_tcp_t *ms;
			if ((ms = msg_mqtt_tcp_get_first()) != NULL)
			{
				if ((conn = conn_find_addr(&ms->addr)) != NULL)
				{
					if (ms->close == 1)
						conn_close(conn);
					else
					{
						if (conn->state == HANDLE_MQTT_MESSAGE)
							sock_send(conn, ms->msg_buf, ms->msg_cnt);
						else
							ws_frame_send(conn, WS_OPCODE_BINARY, ms->msg_buf, ms->msg_cnt);
					}
				}
				else
					msg_tcp_mqtt_add_close_conn(&ms->addr);
				msg_mqtt_tcp_remove(ms);
			}
			continue;
		}

		for (cnt = 0; cnt < MQTT_TCP_SERVERS; ++cnt)
		{
			if (servs[cnt].sock != INVALID_SOCKET && FD_ISSET(servs[cnt].sock, &rd) != 0)
				conn_new(cnt);
		}

		conn = conns;
		while (conn != NULL)
		{
			next = list_tcp_conn_next(conn);
			if (FD_ISSET(conn->sock, &rd) != 0)
			{
				if (conn->state == HANDLE_HTTP_HEADER)
					http_header_handle(conn);
				if (conn->state == HANDLE_WS_FRAME)
					ws_frame_handle(conn);
				if (conn->state == HANDLE_MQTT_MESSAGE)
					mqtt_message_handle(conn);
			}
			conn = next;
		}
	}

	serv_close();
	thread_state = THREAD_STOPPED;
}

//--------------------------------------------
#ifdef WIN32
static unsigned __stdcall thread_launcher(void *param)
{
	thread_run(param);
	return 0;
}
#else
static void *thread_launcher(void *param)
{
	thread_run(param);
	return NULL;
}
#endif

//--------------------------------------------
int thread_tcp_start(void)
{
	pthread_t thread;
	void *param = NULL;
	struct in_addr addr;
	int res;

	res = get_device_ipaddress(thread_options->mqtt_iface, &addr);
	dprintf("%s: %s\n", thread_options->mqtt_iface, inet_ntoa(addr));
	if (res < 0)
	{
		print_error_socket(__LINE__);
		free(thread_options);
		return -1;
	}

	if (serv_init(&addr) < 0)
	{
		free(thread_options);
		return -1;
	}

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
	return 0;
}

//--------------------------------------------
void thread_tcp_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
	free(thread_options);
}

//--------------------------------------------
void thread_tcp_serv_setup(thread_tcp_options_t *options)
{
	size_t cnt = 0;

	thread_options = options;

	servs[cnt].is_tls = 0;
	servs[cnt].port = (unsigned short)thread_options->mqtt_port;
	servs[cnt].mode = MODE_MQTT;

#ifdef USE_TLS_LIBRARY
	++cnt;
	servs[cnt].is_tls = 1;
	servs[cnt].port = (unsigned short)thread_options->mqtt_tls_port;
	servs[cnt].mode = MODE_MQTT;
#endif

	++cnt;
	servs[cnt].is_tls = 0;
	servs[cnt].port = (unsigned short)thread_options->mqtt_ws_port;
	servs[cnt].mode = MODE_WS;

#ifdef USE_TLS_LIBRARY
	++cnt;
	servs[cnt].is_tls = 1;
	servs[cnt].port = (unsigned short)thread_options->mqtt_ws_tls_port;
	servs[cnt].mode = MODE_WS;
#endif

	for (cnt = 0; cnt < MQTT_TCP_SERVERS; ++cnt)
		servs[cnt].sock = INVALID_SOCKET;
}

