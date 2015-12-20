/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2014 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _CLI_PARSER_H_
#define _CLI_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FOREACH(THING, DO) THING(DO)

#define TO_INT(x) atoi(x)
#define TO_BOOL(x) x != NULL ? 1:0
#define STRING_TO_BOOL(x) !strcmp(x, "true") ? 1:0 

#define COMMAND_SET(cmd, desc, count, ...) SET_VALUE(command, args, cmd)
#define COMMAND_INITIALIZE(name, desc, args...) {#name, desc, #args, NULL},

#define ARGUMENT_SET(name, ...) SET_VALUE(argument, args, name)
#define ARGUMENT_INITIALIZE(name, desc) {#name, desc, NULL}, 

#define OPTION_SET(name, ...) SET_VALUE(option, args, name)
#define OPTION_INITIALIZE(name, req, value, desc) {#name, desc, req, 0, #value, #value},

#define SET_VALUE(src, dst, var) if (!strcmp(src->name, #var)) { dst.var = src->value; }
#define GENERATE_CHAR(name, ...) char *name;
#define GENERATE_CLI_ENUM(name, ...) name,

#define CLI_PARSER_STRUCT \
    typedef struct { \
        FOREACH(COMMANDS, GENERATE_CHAR) \
        FOREACH(ARGUMENTS, GENERATE_CHAR) \
        FOREACH(OPTIONS, GENERATE_CHAR) \
    } args_t; \

#define CLI_PARSER \
    size_t i, cli_result; \
    command_t  *command; \
    argument_t *argument; \
    option_t  *option; \
    args_t args = {}; \
    enum CLI_COMMAND_ENUM { \
        FOREACH(COMMANDS, GENERATE_CLI_ENUM) \
        LAST_CLI_COMMAND \
    }; \
    enum CLI_ARGUMENTS_ENUM { \
        FOREACH(ARGUMENTS, GENERATE_CLI_ENUM) \
        LAST_CLI_ARGUMENT \
    }; \
    enum CLI_OPTIONS_ENUM { \
        FOREACH(OPTIONS, GENERATE_CLI_ENUM) \
        LAST_CLI_OPTION \
    }; \
    command_t commands[] = { \
        FOREACH(COMMANDS, COMMAND_INITIALIZE) \
    }; \
    argument_t arguments[] = { \
        FOREACH(ARGUMENTS, ARGUMENT_INITIALIZE) \
    }; \
    option_t options[] = { \
        FOREACH(OPTIONS, OPTION_INITIALIZE) \
    }; \
    cli_t cli = {{LAST_CLI_COMMAND, 0}, {LAST_CLI_ARGUMENT, 0}, {LAST_CLI_OPTION, 0}, commands, arguments, options, cli_get_name(argv[0])}; \
    cli_result = cli_parser(&cli, argc, argv); \
    \
    for (i = 0; i < cli.n_options.len; i++) { \
        option = &cli.options[i]; \
        FOREACH(OPTIONS, OPTION_SET) \
    } \
    for (i = 0; i < cli.n_commands.len; i++) { \
        command = &cli.commands[i]; \
        FOREACH(COMMANDS, COMMAND_SET) \
    } \
    for (i = 0; i < cli.n_arguments.len; i++) { \
        argument = &cli.arguments[i]; \
        FOREACH(ARGUMENTS, ARGUMENT_SET) \
    }

typedef struct {
    const char *name;
    const char *description;
    const char *args_list;
    char       *value;
} command_t;

typedef struct {
    const char *name;
    const char *description;
    char       *value;
} argument_t;

typedef struct {
    const char *name;
    const char *description;
    int         value_required;
    int         seen;
    const char *default_value;
    char       *value;
} option_t;

typedef struct {
    size_t len;
    size_t found;
} counts_t;

typedef struct {
    counts_t      n_commands;
    counts_t      n_arguments;
    counts_t      n_options;
    command_t    *commands;
    argument_t   *arguments;
    option_t     *options;
    const char   *app_name;
} cli_t;

typedef struct {
    size_t  argc;
    char**  argv;
    char*   current;
    size_t  index;
} argv_t;

int cli_parser(cli_t *cli, int argc, char *argv[]);
char *cli_get_name(char *path);

#ifdef __cplusplus
}
#endif

#endif // _CLI_PARSER_H_
