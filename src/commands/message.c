#include "message.h"

void command_privmsg(sds* argv, int client_socket, struct sockaddr* client_addr,
                     struct user_t *single_user, struct server_ctx_t *ctx)
{
    sds nickname = argv[0];
    sds message = argv[1];

    pthread_mutex_t *mutex = &(ctx->lock);
    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    pthread_mutex_unlock(mutex);
    if(nickname[0] == '#') {
        struct channel_t *temp = NULL;

        if(*channels != NULL) {
            pthread_mutex_lock(mutex);
            HASH_FIND_STR(*channels, nickname, temp);

            if(temp != NULL) {
                int in_channel = 0;
                for (int i = 0; i < temp->user_count; i++) {
                    if(temp->channel_users[i] == single_user) {
                        in_channel = 1;
                    }
                }
                pthread_mutex_unlock(mutex);

                if (!in_channel) {
                    err_CANNOTSENDTOCHAN(temp->channel_name,
                                         single_user, single_user);
                } else {
                    pthread_mutex_lock(mutex);
                    for(int i = 0; i < temp->user_count; i++) {
                        if(temp->channel_users[i] != single_user) {
                            privmsg(message, nickname, temp->channel_users[i],
                                    single_user);
                        }
                    }
                    pthread_mutex_unlock(mutex);
                }
                return;
            }

            pthread_mutex_unlock(mutex);
        }

        err_NOSUCHNICK(single_user, single_user, nickname);
        return;
    }


    struct user_t * temp = NULL;

    pthread_mutex_lock(mutex);
    if (*users != NULL) HASH_FIND_STR(*users, nickname, temp);
    pthread_mutex_unlock(mutex);

    if(temp == NULL) {
        err_NOSUCHNICK(single_user, single_user, nickname);
    } else {
        privmsg(message, nickname, temp, single_user);
    }
}

void command_notice(sds *argv, int client_socket,
                    struct sockaddr * client_addr, struct user_t *single_user,
                    struct server_ctx_t *ctx)
{
    sds nickname = argv[0];
    sds message = argv[1];
    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;
    pthread_mutex_unlock(mutex);

    if(nickname[0] == '#') {
        struct channel_t *temp = NULL;
        pthread_mutex_lock(mutex);
        if(*channels != NULL) {
            HASH_FIND_STR(*channels, nickname, temp);

            if(temp != NULL) {
                int in_channel = 0;
                for (int i = 0; i < temp->user_count; i++) {
                    if(temp->channel_users[i] == single_user) {
                        in_channel = 1;
                    }
                }

                if (in_channel)
                    for(int i = 0; i < temp->user_count; i++) {
                        if(temp->channel_users[i] != single_user) {
                            notice(message, nickname, temp->channel_users[i],
                                   single_user);
                        }
                    }

                return;
            }
        }
        pthread_mutex_unlock(mutex);

        return;
    }

    struct user_t * temp = NULL;

    pthread_mutex_lock(mutex);
    if (*users != NULL) HASH_FIND_STR(*users, nickname, temp);
    pthread_mutex_unlock(mutex);

    if(temp) {
        notice(message, nickname, temp, single_user);
    }
}

void command_ping(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx)
{

    pthread_mutex_t *mutex = &(ctx->lock);

    if(single_user->registered == 0) {
        err_NOTREGISTERED(single_user, single_user);
    }

    pong(single_user, client_addr);

}

void command_pong(sds * argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx)
{
    pthread_mutex_t *mutex = &(ctx->lock);

    if(single_user->registered == 0) {
        err_NOTREGISTERED(single_user, single_user);
    }

}

void command_lusers(sds *argv, int client_socket, struct sockaddr * client_addr,
                    struct user_t *single_user, struct server_ctx_t *ctx)
{
    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;

    int *connections = &ctx->num_connections;
    int *operators = &ctx->operators;
    int *clients = &ctx->clients;
    int *active = &ctx->active;
    pthread_mutex_unlock(mutex);

    if(single_user->registered == 0) {
        err_NOTREGISTERED(single_user, single_user);
    }

    int num_channels = 0;
    struct channel_t *iterator, *tmp;
    pthread_mutex_lock(mutex);
    if (*channels != NULL)
        HASH_ITER(hh, *channels, iterator, tmp) {
        num_channels += 1;
    }
    pthread_mutex_unlock(mutex);

    rpl_LUSERCLIENT(*connections, 0, single_user, single_user);
    rpl_LUSEROP(*operators, single_user, single_user);
    rpl_LUSERUNKNOWN(*clients - *active, single_user, single_user);
    rpl_LUSERCHANNELS(num_channels, single_user, single_user);
    rpl_LUSERME(*active, single_user, single_user);
}

void command_whois(sds* argv, int client_socket, struct sockaddr * client_addr,
                   struct user_t *single_user, struct server_ctx_t *ctx)
{
    if(argv == NULL) return;

    sds nickname = argv[0];

    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    pthread_mutex_unlock(mutex);


    struct user_t * temp = NULL;
    pthread_mutex_lock(mutex);
    if (*users != NULL) HASH_FIND_STR(*users, nickname, temp);
    pthread_mutex_unlock(mutex);
    if(temp == NULL) {
        err_NOSUCHNICK(single_user, single_user, nickname);
    } else {
        rpl_WHOISUSER(temp->nickname, temp->username, temp->fullname,
                      single_user, NULL, client_addr, single_user);
        rpl_WHOISSERVER(temp->nickname, single_user, single_user);
        if(single_user->is_operator)
            rpl_WHOISOPERATOR(nickname, single_user, single_user);

        struct channel_t *iterator, *tmp;

        pthread_mutex_lock(mutex);
        if (*channels != NULL)
            HASH_ITER(hh, *channels, iterator, tmp) {
            rpl_WHOISCHANNELS(nickname, iterator->channel_name,
                              single_user, single_user);
        }
        pthread_mutex_unlock(mutex);

        rpl_ENDOFWHOIS(temp->nickname, single_user, single_user);
    }
}