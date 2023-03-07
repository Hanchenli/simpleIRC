#include "reply.h"

/* Send all bytes from a socket */
int sendall(int s, sds buf, int *len, struct user_t * receiver)
{
    int total = 0;
    int bytesleft = *len;
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total;

    return n==-1?-1:0;
}

/* Wrapper function for sendall */
void safe_send(int socket, sds message, int length,
               struct user_t * receiver)
{
    pthread_mutex_lock(&receiver->individual_lock);
    if (sendall(socket, message, &length, receiver) == -1) {
        chilog(INFO, "We only sent %d bytes because of the error!\n", length);
    }
    pthread_mutex_unlock(&receiver->individual_lock);

}

/* Get the client hostname as a string */
sds get_client_host_info(struct sockaddr * client_addr)
{
    struct hostent *hostName;
    char hoststr[200];
    char portstr[200];
    int rc = getnameinfo((struct sockaddr *)client_addr,
                        sizeof(struct sockaddr_storage), hoststr, sizeof(hoststr), portstr,
                        sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);

    if(rc != 0) chilog(ERROR, "IP address missing");

    if ((hostName = gethostbyaddr(hoststr, sizeof (hoststr), AF_INET)) == NULL) {
        chilog(ERROR, "Not Able to Get Client Host Name");
        char *str = calloc(1, 128*(sizeof (char)));
        inet_ntop(AF_INET, &(client_addr->sa_data), str, INET_ADDRSTRLEN);
        return str;
    } else {
        chilog(INFO, "Message before getting address: %s",hostName->h_name);
        return hostName->h_name;
    }
}

/* Create prefix for a given code */
sds create_prefix(struct user_t * new_user, char* code)
{
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    if(new_user->nickname == NULL) {
        return sdscatprintf(sdsempty(), ":%s %s *", hostname, code);
    } else {
        return sdscatprintf(sdsempty(), ":%s %s %s",
                            hostname, code, new_user->nickname);
    }
}

/* Create prefix without code */
sds pure_prefix(struct user_t * new_user)
{
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    return sdscatprintf(sdsempty(), ":%s!%s@%s",
                        new_user->nickname, new_user->username, hostname);
}

void rpl_WELCOME(struct user_t * new_user, struct user_t* receiver,
                 struct sockaddr * client_addr)
{
    if(new_user == NULL) {
        chilog(ERROR, "Full name without nickname");
        exit(-1);
    }

    sds message = create_prefix(new_user, RPL_WELCOME);
    message = sdscatprintf(message,
                           " :Welcome to the Internet Relay Network %s!%s@%s \r\n",
                           new_user->nickname, new_user->username, get_client_host_info(client_addr));


    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);

    rpl_YOURHOST(new_user, receiver);
    rpl_CREATED(new_user, receiver);
    rpl_MYINFO(new_user, receiver);
}

