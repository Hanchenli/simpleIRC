#include "server.h"

typedef void(*command_handler_function)(sds*, int, struct sockaddr*, struct user_t*, struct server_ctx_t*);
typedef void(*error_handler_function)(sds, struct user_t*, struct user_t*);

void incoming_command(sds command_str, int client_socket,
                      struct sockaddr * client_addr, struct user_t *single_user,
                      struct worker_args *wa)
{

    command_handler_function handlers[] = {
        command_nick,
        command_user,
        command_privmsg,
        command_quit,
        command_oper,
        command_ping,
        command_pong,
        command_lusers,
        command_whois,
        command_notice,
        command_join,
        command_part,
        command_mode,
        command_list
    };
    int num_command_handlers = sizeof(handlers) / sizeof(command_handler_function);

    error_handler_function error_handlers[] = {
        err_NONICKNAMEGIVEN,
        err_NOTEXTTOSEND,
        err_NEEDMOREPARAMS,
        err_NORECIPIENT,
        err_UNKNOWNCOMMAND,
        err_UNKNOWNMODE,
        err_UNKNOWNCOMMAND
    };
    int num_error_handlers = sizeof(error_handlers) / sizeof(error_handler_function);

    struct parsed_command_t *real_command = parse_string(command_str);
    chilog(INFO, "Incoming command of type %d is: %s",
           real_command->type, command_str);

    /*Special case for not-registered users*/
    if(real_command->type != COMMAND_NICK && real_command->type != COMMAND_USER
            && real_command->type + OFFSET_TO_ERROR_FUNCTION < 0
            && !single_user->registered) {
        err_NOTREGISTERED(single_user, single_user);
        return;
    }

    /*First see if it is a command, if it is not, offset to error handler*/
    if (real_command->type>=0 && real_command->type < num_command_handlers) {
        handlers[real_command->type](real_command->argv,
                                     client_socket, client_addr, single_user, wa->ctx);
    } else if (real_command->type + OFFSET_TO_ERROR_FUNCTION >= 0
               && real_command->type + OFFSET_TO_ERROR_FUNCTION < num_error_handlers) {

        error_handlers[real_command->type + OFFSET_TO_ERROR_FUNCTION](
            real_command->argv[0], single_user, single_user
        );
    }
}

void *service_single_client(void *args)
{
    struct worker_args *wa = (struct worker_args*) args;

    char buffer[100+1];
    int client_socket = wa->socket, nbytes;
    struct sockaddr *client_addr = wa->client_addr;
    sds command_str = sdsempty();
    struct user_t single_user;
    single_user.username = NULL;
    single_user.nickname = NULL;
    single_user.fullname = NULL;
    single_user.client_socket = -1;
    single_user.comed = 0;
    single_user.is_operator = 0;
    single_user.registered = 0;
    single_user.channel = NULL;
    pthread_mutex_init(&single_user.individual_lock, NULL);


    while (1) {
        nbytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (nbytes == 0) {
            chilog(ERROR, "Client closed the connection");
            close(client_socket);
            pthread_exit("Client closed the connection");
        } else if (nbytes == -1) {
            chilog(ERROR, "Socket recv() function failed");
            close(client_socket);
            pthread_exit("Socket recv() failed");
        }
        single_user.client_socket = client_socket;

        int start = 0;
        for (int i = 0; i < nbytes; i++) {
            if(buffer[i] == '\n') {
                command_str = sdscat(command_str,
                                     sdsnewlen(buffer + start, i - start + 1 ));
                int length = sdslen(command_str);
                if(length >= 2 && command_str[length - 1] == '\n'
                        && command_str[length -2] == '\r') {

                    char *save_for_free = command_str;
                    command_str = sdsnewlen(command_str, length - 2);
                    sdsfree(save_for_free);
                }
                incoming_command(command_str, client_socket, client_addr,
                                 &single_user, wa);
                start = i+1;
                command_str = sdsempty();
            }
        }

        if(start < nbytes)
            command_str = sdscat(command_str, sdsnewlen(buffer + start,
                                 nbytes - start));
    }
}