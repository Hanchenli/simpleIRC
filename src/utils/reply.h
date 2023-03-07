/*
 *  chirc: a simple multi-threaded IRC server
 *
 *  Reply codes
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

#ifndef REPLY_H_
#define REPLY_H_

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
#include "sds.h"
#include "log.h"
#include "uthash.h"
#include "parse.h"

#define MAXIMUM_STR_LENGTH      500

#define RPL_WELCOME             "001"
#define RPL_YOURHOST            "002"
#define RPL_CREATED             "003"
#define RPL_MYINFO              "004"

#define RPL_LUSERCLIENT         "251"
#define RPL_LUSEROP             "252"
#define RPL_LUSERUNKNOWN        "253"
#define RPL_LUSERCHANNELS       "254"
#define RPL_LUSERME             "255"

#define RPL_AWAY                "301"
#define RPL_UNAWAY              "305"
#define RPL_NOWAWAY             "306"

#define RPL_WHOISUSER           "311"
#define RPL_WHOISSERVER         "312"
#define RPL_WHOISOPERATOR       "313"
#define RPL_WHOISIDLE           "317"
#define RPL_ENDOFWHOIS          "318"
#define RPL_WHOISCHANNELS       "319"

#define RPL_WHOREPLY            "352"
#define RPL_ENDOFWHO            "315"

#define RPL_LIST                "322"
#define RPL_LISTEND             "323"

#define RPL_CHANNELMODEIS       "324"

#define RPL_NOTOPIC             "331"
#define RPL_TOPIC               "332"

#define RPL_NAMREPLY            "353"
#define RPL_ENDOFNAMES          "366"

#define RPL_MOTDSTART           "375"
#define RPL_MOTD                "372"
#define RPL_ENDOFMOTD           "376"

#define RPL_YOUREOPER           "381"

#define ERR_NOSUCHNICK          "401"
#define ERR_NOSUCHSERVER        "402"
#define ERR_NOSUCHCHANNEL       "403"
#define ERR_CANNOTSENDTOCHAN    "404"
#define ERR_NORECIPIENT         "411"
#define ERR_NOTEXTTOSEND        "412"
#define ERR_UNKNOWNCOMMAND      "421"
#define ERR_NOMOTD              "422"
#define ERR_NONICKNAMEGIVEN     "431"
#define ERR_NICKNAMEINUSE       "433"
#define ERR_USERNOTINCHANNEL    "441"
#define ERR_NOTONCHANNEL        "442"
#define ERR_NOTwelcomed       "451"
#define ERR_NEEDMOREPARAMS      "461"
#define ERR_ALREADYREGISTRED    "462"
#define ERR_PASSWDMISMATCH      "464"
#define ERR_UNKNOWNMODE         "472"
#define ERR_NOPRIVILEGES        "481"
#define ERR_CHANOPRIVSNEEDED    "482"
#define ERR_UMODEUNKNOWNFLAG    "501"
#define ERR_USERSDONTMATCH      "502"

struct user_t {
    int id;
    sds nickname;
    sds username;
    sds fullname;
    int client_socket;
    int comed;
    int registered;
    int is_operator;
    sds channel;
    pthread_mutex_t individual_lock;

    UT_hash_handle hh;
};

struct channel_t {
    sds channel_name;
    struct user_t **channel_users;
    int user_count;
    int operator_socket;
    int is_in_o_mode;

    UT_hash_handle hh;
};

struct server_ctx_t {
    int num_connections;
    int clients;
    int operators;
    int active;
    pthread_mutex_t lock;
    struct user_t **users;
    struct channel_t **channels;
    sds password;
};

/*
 * safe_send - Send all bytes from the socket
 *
 * socket: socket of the client
 *
 * message: message to send
 *
 * length: length of the message
 *
 * receiver: receiver of the message
 *
 * Returns: Nothing
 */
void safe_send(int socket, sds message, int length, struct user_t * receiver);

void err_NONICKNAMEGIVEN (sds filler, struct user_t * new_user,
                          struct user_t* receiver);
void err_SILENTLYDROP (sds filler, struct user_t * new_user, struct user_t* receiver);
void err_NICKNAMEINUSE (sds nickname, struct user_t* receiver,
                        struct sockaddr * client_addr);
void err_NEEDMOREPARAMS (sds in_command, struct user_t * new_user,
                         struct user_t* receiver);
void err_ALREADYREGISTRED (struct user_t* receiver, struct sockaddr * client_addr);
void err_NOTEXTTOSEND (sds filler, struct user_t *new_user, struct user_t* receiver);
void err_NORECIPIENT (sds command_name, struct user_t *new_user,
                      struct user_t* receiver);
