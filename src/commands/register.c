#include"register.h"

void command_nick(sds* argv, int client_socket,
                  struct sockaddr * client_addr, struct user_t *single_user,
                  struct server_ctx_t *ctx)
{
    int reg_record = single_user->registered;
    sds nickname = argv[0];

    pthread_mutex_t *mutex = &(ctx->lock);
    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;
    pthread_mutex_unlock(mutex);

    struct user_t *temp = NULL;
    if (*users != NULL) {
        pthread_mutex_lock(mutex);
        HASH_FIND_STR(*users, nickname, temp);
        pthread_mutex_unlock(mutex);
    }
    if(single_user->comed == 0) {
        single_user->comed = 1;
        pthread_mutex_lock(mutex);
        *active +=1;
        pthread_mutex_unlock(mutex);
    }

    if (temp != NULL) {
        err_NICKNAMEINUSE(nickname, single_user, client_addr);
    } else {
        if (single_user->nickname != NULL) {
            pthread_mutex_lock(mutex);
            HASH_FIND_STR(*users, single_user->nickname, temp);
            pthread_mutex_unlock(mutex);
        }
        if (temp != NULL) {
            pthread_mutex_lock(mutex);
            HASH_DELETE(hh, *users, single_user);
            pthread_mutex_unlock(mutex);
            if(single_user->registered) {
                struct channel_t *iterator, *tmp;
                pthread_mutex_lock(mutex);
                if (*channels != NULL)
                    HASH_ITER(hh, *channels, iterator, tmp) {
                    int in_channel = 0;
                    for (int i = 0; i < iterator->user_count; i++) {
                        if(iterator->channel_users[i] == single_user)
                            in_channel = 1;

                        if(in_channel == 1) for (int i = 0;
                                                     i < iterator->user_count; i++)
                                rpl_NICKCHANGE(nickname,
                                               iterator->channel_users[i],
                                               single_user);
                    }
                }

                pthread_mutex_unlock(mutex);
            }
        }


        single_user->nickname = nickname;
        if(single_user->username != NULL) {
            pthread_mutex_lock(mutex);
            HASH_ADD_KEYPTR(hh, *users, single_user->nickname,
                            strlen(single_user->nickname), single_user);

            if(!single_user->registered) {
                rpl_WELCOME(single_user, single_user, client_addr);
                single_user->registered = 1;
                *num_of_connections += 1;
            }
            pthread_mutex_unlock(mutex);
        }
    }

    if(reg_record == 0 && single_user->registered == 1) {
        command_lusers(argv,client_socket, client_addr, single_user, ctx);
        err_NOMOTD(single_user, single_user);
    }
}


void command_user(sds *argv, int client_socket,
                  struct sockaddr * client_addr, struct user_t *single_user,
                  struct server_ctx_t *ctx)
{
    sds username = argv[0];
    sds fullname = argv[1];
    int reg_record = single_user->registered;

    pthread_mutex_t *mutex = &(ctx->lock);

    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;

    if(single_user->comed == 0) {
        single_user->comed = 1;
        *active +=1;
    }
    pthread_mutex_unlock(mutex);

    if(single_user->registered == 1) {
        err_ALREADYREGISTRED(single_user, client_addr);
        return;
    }

    single_user->username = username;
    single_user->fullname = fullname;

    if(single_user->nickname != NULL) {
        pthread_mutex_lock(mutex);
        HASH_ADD_KEYPTR(hh, *users, single_user->nickname,
                        strlen(single_user->nickname), single_user);
        pthread_mutex_unlock(mutex);

        rpl_WELCOME(single_user, single_user, client_addr);

        pthread_mutex_lock(mutex);
        single_user->registered = 1;
        *num_of_connections += 1;
        pthread_mutex_unlock(mutex);

        sds temp_str = sdsnew(single_user->nickname);
        struct user_t *temp = NULL;
        pthread_mutex_lock(mutex);
        HASH_FIND_STR(*users, temp_str, temp);
        pthread_mutex_unlock(mutex);
        if (temp == NULL) {
            chilog(ERROR, "what is wrong");
            chilog(ERROR, (*users)->nickname);
            chilog(ERROR, single_user->nickname);
        }
    }

    if(reg_record == 0 && single_user->registered == 1) {
        command_lusers(argv,client_socket, client_addr, single_user, ctx);
        err_NOMOTD(single_user, single_user);
    }
}

void command_quit(sds* argv, int client_socket,
                  struct sockaddr * client_addr, struct user_t *single_user,
                  struct server_ctx_t *ctx)
{
    sds quit_message = argv[0];

    pthread_mutex_t *mutex = &(ctx->lock);
    pthread_mutex_lock(mutex);
    struct channel_t ** channels = ctx->channels;
    struct user_t ** users = ctx->users;
    int *num_of_connections = &ctx->num_connections;
    int *active = &ctx->active;
    pthread_mutex_unlock(mutex);

    struct channel_t *iterator, *tmp;

    pthread_mutex_lock(mutex);
    if (*channels != NULL) {
        HASH_ITER(hh, *channels, iterator, tmp) {
            int in_channel = 0;
            for (int i = 0; i < iterator->user_count; i++) {
                if(iterator->channel_users[i] == single_user) {
                    in_channel = 1;
                }
                if(in_channel == 1 && i != iterator->user_count - 1) {
                    iterator->channel_users[i]
                        = iterator->channel_users[i+1];
                }
            }
            if (in_channel) {
                iterator->user_count--;
                if (iterator->user_count == 0) {
                    HASH_DELETE(hh, *channels, iterator);
                } else {
                    pthread_mutex_unlock(mutex);
                    for (int i = 0; i < iterator->user_count; i++) {
                        quit_ERR_relay(iterator->channel_users[i],
                                       single_user, quit_message);
                    }
                    pthread_mutex_lock(mutex);
                }
            }
        }

    }
    *num_of_connections -= 1;
    pthread_mutex_unlock(mutex);

    chilog(ERROR, "before quit_ERR");
    quit_ERR(single_user, client_addr, quit_message);
    pthread_mutex_destroy(&single_user->individual_lock);

    close(client_socket);
    pthread_exit(0);
}
