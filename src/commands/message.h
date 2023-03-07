#ifndef MESSAGE_H
#define MESSAGE_H
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
#include "parse.h"

/*
 * command_privmsg - Send message to channel or user
 *
 * argv[0]: nickname of the user
 *
 * argv[1]: message to send
 *
 * client_addr: client socket address
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_privmsg(sds* argv, int client_socket, struct sockaddr * client_addr,
                     struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_notice - Send message to channel or user without error
 *
 * argv[0]: nickname of the user
 *
 * argv[1]: message to send
 *
 * client_addr: client socket address
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_notice(sds* argv, int client_socket, struct sockaddr * client_addr,
                    struct user_t *single_user, struct server_ctx_t *ctx);


/*
 * command_ping - Send back PONG
 *
 * argv: filler variable
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
void command_ping(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_pong - Ignore the Ping message
 *
 * argv: filler variable
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
void command_pong(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_luser - Send server information
 *
 * client_socket: socket of the client
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_lusers(sds *argv, int client_socket, struct sockaddr * client_addr,
                    struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_whois - Get information with respect to nickname
 *
 * argv[0]: nickname of the user
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
void command_whois(sds *argv, int client_socket, struct sockaddr * client_addr,
                   struct user_t *single_user, struct server_ctx_t *ctx);

#endif