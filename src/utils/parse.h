#ifndef PARSE_H
#define PARSE_H

#include "sds.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "reply.h"


/*

These MACROS are used for communication between the parsed_command_t with
server. Thus server knows the type of the command.
 */
#define COMMAND_NICK            0
#define COMMAND_USER            1
#define COMMAND_PRIVMSG         2
#define COMMAND_QUIT            3
#define COMMAND_OPER            4
#define COMMAND_PING            5
#define COMMAND_PONG            6
#define COMMAND_LUSERS           7
#define COMMAND_WHOIS           8
#define COMMAND_NOTICE          9
#define COMMAND_JOIN            10
#define COMMAND_PART            11
#define COMMAND_MODE            12
#define COMMAND_LIST            13
#define OFFSET_TO_ERROR_FUNCTION -14
#define ERRORTYPE_NONICKNAMEGIVEN 14
#define ERRORTYPE_NOTEXTTOSEND    15
#define ERRORTYPE_NEEDMOREPARAMS  16
#define ERRORTYPE_NORECIPIENT     17
#define ERRORTYPE_UNKNOWNCOMMAND  18
#define ERRORTYPE_UNKNOWNMODE     19
#define ERRORTYPE_UNKNOWN         20
#define ERRORTYPE_SILENTDROP      21


struct parsed_command_t {
    int type;
    int argc;
    sds* argv;
};

/*
 * parse_string - Parse a string into a parsed_command struct
 *
 * line: Raw command string
 *
 * Returns: A parsed_command struct
 */
struct parsed_command_t* parse_string(char* line);

/*
 * free_parsed_command - Free the parsed_command struct
 *
 * cmd: a pointer to the parsed_command struct
 *
 * Returns: nothing
 */
void free_parsed_command(struct parsed_command_t* cmd);

#endif