void rpl_YOURHOST(struct user_t * new_user, struct user_t* receiver)
{
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    sds message = create_prefix(new_user, RPL_YOURHOST);
    message = sdscatprintf(message,
                           " :Your host is %s, running version chirc-0.1.0\r\n", hostname);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_CREATED(struct user_t * new_user, struct user_t* receiver)
{
    sds message = create_prefix(new_user, RPL_CREATED);
    message = sdscat(message,
                     " :This server was created 2022-10-10 19:46:41\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_MYINFO(struct user_t * new_user, struct user_t* receiver)
{
    sds message = create_prefix(new_user, RPL_MYINFO);
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    message = sdscat(message, " ");
    message = sdscat(message, hostname);
    message = sdscat(message, " chirc-0.1.0 ao mtov\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NICKNAMEINUSE (sds nickname, struct user_t* receiver,
                        struct sockaddr * client_addr)
{
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    sds message = sdsnew(":");
    message = sdscat(message, hostname);
    message = sdscat(message, " 433 * ");
    message = sdscat(message, nickname);
    message = sdscat(message, " :Nickname is already in use\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NONICKNAMEGIVEN (sds filler, struct user_t * new_user, struct user_t *receiver)
{
    sds message = create_prefix(new_user, ERR_NONICKNAMEGIVEN);
    message = sdscat(message, " :No nickname given");

    message = sdscat(message, "\r\n");


    safe_send(receiver->client_socket, message, sdslen(message), receiver);


    sdsfree(message);
}

void err_NEEDMOREPARAMS (sds in_command, struct user_t * new_user,
                         struct user_t* receiver)
{
    sds message = create_prefix(new_user, ERR_NEEDMOREPARAMS);
    message = sdscatprintf(message,
                           " %s :Not enough parameters\r\n", in_command);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_ALREADYREGISTRED (struct user_t* receiver,
                           struct sockaddr * client_addr)
{
    sds message = create_prefix(receiver, ERR_ALREADYREGISTRED);
    message = sdscat(message,
                     " :Unauthorized command (already registered)\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NOTEXTTOSEND (sds filler, struct user_t *new_user, struct user_t* receiver)
{
    sds message = create_prefix(new_user, ERR_NOTEXTTOSEND);
    message = sdscat(message, " :No text to send\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NORECIPIENT (sds command_name, struct user_t *new_user,
                      struct user_t* receiver)
{
    sds message = create_prefix(new_user, ERR_NORECIPIENT);
    message = sdscatprintf(message,
                           " :No recipient given (%s)\r\n", command_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NOSUCHNICK (struct user_t *new_user, struct user_t* receiver,
                     sds recepient_nick)
{
    sds message = create_prefix(new_user, ERR_NOSUCHNICK);
    message = sdscatprintf(message,
                           " %s :No such nick/channel\r\n", recepient_nick);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NOTREGISTERED(struct user_t *new_user, struct user_t* receiver)
{
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    sds message = sdsempty();
    if(new_user->nickname == NULL) {
        message = sdscatprintf(message, ":%s %s * :You have not registered\r\n",
                               hostname, ERR_NOTwelcomed);
    } else {
        message = sdscatprintf(message, ":%s %s %s :You have not registered\r\n",
                               hostname, ERR_NOTwelcomed, new_user->nickname);
    }

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_UNKNOWNCOMMAND (sds command_name, struct user_t *new_user,
                         struct user_t* receiver)
{
    if (!new_user->registered) return; //silently drop
    sds message = create_prefix(new_user, ERR_UNKNOWNCOMMAND);
    message = sdscatprintf(message, " %s :Unknown command\r\n", command_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NOTONCHANNEL (sds channel_name, struct user_t *new_user,
                       struct user_t* receiver)
{
    sds message = create_prefix(new_user, ERR_NOTONCHANNEL);
    message = sdscatprintf(message,
                           " %s :You're not on that channel\r\n", channel_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NOSUCHCHANNEL (sds channel_name, struct user_t *new_user,
                        struct user_t* receiver)
{
    sds message = create_prefix(new_user, ERR_NOSUCHCHANNEL);
    message = sdscatprintf(message, " %s :No such channel\r\n", channel_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_PASSWDMISMATCH(struct user_t *new_user, struct user_t* receiver)
{
    sds message = create_prefix(new_user, ERR_PASSWDMISMATCH);
    message = sdscat(message, " :Password incorrect\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void privmsg(sds send_message, sds receiver_nick, struct user_t* receiver,
             struct user_t * new_user)
{
    sds message = sdscatprintf(pure_prefix(new_user), " PRIVMSG %s :%s\r\n",
                               receiver_nick, send_message);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void notice(sds send_message, sds receiver_nick, struct user_t* receiver,
            struct user_t * new_user)
{
    sds message = sdscatprintf(pure_prefix(new_user), " NOTICE %s :%s\r\n",
                               receiver_nick, send_message);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}


void pong(struct user_t* receiver, struct sockaddr * client_addr)
{
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    sds message = sdscatprintf(sdsempty(), "PONG %s", hostname);
    message = sdscatprintf(message, "\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}


void quit_ERR(struct user_t* receiver, struct sockaddr * client_addr,
              sds quit_message)
{
    sds client_hostname = get_client_host_info(client_addr);
    sds leave_command = "Client Quit";
    if(quit_message!= NULL) {
        leave_command = quit_message;
    }
    sds message = sdscatprintf(sdsempty(), "ERROR :Closing Link: %s (%s)\r\n",
                               client_hostname, leave_command);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}


/* Five LUSERS replies */


void rpl_LUSERCLIENT(int num_of_users, int num_of_services,
                     struct user_t* receiver, struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_LUSERCLIENT);
    message = sdscatprintf(message,
                           " :There are %d users and %d services on %d servers\r\n",
                           num_of_users, num_of_services, 1);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_LUSEROP(int num_of_operators,  struct user_t* receiver,
                 struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_LUSEROP);
    message = sdscatprintf(message,
                           " %d :operator(s) online\r\n", num_of_operators);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_LUSERUNKNOWN(int num_of_unknown, struct user_t* receiver,
                      struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_LUSERUNKNOWN);
    message = sdscatprintf(message,
                           " %d :unknown connection(s)\r\n", num_of_unknown);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_LUSERCHANNELS(int num_of_channels, struct user_t* receiver,
                       struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_LUSERCHANNELS);
    message = sdscatprintf(message,
                           " %d :channels formed\r\n", num_of_channels);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_LUSERME(int num_of_users, struct user_t* receiver,
                 struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_LUSERME);
    message = sdscatprintf(message, " :I have %d clients and %d servers\r\n",
                           num_of_users, 1);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}


/* Three WHOIS */

void rpl_WHOISUSER(sds nickname, sds username, sds fullname,
                   struct user_t* receiver, struct sockaddr * client_addr,
                   struct sockaddr * other_addr, struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_WHOISUSER);
    sds other_server = get_client_host_info(other_addr);
    message = sdscatprintf(message, " %s %s %s * :%s\r\n", nickname,
                           username, other_server, fullname);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_WHOISSERVER(sds nickname, struct user_t* receiver,
                     struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_WHOISSERVER);
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    message = sdscatprintf(message, " %s %s :%s\r\n", nickname,
                           hostname, "best_irc_server_ever");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_ENDOFWHOIS(sds nickname, struct user_t* receiver,
                    struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_ENDOFWHOIS);
    message = sdscatprintf(message, " %s :End of WHOIS list\r\n", nickname);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}


void rpl_JOIN(sds channel_name, struct user_t* receiver,
              struct user_t * new_user)
{
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    sds message = sdscatprintf(sdsempty(), ":%s!%s@%s JOIN %s\r\n",
                               new_user->nickname, new_user->username, hostname, channel_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_NAMREPLY(struct channel_t *cur_channel, struct user_t* receiver,
                  struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_NAMREPLY);
    message = sdscatprintf(message, " = %s :=", cur_channel->channel_name);
    for(int i = 0; i < cur_channel->user_count; i++) {
        message = sdscat(message, cur_channel->channel_users[i]->nickname);
        message = sdscat(message, " ");
    }
    sdstrim(message, " ");
    message = sdscat(message, "\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_ENDOFNAMES(sds channel_name, struct user_t* receiver,
                    struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_ENDOFNAMES);
    message = sdscatprintf(message, " %s :End of NAMES list\r\n", channel_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_PART(sds channel_name, sds parting_message, struct user_t* receiver,
              struct user_t * new_user)
{
    sds message = pure_prefix(new_user);
    message = sdscatprintf(message, " PART %s", channel_name);
    if(parting_message != NULL) {
        message = sdscatprintf(message, " :%s", parting_message);
    }
    message = sdscat(message, "\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

/* MODE */
void rpl_YOUREOPER(struct user_t* receiver, struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_YOUREOPER);
    message = sdscat(message, " :You are now an IRC operator\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_UMODEUNKNOWNFLAG(struct user_t* receiver, struct user_t * new_user)
{
    sds message = create_prefix(new_user, ERR_UMODEUNKNOWNFLAG);
    message = sdscat(message, " :Unknown MODE flag\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_CHANOPRIVSNEEDED(sds channel_name, struct user_t* receiver,
                          struct user_t * new_user)
{
    sds message = create_prefix(new_user, ERR_CHANOPRIVSNEEDED);
    message = sdscat(message, " :You're not channel operator\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_CANNOTSENDTOCHAN(sds channel_name, struct user_t* receiver,
                          struct user_t * new_user)
{
    sds message = create_prefix(new_user, ERR_CANNOTSENDTOCHAN);
    message = sdscatprintf(message, " %s :Cannot send to channel\r\n", channel_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_UNKNOWNMODE(sds mode, struct user_t* receiver, struct user_t * new_user)
{
    sds message = create_prefix(new_user, ERR_UNKNOWNMODE);
    message = sdscatprintf(message, "%s :is unknown mode char to me for \r\n channel", mode);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_USERNOTINCHANNEL(sds nickname, sds channel_name, struct user_t* receiver,
                          struct user_t * new_user)
{
    sds message = create_prefix(new_user, ERR_USERNOTINCHANNEL);
    message = sdscatprintf(message, "%s %s :They aren't on that channel\r\n",
                           nickname, channel_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

/* LIST */

void rpl_LIST(sds channel_name, int num_of_users, struct user_t* receiver,
              struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_LIST);
    message = sdscatprintf(message, " %s %d :\r\n", channel_name, num_of_users);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_LISTEND(struct user_t* receiver, struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_LISTEND);
    message = sdscat(message, " :End of LIST\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

/* New Two WHOIS */
void rpl_WHOISOPERATOR(sds nickname, struct user_t* receiver,
                       struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_WHOISOPERATOR);
    message = sdscatprintf(message, " %s :is an IRC operator\r\n", nickname);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_WHOISCHANNELS(sds nickname, sds channel_name, struct user_t* receiver,
                       struct user_t * new_user)
{
    sds message = create_prefix(new_user, RPL_WHOISCHANNELS);
    message = sdscatprintf(message, " %s :%s\r\n", nickname, channel_name);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_NOMOTD(struct user_t* receiver, struct user_t * new_user)
{
    sds message = create_prefix(new_user, ERR_NOMOTD);
    message = sdscat(message, " :MOTD File is missing\r\n");

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_NICKCHANGE(sds new_nick, struct user_t* receiver,
                    struct user_t * new_user)
{
    sds message = sdscatprintf(pure_prefix(new_user), " NICK :%s\r\n", new_nick);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void rpl_MODE(sds channel_name, sds flag, sds target_nickname,
              struct user_t* receiver, struct user_t * new_user)
{
    sds message = sdscatprintf(pure_prefix(new_user), " MODE %s %s %s\r\n",
                               channel_name, flag, target_nickname);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}


void quit_ERR_relay(struct user_t* receiver, struct user_t * new_user,
                    sds quit_message)
{
    sds leave_command = "Client Quit";
    if(quit_message!= NULL) {
        leave_command = quit_message;
    }
    sds message = sdscatprintf(pure_prefix(new_user),
                               " QUIT :%s\r\n", leave_command);

    safe_send(receiver->client_socket, message, sdslen(message), receiver);

    sdsfree(message);
}

void err_SILENTLYDROP (sds filler, struct user_t * new_user, struct user_t* receiver)
{
    return;
}