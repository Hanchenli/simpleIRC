#ifndef CHANNEL_H
#define CHANNEL_H
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
 * command_join - Join a channel
 *
 * argv[0]: name of the channel
 *
 * client_socket: client socket
 *
 * client_addr: filler
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_join(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_oper - Get operator access
 *
 * argv[1]: password of the channel
 *
 * client_socket: client socket
 *
 * single_user: pointer to the current user
 *
 * client_addr: filler variable
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_oper(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_mode - Change channel mode
 *
 * argv[0]: name of the channel
 *
 * argv[1]: flag of the oper
 *
 * argv[2]: nickname of user
 *
 * client_socket: client socket
 *
 * client_addr: filler variable
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_mode(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_part - Leave the channel
 *
 * argv[0]: name of the channel
 *
 * argv[1]: parting message
 *
 * client_socket: client socket
 *
 * client_addr: filler variable
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_part(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

/*
 * command_list - List specific channel if given one argument, otherwise list all
 *
 * channel_name: name of the channel
 *
 * client_socket: client socket
 *
 * single_user: pointer to the current user
 *
 * ctx: server context
 *
 * Returns: Nothing
 */
void command_list(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx);

#endif