void err_NOSUCHNICK (struct user_t *new_user, struct user_t* receiver,
                     sds recepient_nick);
void err_NOTREGISTERED(struct user_t *new_user, struct user_t* receiver);
void err_UNKNOWNCOMMAND (sds command_name, struct user_t *new_user,
                         struct user_t* receiver);
void err_NOTONCHANNEL (sds channel_name, struct user_t *new_user,
                       struct user_t* receiver);
void err_NOSUCHCHANNEL (sds channel_name, struct user_t *new_user,
                        struct user_t* receiver);
void err_PASSWDMISMATCH(struct user_t *new_user, struct user_t* receiver);
void err_UMODEUNKNOWNFLAG(struct user_t* receiver, struct user_t * new_user);
void err_CHANOPRIVSNEEDED(sds channel_name, struct user_t* receiver,
                          struct user_t * new_user);
void err_CANNOTSENDTOCHAN(sds channel_name, struct user_t* receiver,
                          struct user_t * new_user);
void err_UNKNOWNMODE(sds mode, struct user_t* receiver, struct user_t * new_user);
void err_USERNOTINCHANNEL(sds nickname, sds channel_name, struct user_t* receiver,
                          struct user_t * new_user);
void err_NOMOTD(struct user_t* receiver, struct user_t * new_user);

void pong(struct user_t* receiver, struct sockaddr * client_addr);
void quit_ERR(struct user_t* receiver, struct sockaddr * client_addr,
              sds quit_message);
void privmsg(sds send_message, sds receiver_nick, struct user_t* receiver,
             struct user_t * new_user);
void notice(sds send_message, sds receiver_nick, struct user_t* receiver,
            struct user_t * new_user);


void rpl_WELCOME(struct user_t * new_user, struct user_t* receiver,
                 struct sockaddr * client_addr);
void rpl_YOURHOST(struct user_t * new_user, struct user_t* receiver);
void rpl_CREATED(struct user_t * new_user, struct user_t* receiver);
void rpl_MYINFO(struct user_t * new_user, struct user_t* receiver);
void rpl_LUSERCLIENT(int num_of_users, int num_of_services, struct user_t* receiver,
                     struct user_t * new_user);
void rpl_LUSEROP(int num_of_operators,  struct user_t* receiver,
                 struct user_t * new_user);
void rpl_LUSERUNKNOWN(int num_of_unknown, struct user_t* receiver,
                      struct user_t * new_user);
void rpl_LUSERCHANNELS(int num_of_channels, struct user_t* receiver,
                       struct user_t * new_user);
void rpl_LUSERME(int num_of_users, struct user_t* receiver, struct user_t * new_user);
void rpl_WHOISUSER(sds nickname, sds username, sds fullname,
                   struct user_t* receiver,
                   struct sockaddr * client_addr, struct sockaddr * other_addr,
                   struct user_t * new_user);
void rpl_WHOISSERVER(sds nickname, struct user_t* receiver, struct user_t * new_user);
void rpl_ENDOFWHOIS(sds nickname, struct user_t* receiver, struct user_t * new_user);
void rpl_JOIN(sds channel_name, struct user_t* receiver, struct user_t * new_user);
void rpl_NAMREPLY(struct channel_t *cur_channel, struct user_t* receiver,
                  struct user_t * new_user);
void rpl_ENDOFNAMES(sds channel_name, struct user_t* receiver,
                    struct user_t * new_user);
void rpl_PART(sds channel_name, sds parting_message, struct user_t* receiver,
              struct user_t * new_user);
void rpl_YOUREOPER(struct user_t* receiver, struct user_t * new_user);
void rpl_LIST(sds channel_name, int num_of_users, struct user_t* receiver,
              struct user_t * new_user);
void rpl_LISTEND(struct user_t* receiver, struct user_t * new_user);
void rpl_WHOISOPERATOR(sds nickname, struct user_t* receiver,
                       struct user_t * new_user);
void rpl_WHOISCHANNELS(sds nickname, sds channel_name, struct user_t* receiver,
                       struct user_t * new_user);
void rpl_NICKCHANGE(sds new_nick, struct user_t* receiver, struct user_t * new_user);
void rpl_MODE(sds channel_name, sds flag, sds target_nickname,
              struct user_t* receiver, struct user_t * new_user);
void quit_ERR_relay(struct user_t* receiver, struct user_t * new_user,
                    sds quit_message);


#endif /* REPLY_H_ */
