#include "channel.h"

void command_join(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx)
{
    sds channel_name = argv[0];
    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;
    pthread_mutex_unlock(mutex);

    struct channel_t *temp = NULL;
    pthread_mutex_lock(mutex);
    if (*channels != NULL) {
        HASH_FIND_STR(*channels, channel_name, temp);
    }

    if(temp == NULL) {
        struct channel_t *new = malloc(sizeof(struct channel_t));
        new->channel_name = channel_name;
        new->user_count = 1;

        HASH_ADD_KEYPTR(hh, *channels, new->channel_name,
                        strlen(new->channel_name), new);
        new->channel_users = malloc(30 * sizeof(struct user_t*));
        if(new->channel_users == NULL) {
            chilog(ERROR, "Not Able to Malloc List of Users");
        }

        new->channel_users[0] = single_user;
        new->operator_socket = single_user->client_socket;
        new->is_in_o_mode = 0;
        pthread_mutex_unlock(mutex);

        rpl_JOIN(channel_name, single_user, single_user);
        rpl_NAMREPLY(new, single_user, single_user);
        rpl_ENDOFNAMES(new->channel_name, single_user, single_user);
        chilog(INFO, "Cur_user_is: %s  User connected is: %d\n",
               single_user->nickname, new->user_count);
    } else {
        for (int i = 0; i < temp->user_count; i++) {
            if(!strncmp(temp->channel_users[i]->nickname,
                        single_user->nickname,
                        MAXIMUM_STR_LENGTH)) {
                return;
            }
        }
        temp->user_count += 1;
        temp->channel_users[temp->user_count - 1] = single_user;
        pthread_mutex_unlock(mutex);

        for (int i = 0; i < temp->user_count; i++) {
            rpl_JOIN(channel_name, temp->channel_users[i], single_user);
        }
        rpl_NAMREPLY(temp, single_user, single_user);
        rpl_ENDOFNAMES(temp->channel_name, single_user, single_user);
    }
}

void command_oper(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx)
{
    sds password = argv[1];
    pthread_mutex_t *mutex = &(ctx->lock);

    sds real_password = ctx->password;
    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;
    int *num_of_oper = &ctx->operators;
    pthread_mutex_unlock(mutex);

    if (strncmp(password, real_password, MAXIMUM_STR_LENGTH) == 0) {
        pthread_mutex_lock(mutex);
        *num_of_oper += 1;
        single_user->is_operator = 1;
        pthread_mutex_unlock(mutex);
        rpl_YOUREOPER(single_user, single_user);
    } else {
        err_PASSWDMISMATCH(single_user, single_user);
    }
}

void command_mode(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx)
{
    sds channel_name = argv[0];
    sds flag = argv[1];
    sds nickname = argv[2];
    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;

    struct channel_t *temp = NULL;
    if (*channels != NULL) {
        HASH_FIND_STR(*channels, channel_name, temp);
    }
    pthread_mutex_unlock(mutex);

    if(temp == NULL) {
        err_NOSUCHCHANNEL(channel_name, single_user, single_user);
    } else {
        int in_channel = 0;
        pthread_mutex_lock(mutex);
        for (int i = 0; i < temp->user_count; i++) {
            if(temp->channel_users[i] == single_user) {
                in_channel = 1;
            }
        }
        pthread_mutex_unlock(mutex);

        if (!in_channel) {
            err_USERNOTINCHANNEL(single_user->nickname, channel_name,
                                 single_user, single_user);
        } else {
            pthread_mutex_lock(mutex);
            if (temp->operator_socket != single_user->client_socket &&
                    single_user->is_operator == 0) {
                pthread_mutex_unlock(mutex);
                err_CHANOPRIVSNEEDED(channel_name, single_user,
                                     single_user);
            } else {
                if (flag[0] == '+') {
                    in_channel = 0;
                    for (int i = 0; i < temp->user_count; i++) {
                        if(strncmp(temp->channel_users[i]->nickname,
                                   nickname, MAXIMUM_STR_LENGTH) == 0) {
                            in_channel = 1;
                        }
                    }

                    if(in_channel == 1) {
                        for (int i = 0; i < temp->user_count; i++) {
                            rpl_MODE(channel_name, flag, nickname,
                                     temp->channel_users[i], single_user);
                        }
                        pthread_mutex_unlock(mutex);
                    } else {
                        pthread_mutex_unlock(mutex);
                        err_USERNOTINCHANNEL(nickname, channel_name,
                                             single_user, single_user);
                    }
                } else {
                    pthread_mutex_lock(mutex);
                    for (int i = 0; i < temp->user_count; i++) {
                        rpl_MODE(channel_name, flag, nickname,
                                 temp->channel_users[i], single_user);
                    }
                    pthread_mutex_unlock(mutex);
                }
            }
        }
    }
}

void command_part(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx)
{
    sds channel_name = argv[0];
    sds message = argv[1];

    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;
    struct channel_t *temp = NULL;

    if (*channels != NULL) {
        HASH_FIND_STR(*channels, channel_name, temp);
    }
    pthread_mutex_unlock(mutex);

    if(temp == NULL) {
        err_NOSUCHCHANNEL(channel_name, single_user, single_user);
    } else {
        int in_channel = 0;
        struct user_t *cur_user = NULL;
        pthread_mutex_lock(mutex);
        for (int i = 0; i < temp->user_count; i++) {
            if(temp->channel_users[i] == single_user) {
                in_channel = 1;
            }
            if(in_channel == 1 && i != temp->user_count - 1) {
                temp->channel_users[i] = temp->channel_users[i+1];
                chilog(INFO, "moving %d", i);
            }
        }
        pthread_mutex_unlock(mutex);

        if (!in_channel) {
            err_NOTONCHANNEL(channel_name, single_user, single_user);
        } else {
            chilog(INFO, "User %s leaves channel %s",
                   single_user->nickname, channel_name);
            pthread_mutex_lock(mutex);
            temp->user_count-=1;
            for (int i = 0; i < temp->user_count; i++) {
                rpl_PART(channel_name, message, temp->channel_users[i],
                         single_user);
            }
            pthread_mutex_unlock(mutex);
            rpl_PART(channel_name, message, single_user, single_user);

            pthread_mutex_lock(mutex);
            if (temp->user_count == 0) {
                HASH_DELETE(hh, *channels, temp);
            }
            pthread_mutex_unlock(mutex);
        }
    }

}

void command_list(sds *argv, int client_socket, struct sockaddr * client_addr,
                  struct user_t *single_user, struct server_ctx_t *ctx)
{
    sds channel_name = argv[0];
    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;

    if(channel_name != NULL) {
        struct channel_t *temp = NULL;
        if (*channels != NULL) {
            HASH_FIND_STR(*channels, channel_name, temp);
        }
        pthread_mutex_unlock(mutex);

        rpl_LIST(channel_name, temp->user_count, single_user,
                 single_user);
        rpl_LISTEND(single_user, single_user);
    } else {
        struct channel_t *iterator;
        for (iterator = *channels; iterator != NULL;
                iterator = iterator->hh.next) {
            rpl_LIST(iterator->channel_name, iterator->user_count,
                     single_user, single_user);
        }
        pthread_mutex_unlock(mutex);
        rpl_LISTEND(single_user, single_user);
    }
}
