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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cli_parser.h"


/* prototypes */
void  print_usage(cli_t *c);

/* print usage */
void print_usage(cli_t *c) {
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
        char *str = strdup(o.args_list);
        char *name;
        fprintf(stderr, "    opendcp_encode [options] %s ", o.name);
        while ((name = strsep(&str, ","))) {
            fprintf(stderr, " <%s>", name);
        }
        fprintf(stderr, "\n");
    }
} 

/* get the cli executable name */
char *cli_get_name(char *path) {
    char *base = strrchr(path, '/');

    return base ? base + 1 : path;
}

/* count required args from args list string */
int get_args_required(const char *str) {
   int i, count = 0;

   for (i = 0; str[i] != '\0'; i++) {
       if (str[i] == ',') {
           count++;
       }
   }

   /* number of commands is equal to number of , + 1 */
   return ++count;
}
     
/* create context */
argv_t argv_create(int argc, char **argv) {
    argv_t a = {argc, argv, argv[0], 0};

    return a;
}

/* increment argv index */
argv_t *argv_increment(argv_t *a) {
    if (a->index < a->argc) {
        a->current = a->argv[++a->index];
    }

    if (a->index == a->argc) {
        a->current = NULL;
    }

    return a;
}

/* get option from cli */
int get_option(argv_t *a, option_t *options, size_t options_len) {
    size_t i;
    option_t *option = NULL;
    char *name = &a->current[2];

    for (i = 0; i < options_len ; i++) {
        if (!strcmp(name, options[i].name)) {
            option = &options[i];
            break;
        }
    }

    if (option->seen++) {
        fprintf(stderr, "ERROR: %s was supplied more than once\n", a->current);
        return 1;
    }

    if (!option) {
        fprintf(stderr, "ERROR: %s is not a valid option\n", a->current);
        return 1;
    }

    argv_increment(a);

    if (option->value_required) {
        if (a->current == NULL) {
            fprintf(stderr, "ERROR: %s requires an argument\n", option->name);
            return 1;
        }
        option->value = a->current;
        argv_increment(a);
    }
    else {
        if (a->current != NULL) {
            fprintf(stderr, "ERROR: %s does not take an argument\n", option->name);
            return 1;
        }
        option->value = "1";
    }

    return 0;
}

/* get commnad from cli */
command_t get_command(const char *a, command_t *commands, size_t commands_len) {
    size_t i;
    command_t command = {0, 0, 0, 0};

    for (i = 0; i < commands_len; i++) {
        if (!strcmp(commands[i].name, a)) {
            commands[i].value = "1";
            command = commands[i]; 
        }
    }

    return command;
}

/* set argument element */
int set_argument(char *name, char *value, argument_t *arguments, size_t arguments_len) {
    size_t i;

    for (i = 0; i < arguments_len; i++) {
        if (!strcmp(arguments[i].name, name) && arguments[i].value == NULL) {
            arguments[i].value = value;
            return true;
        }
    }

    return false;
}

/* get argument from cli */
int get_argument(command_t *command, char *arg_value, argument_t *arguments, size_t arguments_len) {
    char *str = strdup(command->args_list);
    char *name;

    /* iterate command arg list */
    while ((name = strsep(&str, ","))) {
        if (set_argument(name, arg_value, arguments, arguments_len) == true) {
            return true;
        }
    }

    return false;
}

/* parse input arguments and build cli */
int parse_args(argv_t *a, cli_t *cli) {
    command_t command = {NULL, NULL, NULL, NULL};

    /* first argv is the command, skip that */
    argv_increment(a);

    while (a->current != NULL) {
        if (a->current[0] == '-' && a->current[1] == '-') {
            if (get_option(a, cli->options, cli->n_options.len)) {
                return 1;
            }
        }
        else {
            if (!command.value) {
                command = get_command(a->current, cli->commands, cli->n_commands.len);
                if (command.value) {
                    cli->n_commands.found++;
                }
            }
            else if (get_argument(&command, a->current, cli->arguments, cli->n_arguments.len) == true){
                cli->n_arguments.found++;
            }
            argv_increment(a);
        }
    }

    return 0;
}

int cli_parser(cli_t *cli, int argc, char *argv[]) {
    size_t i;
    argv_t a;

    for (i = 0; i < cli->n_options.len; i++) {
        if (!strcmp("NULL", cli->options[i].value)) {
            /* make string NULL values really NULL */
            cli->options[i].value = NULL;
        }
    }

    a = argv_create(argc, argv);
   
    if (parse_args(&a, cli)) {
        return 1;
    }

    for (i = 0; cli->options[i].name != NULL; i++) {
        if (!strcmp("help", cli->options[i].name)) {
            if (cli->options[i].value) {
                print_usage(cli);
                exit(1);
            }
        }
    }

    /* check if command specified */
    if (cli->n_commands.found != 1) {
        fprintf(stderr, "ERROR: No command supplied\n");
        exit(1);
    }

    /* get the command issued (currently sub-commands are not supported) */
    command_t command;
    for (i = 0; i < cli->n_commands.len; i++) {
        if (cli->commands[i].value) {
            command = cli->commands[i];
        }
    }
    size_t args_required = get_args_required(command.args_list);
    
    /* check if right amount of arguments was found */
    if (args_required != cli->n_arguments.found) {
        fprintf(stderr, "ERROR: Missing arguments %s requires %s\n", command.name, command.args_list);
        exit(1);
    }

    return 0;
}
