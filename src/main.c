/*
 *
 *  chirc: a simple multi-threaded IRC server
 *
 *  This module provides the main() function for the server,
 *  and parses the command-line arguments to the chirc executable.
 *
 */

/*
 *  Copyright (c) 2011-2020, The University of Chicago
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or withsend
 *  modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  - Neither the name of The University of Chicago nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software withsend specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY send OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "sds.h"
#include "uthash.h"
#include "utarray.h"
#include "parse.h"
#include "server.h"

#include "log.h"


int main(int argc, char *argv[])
{

    int opt;
    char *port = "6667", *passwd = NULL, *servername = NULL, *network_file = NULL;
    int verbosity = 0;

    while ((opt = getopt(argc, argv, "p:o:s:n:vqh")) != -1)
        switch (opt) {
        case 'p':
            port = strdup(optarg);
            break;
        case 'o':
            passwd = strdup(optarg);
            break;
        case 's':
            servername = strdup(optarg);
            break;
        case 'n':
            if (access(optarg, R_OK) == -1) {
                printf("ERROR: No such file: %s\n", optarg);
                exit(-1);
            }
            network_file = strdup(optarg);
            break;
        case 'v':
            verbosity++;
            break;
        case 'q':
            verbosity = -1;
            break;
        case 'h':
            printf("Usage: chirc -o OPER_PASSWD [-p PORT] [-s SERVERNAME] [-n NETWORK_FILE] [(-q|-v|-vv)]\n");
            exit(0);
            break;
        default:
            fprintf(stderr, "ERROR: Unknown option -%c\n", opt);
            exit(-1);
        }

    if (!passwd) {
        fprintf(stderr, "ERROR: You must specify an operator password\n");
        exit(-1);
    }

    if (network_file && !servername) {
        fprintf(stderr, "ERROR: If specifying a network file, you must also specify a server name.\n");
        exit(-1);
    }

    /* Set logging level based on verbosity */
    switch(verbosity) {
    case -1:
        chirc_setloglevel(QUIET);
        break;
    case 0:
        chirc_setloglevel(INFO);
        break;
    case 1:
        chirc_setloglevel(DEBUG);
        break;
    case 2:
        chirc_setloglevel(TRACE);
        break;
    default:
        chirc_setloglevel(TRACE);
        break;
    }

    /*parse argv to get password*/

    pthread_t worker_thread;
    struct worker_args *wa;
    struct user_t* users = NULL;
    struct channel_t* channels = NULL;
    struct server_ctx_t *ctx = calloc(1, sizeof(struct server_ctx_t));
    ctx->num_connections = 0;
    ctx->clients = 0;
    ctx->operators = 0;
    ctx->active = 0;
    pthread_mutex_init(&ctx->lock, NULL);
    ctx->channels = &channels;
    ctx->users = &users;

    ctx->password = passwd;

    char buffer[100+1];
    int nbytes;
    int server_socket;
    int client_socket = -1;
    int yes = 1;

    struct sockaddr_storage *client_addr;
    socklen_t sin_size = sizeof(struct sockaddr_storage);
    struct addrinfo hints,*res, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(INADDR_ANY, port, &hints, &res) != 0) {
        chilog(ERROR, "getaddrinfo() failed");
        exit(-1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            chilog(ERROR, "Could not open socket");
            continue;
        }

        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            chilog(ERROR, "Socket setsockopt() failed");
            close(server_socket);
            continue;
        }

        if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
            chilog(ERROR, "Socket bind() failed");
            close(server_socket);
            continue;
        }

        if (listen(server_socket, 5) == -1) {
            chilog(ERROR, "Socket listen() failed");
            close(server_socket);
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    while(1) {
        client_addr = calloc(1, sin_size);

        if ((client_socket = accept(server_socket, (struct sockaddr *) 
            client_addr, &sin_size)) == -1) {
            chilog(ERROR, "Socket accept() failed");
            free(client_addr);
            close(server_socket);
            exit(-1);
        }

        wa = calloc(1, sizeof(struct worker_args));
        wa->socket = client_socket;
        wa->client_addr = (struct sockaddr *) client_addr;
        wa->ctx = ctx;


        char hoststr[200];
        char portstr[200];
        int rc = getnameinfo((struct sockaddr *)client_addr, sin_size, hoststr,
                            sizeof(hoststr), portstr, sizeof(portstr), 
                            NI_NUMERICHOST | NI_NUMERICSERV);
        chilog(INFO, "Client accepted: %s", hoststr);

        ctx->clients+=1;
        if (pthread_create(&worker_thread, NULL, service_single_client, wa) != 0) {
            ctx->clients-=1;
            chilog(ERROR, "Could not create a worker thread");
            free(client_addr);
            free(wa);
            close(client_socket);
            close(server_socket);
            return EXIT_FAILURE;
        }
    }

    close(server_socket);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
    return 0;
}

