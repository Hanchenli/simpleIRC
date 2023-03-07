#ifndef REGISTER_H
#define REGISTER_H
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

#include "uthash.h"
#include "log.h"
#include "reply.h"
#include "sds.h"
#include "register.h"
#include "message.h"

/*
 * command_nick - Process NICK command and update number of connections
 *
 * argv: argv[0] is the nickname
 *
 * client_socket: socket of the client
 *
 * client_addr: client socket address
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_nick(sds* argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);


/*
 * command_user - Process USER command, if double registration then get error
 *
 * argv[0]: username of the user
 *
 * argv[1]: fullname of the user
 *
 * client_socket: socket of the client
 *
 * client_addr: client socket address
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_user(sds* argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_quit - Process QUIT command and quits channel.
 *
 * argv[0]: quit message
 *
 * client_socket: socket of the client
 *
 * client_addr: client socket address
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_quit(sds* argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

#endif