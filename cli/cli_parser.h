/*
  The MIT License (MIT)

  Copyright (c) 2015 Terrence Meiczinger

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
    const char* name;
    const char* description;
    const char* args_list;
    char*       value;
} command_t;

typedef struct {
    const char* name;
    const char* description;
    char*       value;
} argument_t;

typedef struct {
    const char* name;
    const char* description;
    int         value_required;
    int         seen;
    const char* default_value;
    char*       value;
} option_t;

typedef struct {
    size_t len;
    size_t found;
} counts_t;

typedef struct {
    counts_t      n_commands;
    counts_t      n_arguments;
    counts_t      n_options;
    command_t*    commands;
    argument_t*   arguments;
    option_t*     options;
    const char*   app_name;
} cli_t;

typedef struct {
    size_t    argc;
    char**    argv;
    char*     current;
    size_t    index;
} argv_t;

static inline int   cli_parser(cli_t* cli, int argc, char* argv[]);
static inline char* cli_get_name(char* path);
static inline void  cli_print_usage(cli_t* c);

static inline void print_argify(const char *args) {
    char *name;
    char *str, *ptr;
    str = ptr = strdup(args);

    while ((name = strsep(&str, ","))) {
        fprintf(stderr, " <%s>", name);
    }
    fprintf(stderr, "\n");

    if (ptr != NULL) {
        free(ptr);
    }
}

/* print usage */
static inline void cli_print_usage(cli_t *c) {
    size_t i;

    fprintf(stderr, "Usage: %s [--version] [--help] [options] command <args>\n", c->app_name);
    fprintf(stderr, "\nOptions:\n");
    for (i = 0; i < c->n_options.len; i++) {
        option_t o = c->options[i];
        fprintf(stderr, "  --%-15.15s  %s [default %s]\n", o.name,  o.description, o.default_value);
    }
    fprintf(stderr, "\nCommands:\n");

    for (i = 0; i < c->n_commands.len; i++) {
        command_t o = c->commands[i];
        fprintf(stderr, "    %-15.15s  %s\n", o.name,  o.description);
    }
    fprintf(stderr, "\nCommand Usage:\n");
    for (i = 0; i < c->n_commands.len; i++) {
        command_t o = c->commands[i];
        fprintf(stderr, "    %s [options] %s ", c->app_name, o.name);
        print_argify(o.args_list);
    }
}

/* get the cli executable name */
static inline char* cli_get_name(char* path) {
    char* base = strrchr(path, '/');
    return base ? base + 1 : path;
}

/* count required args from args list string */
static inline int get_args_required(const char* str) {
    int i, count = 0;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == ',') {
            count++;
        }
    }

    /* number of commands is equal to number of , + 1 */
    return ++count;
}

/* increment argv index */
static inline argv_t* argv_next(argv_t* a) {
    if (a->index < a->argc) {
        a->current = a->argv[++a->index];
    }

    if (a->index == a->argc) {
        a->current = NULL;
    }

    return a;
}

static inline int get_long_option(argv_t* a, option_t* options, size_t len) {
    size_t    i, j;
    char*     name = NULL;
    char*     value = NULL;
    char*     delimeter = NULL;
    option_t* option = NULL;

    name = a->current + 2;
    delimeter = strchr(name, '=');

    if (delimeter) {
        value = delimeter + 1;
    } else {
        argv_next(a);
        value = a->current;
    }

    for (i = 0; i < len ; i++) {
        if (strlen(options[i].name) > (unsigned long )(delimeter - name)) {
            j = strlen(options[i].name);
        } else {
            j = delimeter - name;
        }
        if (!strncmp(options[i].name, name, j)) {
            option = &options[i];
            break;
        }
    }

    if (option == NULL) {
        fprintf(stderr, "ERROR: %s is not a valid option\n", name);
        return 1;
    }

    if (option->seen++) {
        fprintf(stderr, "ERROR: %s was supplied more than once\n", name);
        return 1;
    }

    if (option->value_required) {
        if (value == NULL) {
            fprintf(stderr, "ERROR: %s requires an argument\n", option->name);
            return 1;
        }
        option->value = value;
    }
    else {
        if (delimeter != NULL) {
            fprintf(stderr, "ERROR: %s does not take an argument\n", option->name);
            return 1;
        }
        option->value = "1";
    }

    return 0;
}

/* get commnad from cli */
static inline command_t get_command(argv_t* a, cli_t* cli) {
    size_t i;
    command_t command = {0, 0, 0, 0};

    for (i = 0; i < cli->n_commands.len; i++) {
        if (!strcmp(cli->commands[i].name, a->current)) {
            cli->commands[i].value = "1";
            cli->n_commands.found++;
            command = cli->commands[i];
            break;
        }
    }

    return command;
}

/* set argument element */
static inline int set_argument(char* name, char* value, argument_t* arguments, size_t len) {
    size_t i;

    for (i = 0; i < len; i++) {
        if (!strcmp(arguments[i].name, name)) {
            arguments[i].value = value;
            return 0;
        }
    }

    fprintf(stderr, "ERROR: There is a mismatch between commands args_list and define arguments\n");

    return 1;
}

/* get argument from cli */
static inline int get_arguments(const char* args_list, argv_t* a, cli_t* cli) {
    char* name;
    char* ptr, *str;

    str = ptr = strdup(args_list);

    /* iterate command arg list */
    while ((name = strsep(&str, ","))) {
        if (set_argument(name, a->current, cli->arguments, cli->n_arguments.len)) {
            return 1;
        }
        cli->n_arguments.found++;
        argv_next(a);
    }

    size_t args_required = get_args_required(args_list);
    if (args_required != cli->n_arguments.found) {
        fprintf(stderr, "ERROR: Incorrect numbers of arguments. Expected");
        print_argify(args_list);
        return 1;
    }

    if (ptr != NULL) {
        free(ptr);
    }

    return 0;
}

/* parse input arguments and build cli */
static inline int parse_args(argv_t* a, cli_t* cli) {

    command_t command = {NULL, NULL, NULL, NULL};
    argv_next(a);

    while (a->current != NULL) {
        if (strstr(a->current, "--")) {
            if (get_long_option(a, cli->options, cli->n_options.len)) {
                return 1;
            }
            argv_next(a);
        }
        else {
            if (!command.name) {
                command = get_command(a, cli);
                argv_next(a);
            }
            else if (get_arguments(command.args_list, a, cli) ){
                return 1;
            }
        }
    }

    return 0;
}

static inline int cli_parser(cli_t* cli, int argc, char* argv[]) {
    size_t i;
    argv_t a  = {argc, argv, argv[0], 0};

    for (i = 0; i < cli->n_options.len; i++) {
        if (!strcmp("NULL", cli->options[i].value)) {
            /* make string NULL values really NULL */
            cli->options[i].value = NULL;
        }
    }

    if (parse_args(&a, cli)) {
        return 1;
    }

    for (i = 0; cli->options[i].name != NULL; i++) {
        if (!strcmp("help", cli->options[i].name)) {
            if (cli->options[i].value) {
                cli_print_usage(cli);
                return 1;
            }
        }
        if (!strcmp("version", cli->options[i].name)) {
            if (cli->options[i].value) {
                return 0;
            }
        }
    }

    /* check if command specified */
    if (cli->n_commands.found != 1) {
        fprintf(stderr, "ERROR: No command supplied\n");
        return 1;
    }

    /* get the command issued (currently sub-commands are not supported) */
    command_t command;
    for (i = 0; i < cli->n_commands.len; i++) {
        if (cli->commands[i].value) {
            command = cli->commands[i];
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif // _CLI_PARSER_H_
