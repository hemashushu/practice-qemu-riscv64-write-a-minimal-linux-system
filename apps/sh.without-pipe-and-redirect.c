/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

struct Program
{
    int argc;    // number of arguments
    char **argv; // argv[0] is the program file path
};

extern char **environ;

const int MAX_PATH_LENGTH = 1024;
const int MAX_ARGS = 256;

void loop(void);
int run_script(char *);
char *get_current_working_directory(void);
char *get_command_line(void);
struct Program *get_program(char *);
void free_program(struct Program *);
void command_cd(char *);
void command_export(char **);
void command_help(void);
int execute_program(struct Program *);
int execute_external(char **);

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        return run_script(argv[1]);
    }
    else if (argc == 1)
    {
        loop();
        return EXIT_SUCCESS;
    }
    else
    {
        fputs("Usage:\n", stderr);
        fputs("    sh\n", stderr);
        fputs("    sh /path/to/script\n", stderr);
        return EXIT_FAILURE;
    }
}

void loop(void)
{
    const int MAX_PROMPT_LENGTH = 1024;
    char prompt[MAX_PROMPT_LENGTH];

    char *cwd;
    char *line;
    struct Program *program;

    while (true)
    {
        cwd = get_current_working_directory();
        snprintf(prompt, sizeof(prompt), "%s > ", cwd);
        fputs(prompt, stdout);

        line = get_command_line();
        program = get_program(line);
        pid_t pid = execute_program(program);

        if (pid == 0)
        {
            // builtin commands
        }
        else
        {
            // wait until program process exit or is killed
            int status;
            do
            {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        free_program(program);
        free(cwd);
        free(line);
    }
}

int run_script(char *filepath)
{
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        perror("fopen");
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1)
    {
        // skip line comment
        if (line[0] == '#')
        {
            continue;
        }

        struct Program *program = get_program(line);
        pid_t pid = execute_program(program);

        if (pid == 0)
        {
            // builtin commands
        }
        else
        {
            // wait until program process exit or is killed
            int status;
            do
            {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        free_program(program);
    }

    free(line);
    fclose(file);

    return EXIT_SUCCESS;
}

char *get_current_working_directory(void)
{
    char *path = malloc(MAX_PATH_LENGTH * sizeof(char));
    if (getcwd(path, MAX_PATH_LENGTH) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    return path;
}

char *get_command_line(void)
{
    char *line = NULL;
    size_t len = 0;

    if (getline(&line, &len, stdin) == -1)
    {
        perror("getline");
        exit(EXIT_FAILURE);
    }

    return line;
}

struct Program *get_program(char *line)
{
    const char *DELIMITERS = " \t\n";

    struct Program *program = malloc(sizeof(*program));
    char **argv = malloc(MAX_ARGS * sizeof(char *));

    int i = 0;
    char *token = strtok(line, DELIMITERS);

    while (token != NULL)
    {
        argv[i] = token;
        i++;
        token = strtok(NULL, DELIMITERS);
    }

    // set NULL terminate
    argv[i] = NULL;

    program->argc = i;
    program->argv = argv;

    return program;
}

void free_program(struct Program *program)
{
    free(program->argv);
    free(program);
}

pid_t execute_program(struct Program *program)
{
    if (program->argc == 0)
    {
        // empty command, the length of argv is 0
        return 0;
    }
    else
    {
        char *cmd = program->argv[0];

        // builtin commands
        // https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html

        if (strcmp(cmd, "cd") == 0)
        {
            // change the current working directory
            command_cd(program->argv[1]);
            return 0;
        }
        else if (strcmp(cmd, "export") == 0)
        {
            // set environment variable
            command_export(program->argv);
            return 0;
        }
        else if (strcmp(cmd, "help") == 0)
        {
            // print help information
            command_help();
            return 0;
        }
        else if (strcmp(cmd, "exit") == 0)
        {
            // exit the current shell
            exit(EXIT_SUCCESS);
        }
        else
        {
            // execute external program
            return execute_external(program->argv);
        }
    }
}

void command_cd(char *dest)
{
    if (dest == NULL)
    {
        // change to $HOME directory
        dest = getenv("HOME");
    }

    if (dest != NULL)
    {
        // change to `dest` directory
        if (chdir(dest) != 0)
        {
            perror("chdir");
        }
        else
        {
            // update $PWD
            char cwd[MAX_PATH_LENGTH];
            getcwd(cwd, MAX_PATH_LENGTH);
            setenv("PWD", cwd, 1);
        }
    }
}

void command_help(void)
{
    puts("Shell 1.0");
}

void command_export(char **argv)
{
    if (argv[1] == NULL)
    {
        // list all environment varables
        for (char **idx = environ; *idx != NULL; idx++)
        {
            puts(*idx);
        }
    }
    else if (argv[2] != NULL)
    {
        fputs("Usage:\n", stderr);
        fputs("    export\n", stderr);
        fputs("    export NAME=VALUE\n", stderr);
    }
    else
    {
        char *kvp = argv[1];
        char *copied = malloc(strlen(kvp) + 1);
        strcpy(copied, kvp);
        if (putenv(copied) != 0)
        {
            perror("putenv");
        }
    }
}

pid_t execute_external(char **argv)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // child process
        // execve nerver return unless error occured.
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        // parent process
        return pid;
    }
    else
    {
        // failed to fork
        perror("fork");
        return 0;
    }
}