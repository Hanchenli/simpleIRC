#include<parse.h>
#include "log.h"

struct parsed_command_t* parse_string(char* line)
{
    int count;
    struct parsed_command_t* res = malloc(sizeof(struct parsed_command_t));

    sdstrim(line, " ");
    if(sdslen(line) == 0) {
        res->type = ERRORTYPE_SILENTDROP;
        res->argv = malloc(sizeof(char*));
        res->argv[0] = NULL;
        return res;
    }

    char firstfour[5];
    memcpy(firstfour, &line[0], 4);
    firstfour[4] = '\0';

    if(!strncmp(firstfour, "NICK", MAXIMUM_STR_LENGTH)) {
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if (count == 1) {
            res->type = ERRORTYPE_NONICKNAMEGIVEN;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
        } else if (count == 2) {
            res->type = COMMAND_NICK;
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        } else {
            chilog(ERROR, "parsed_command(): NICK: to much arguments");
        }
        sdsfreesplitres(tokens, count);

    } else if (!strncmp(firstfour, "USER", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_USER;
        char* e = strchr(line, ':');
        if(!e) {
            chilog(ERROR, "USER: no column");
            res->type = ERRORTYPE_NEEDMOREPARAMS;
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = sdsnew("USER");
            return res;
        }

        int idx = (int)(e - line);
        int first_len = idx;
        int second_len = sdslen(line) - idx;

        char* first_part = malloc(first_len * sizeof(char));
        char* second_part = malloc(second_len * sizeof(char));
        strncpy(first_part, line, first_len);
        strncpy(second_part, line+idx+1, second_len-1);
        first_part[first_len-1] = '\0';
        second_part[second_len-1] = '\0';

        char** tokens = sdssplitlen(first_part, first_len, " ", 1, &count);
        free(first_part);

        if(count != 4) {
            res->type = ERRORTYPE_NEEDMOREPARAMS;
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = sdsnew("USER");
            sdsfreesplitres(tokens, count);
            return res;
        }

        res->argc = 2;
        res->argv = malloc(2 * sizeof(char*));
        res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        res->argv[1] = second_part;
        sdsfreesplitres(tokens, count);

    } else if (!strncmp(firstfour, "PRIV", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_PRIVMSG;

        char** tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);

        if(strncmp(tokens[0], "PRIVMSG", MAXIMUM_STR_LENGTH)) {
            res->type = ERRORTYPE_UNKNOWNCOMMAND;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = strndup(tokens[0], sdslen(tokens[0]));
            sdsfreesplitres(tokens, count);
            return res;
        } else if (count == 2) {
            res->type = ERRORTYPE_NOTEXTTOSEND;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
            sdsfreesplitres(tokens, count);
            return res;
        } else if (count == 1) {
            res->type = ERRORTYPE_NORECIPIENT;
            res->argc = 1;
            res->argv = malloc(1 * sizeof(char*));
            res->argv[0] = strndup(tokens[0], sdslen(tokens[0]));
            sdsfreesplitres(tokens, count);
            return res;
        }

        char* e = strchr(line, ':');
        int idx = (int)(e - line);
        int first_len = idx;
        int second_len = sdslen(line) - idx;

        char* first_part = malloc(first_len * sizeof(char));
        char* second_part = malloc(second_len * sizeof(char));
        strncpy(first_part, line, first_len);
        strncpy(second_part, line+idx+1, second_len-1);
        first_part[first_len-1] = '\0';
        second_part[second_len-1] = '\0';

        tokens = sdssplitlen(first_part, first_len, " ", 1, &count);
        free(first_part);

        res->argc = 2;
        res->argv = malloc(2 * sizeof(char*));
        res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        res->argv[1] = second_part;
        sdsfreesplitres(tokens, count);

    } else if (!strncmp(firstfour, "QUIT", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_QUIT;
        char* e = strchr(line, ':');
        if(!e) {
            res->argc = 0;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
            return res;
        }

        int idx = (int)(e - line);
        int second_len = sdslen(line) - idx;
        char* second_part = malloc(second_len * sizeof(char));
        strncpy(second_part, line+idx+1, second_len-1);
        second_part[second_len-1] = '\0';

        res->argc = 1;
        res->argv = malloc(2 * sizeof(char*));
        res->argv[0] = second_part;

    } else if (!strncmp(firstfour, "OPER", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_OPER;
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if(count < 3) {
            res->type = ERRORTYPE_NEEDMOREPARAMS;
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = sdsnew("OPER");
            sdsfreesplitres(tokens, count);
            return res;
        }

        res->argc = 2;
        res->argv = malloc(2 * sizeof(char*));
        res->argv[0] = tokens[1];
        res->argv[1] = tokens[2];

    } else if (!strncmp(firstfour, "PING", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_PING;
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if(count == 2) {
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        } else if(count == 3) {
            res->argc = 2;
            res->argv = malloc(2 * sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
            res->argv[1] = strndup(tokens[2], sdslen(tokens[2]));
        } else {
            chilog(ERROR, "PING: number of arguments not 1 or 2");
        }
        sdsfreesplitres(tokens, count);

    } else if (!strncmp(firstfour, "PONG", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_PONG;
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if(count == 2) {
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        } else if(count == 3) {
            res->argc = 2;
            res->argv = malloc(2 * sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
            res->argv[1] = strndup(tokens[2], sdslen(tokens[2]));
        }
        sdsfreesplitres(tokens, count);

    } else if (!strncmp(firstfour, "LUSE", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_LUSERS;
        if(strncmp(line, "LUSERS", MAXIMUM_STR_LENGTH)) {
            res->type = ERRORTYPE_UNKNOWNCOMMAND;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
        }

    } else if (!strncmp(firstfour, "WHOI", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_WHOIS;
        if(sdslen(line) < 5 || line[4] != 'S') {
            res->type = ERRORTYPE_UNKNOWNCOMMAND;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
        }
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if(count == 2) {
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        } else {
            res->argc = 0;
            res->argv = NULL;
        }

    } else if (!strncmp(firstfour, "NOTI", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_NOTICE;
        char* e = strchr(line, ':');
        if(!e) {
            res->type = ERRORTYPE_SILENTDROP;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
            return res;
        }

        int idx = (int)(e - line);
        int first_len = idx;
        int second_len = sdslen(line) - idx;

        if(second_len <= 1) {
            res->type = ERRORTYPE_NOTEXTTOSEND;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
            return res;
        }

        char* first_part = malloc(first_len * sizeof(char));
        char* second_part = malloc(second_len * sizeof(char));
        strncpy(first_part, line, first_len);
        strncpy(second_part, line+idx+1, second_len-1);
        first_part[first_len-1] = '\0';
        second_part[second_len-1] = '\0';

        char** tokens = sdssplitlen(first_part, first_len, " ", 1, &count);
        free(first_part);

        if(strncmp(tokens[0], "NOTICE", MAXIMUM_STR_LENGTH)) {
            res->type = ERRORTYPE_UNKNOWNCOMMAND;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = strndup(tokens[0], sdslen(tokens[0]));
            sdsfreesplitres(tokens, count);
            return res;
        } else if(count < 2) {
            res->type = ERRORTYPE_SILENTDROP;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
            sdsfreesplitres(tokens, count);
            return res;
        }

        res->argc = 2;
        res->argv = malloc(2 * sizeof(char*));
        res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        res->argv[1] = second_part;
        sdsfreesplitres(tokens, count);

    } else if (!strncmp(firstfour, "JOIN", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_JOIN;
        char** tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if(count < 2) {
            res->type = ERRORTYPE_NEEDMOREPARAMS;
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = sdsnew("JOIN");
            sdsfreesplitres(tokens, count);
            return res;
        }
        res->argc = 1;
        res->argv = malloc(sizeof(char *));
        res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        sdsfreesplitres(tokens, count);

    } else if (!strncmp(firstfour, "PART", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_PART;
        char* e = strchr(line, ':');
        if(!e) {
            char** tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
            if(count < 2) {
                res->type = ERRORTYPE_NEEDMOREPARAMS;
                res->argc = 1;
                res->argv = malloc(2*sizeof(char*));
                res->argv[0] = sdsnew("PART");
                res->argv[1] = NULL;
                sdsfreesplitres(tokens, count);
                return res;
            }
            res->argc = 1;
            res->argv = malloc(2*sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
            res->argv[1] = NULL;
            sdsfreesplitres(tokens, count);
        } else {
            int idx = (int)(e - line);
            int first_len = idx;
            int second_len = sdslen(line) - idx;

            char* first_part = malloc(first_len * sizeof(char));
            char* second_part = malloc(second_len * sizeof(char));
            strncpy(first_part, line, first_len);
            strncpy(second_part, line+idx+1, second_len-1);
            first_part[first_len-1] = '\0';
            second_part[second_len-1] = '\0';

            char** tokens = sdssplitlen(first_part, first_len, " ", 1,
                                        &count);
            free(first_part);

            if(count < 2) {
                res->type = ERRORTYPE_NEEDMOREPARAMS;
                res->argc = 1;
                res->argv = malloc(sizeof(char*));
                res->argv[0] = sdsnew("PART");
                res->argv[1] = NULL;
                sdsfreesplitres(tokens, count);
                return res;
            }

            res->argc = 2;
            res->argv = malloc(2 * sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
            res->argv[1] = second_part;
            sdsfreesplitres(tokens, count);
        }

    } else if(!strncmp(firstfour, "MODE", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_MODE;
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if(count < 4) {
            res->type = ERRORTYPE_NEEDMOREPARAMS;
            res->argc = 1;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = sdsnew("MODE");
            sdsfreesplitres(tokens, count);
            return res;
        } else if(sdslen(tokens[2]) != 2 || (tokens[2][0] != '+' &&
                                             tokens[2][1] != '-') || (tokens[2][1] != 'o')) {
            res->type = ERRORTYPE_UNKNOWNMODE;
            res->argv = malloc(sizeof(char*));
            res->argv[0] = NULL;
            sdsfreesplitres(tokens, count);
            return res;
        }

        res->argc = 3;
        res->argv = malloc(3 * sizeof(char*));
        res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        res->argv[1] = strndup(tokens[2], sdslen(tokens[2]));
        res->argv[2] = strndup(tokens[3], sdslen(tokens[3]));
        sdsfreesplitres(tokens, count);

    } else if(!strncmp(firstfour, "LIST", MAXIMUM_STR_LENGTH)) {
        res->type = COMMAND_LIST;
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        if(count == 1) {
            res->argc = 0;
            res->argv = malloc(1 * sizeof(char*));
            res->argv[0] = NULL;
            sdsfreesplitres(tokens, count);
            return res;
        } else if(count == 2) {
            res->argc = 1;
            res->argv = malloc(1 * sizeof(char*));
            res->argv[0] = strndup(tokens[1], sdslen(tokens[1]));
        }
        sdsfreesplitres(tokens, count);

    } else {
        res->type = ERRORTYPE_UNKNOWN;
        sds* tokens = sdssplitlen(line, sdslen(line), " ", 1, &count);
        res->argc = 1;
        res->argv = malloc(1 * sizeof(char*));
        res->argv[0] = strndup(tokens[0], sdslen(tokens[0]));
        sdsfreesplitres(tokens, count);
    }

    return res;
}

void free_parsed_command(struct parsed_command_t* cmd)
{
    for(int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    free(cmd);
}

