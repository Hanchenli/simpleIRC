#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include "log.h"
#include "reply.h"
#include "sds.h"
#include "parse.h"
#include "register.h"
#include "message.h"
#include "channel.h"

struct worker_args {
    int socket;
    struct sockaddr *client_addr;
    struct server_ctx_t *ctx; // context
};

/*
 * service_single_client - Function for the server thread
 *
 * args: Arguments for the server thread
 *
 * Returns: nothing.
 */
void *service_single_client(void *args);

#endif