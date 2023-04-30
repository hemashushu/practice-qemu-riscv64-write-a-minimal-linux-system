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
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

// simple pipe and redirect are supported
//
// pipe:
//     a | b
//     a | b | c
//
// redirect:
//     a > output
//     a < input
//     a > output < input
//
// "< input" must be followed by "> output"
// "a" can be a pipe, e.g.
//
//     a | b | c > output
//     a | b | c < input
//     a | b | c > output < input
//
// e.g.
// cat hello.txt > world.txt
// cat hello.txt | tr [:lower:] [:upper:] | tr [:blank:] _ > HELLO.txt

struct Program
{
    int argc;    // number of arguments
    char **argv; // argv[0] is the program file path
};

struct Task
{
    char *output_filepath;     // NULL for stdout
    char *input_filepath;      // NULL for stdin
    int number_of_programs;    // number of programs
    struct Program **programs; // array of programs
    bool is_background;        // indicates do not wait for the last program to finish
};

extern char **environ;

const int MAX_PATH_LENGTH = 1024;
const int MAX_PROGRAMS = 256;
const int MAX_ARGS = 256;

// functions prototypes

void loop(void);
int run_script(char *);
char *get_current_working_directory(void);
char *get_command_line(void);
struct Task *get_task(char *);
void free_task(struct Task *);
bool check_background_operator(char *);
char *get_input_filepath(char *);
char *get_output_filepath(char *);
struct Program *get_program(char *);
void free_program(struct Program *);
void command_cd(char *);
void command_export(char **);
void command_help(void);
void execute_task(struct Task *);
int execute_program(struct Program *);
int execute_external(char **);
size_t trim(char *, size_t, const char *);
char *trim_inplace(char *);
void test_trim(void);
void test_trim_inplace(void);

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
    struct Task *task;

    while (true)
    {
        cwd = get_current_working_directory();
        snprintf(prompt, sizeof(prompt), "%s > ", cwd);
        fputs(prompt, stdout);

        line = get_command_line();
        task = get_task(line);

        if (task != NULL)
        {
            execute_task(task);
            free_task(task);
        }

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
        struct Task *task = get_task(line);
        if (task != NULL)
        {
            execute_task(task);
            free_task(task);
        }
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

// return NULL when the line is comment.
struct Task *get_task(char *line)
{
    line = trim_inplace(line);

    // line comment
    if (line[0] == '#')
    {
        return NULL;
    }

    bool is_background = check_background_operator(line);
    char *input_filepath = get_input_filepath(line);
    char *output_filepath = get_output_filepath(line);
    char *task_line = trim_inplace(line);

    if (input_filepath != NULL && output_filepath == NULL)
    {
        // the "> ..." may be followed by "< ..."
        output_filepath = get_output_filepath(input_filepath);
        input_filepath = trim_inplace(input_filepath);
    }

    const char *DELIMITERS = "|";
    char **program_lines = malloc(MAX_PROGRAMS * sizeof(char *));

    int count = 0;
    char *token = strtok(task_line, DELIMITERS);

    while (token != NULL)
    {
        program_lines[count] = token;
        count++;
        token = strtok(NULL, DELIMITERS);
    }

    struct Program **programs = malloc(count * sizeof(struct Program *));
    for (int idx = 0; idx < count; idx++)
    {
        programs[idx] = get_program(program_lines[idx]);
    }

    free(program_lines);

    struct Task *task = malloc(sizeof(*task));
    task->input_filepath = input_filepath;
    task->output_filepath = output_filepath;
    task->number_of_programs = count;
    task->programs = programs;
    task->is_background = is_background;

    return task;
}

void free_task(struct Task *task)
{
    for (int i = 0; i < task->number_of_programs; i++)
    {
        free_program(task->programs[i]);
    }

    free(task->programs);
    free(task);
}

bool check_background_operator(char *line)
{
    int len = strlen(line);
    bool result = false;

    for (int i = len - 1; i > 0; i--)
    {
        if (isspace(line[i]))
        {
            continue;
        }
        else
        {
            if (line[i] == '&')
            {
                result = true;
                line[i] = '\0';
            }
            break;
        }
    }

    return result;
}

char *get_input_filepath(char *line)
{
    char *ptr = strrchr(line, '<');
    if (ptr != NULL)
    {
        *ptr = '\0'; // set NULL terminate
        char *text = ptr + 1;
        return trim_inplace(text);
    }
    else
    {
        return NULL;
    }
}

char *get_output_filepath(char *line)
{
    char *ptr = strrchr(line, '>');
    if (ptr != NULL)
    {
        *ptr = '\0'; // set NULL terminate
        char *text = ptr + 1;
        return trim_inplace(text);
    }
    else
    {
        return NULL;
    }
}

struct Program *get_program(char *line)
{
    const char *DELIMITERS = " \t\n";
    char **argv = malloc(MAX_ARGS * sizeof(char *));

    int count = 0;
    char *token = strtok(line, DELIMITERS);

    while (token != NULL)
    {
        argv[count] = token;
        count++;
        token = strtok(NULL, DELIMITERS);
    }

    // set NULL terminate
    argv[count] = NULL;

    struct Program *program = malloc(sizeof(*program));
    program->argc = count;
    program->argv = argv;

    return program;
}

void free_program(struct Program *program)
{
    free(program->argv);
    free(program);
}

void execute_task(struct Task *task)
{
    // parent process (the current process)
    //  |
    //  |           first input stream
    //  |            |
    //  |-- fork/exec 1st program
    //  |            | pipe 0
    //  |-- fork/exec 2st program
    //  |            | pipe 1
    //  |-- fork/exec 3st program
    //               |
    //              last output stream
    //
    // it's slightly different from the Bourne again shell (Bash), which
    // fork and create the first child process, then fork and exec
    // all programs (except the last one) in the first child process,
    // at last the first child process exec the last program.
    // the first child process will create a new process group, thus
    // all child processes run in one process group.
    //
    // parent process (the current process)
    //  |
    //  |-- fork, child process
    //        |
    //        |           first input stream
    //        |            |
    //        |-- fork/exec 1st program
    //        |            | pipe 0
    //        |-- fork/exec 2st program
    //        |            | pipe 1
    //        |-- exec 3st program
    //                     |
    //                    last output stream
    //
    // check APUE chapter 9.9

    pid_t final_pid = 0;

    // save the stdin and stdout for restore them when task complete
    int saved_in = dup(0);
    int saved_out = dup(1);

    int fd_in;
    int fd_out;

    // get the first input stream
    if (task->input_filepath != NULL)
    {
        fd_in = open(task->input_filepath, O_RDONLY);
        if (fd_in == -1)
        {
            perror("open");
            return;
        }
    }
    else
    {
        fd_in = dup(saved_in);
    }

    for (int idx = 0; idx < task->number_of_programs; idx++)
    {
        struct Program *program = task->programs[idx];

        // redirect input stream
        dup2(fd_in, 0);
        close(fd_in);

        // set up redirect output stream
        if (idx == task->number_of_programs - 1)
        {
            // it is the last program
            // get the last output stream
            if (task->output_filepath != NULL)
            {
                //` 0666` is an oct number, the actual permission will be `0666 & umask`
                fd_out = open(task->output_filepath, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                if (fd_out == -1)
                {
                    perror("open");
                    return;
                }
            }
            else
            {
                fd_out = dup(saved_out);
            }
        }
        else
        {
            // redirect output stream to pipe
            // craete pipe
            int fd_pipe[2];
            if (pipe(fd_pipe) != 0)
            {
                perror("pipe");
                return;
            }

            int fd_writing_port = fd_pipe[1];
            int fd_reading_port = fd_pipe[0];

            fd_out = fd_writing_port;

            // set the next program input stream
            fd_in = fd_reading_port;
        }

        // redirect output stream
        dup2(fd_out, 1);
        close(fd_out);

        pid_t pid = execute_program(program);

        if (!task->is_background && pid != 0)
        {
            // only keep the PID which is not builtin command
            final_pid = pid;
        }
    }

    // restore the saved stdin and stdout
    dup2(saved_in, 0);
    dup2(saved_out, 1);
    close(saved_in);
    close(saved_out);

    if (final_pid != 0)
    {
        // wait until the last program process exit or is killed
        int status;
        do
        {
            waitpid(final_pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

// return 0 if program is the builtin function
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

        // builtin commands:
        // cd, export, exit, help
        //
        // unimplement:
        // source(.), set
        //
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
            if (getppid() == 1)
            {
                // when the parent PID == 1, it means
                // the current process is the only shell.
                fputs("Can not exit to init.\n", stderr);
                fputs("You may shutdown system through execute command `poweroff`.\n", stderr);
                return 0;
            }
            else
            {
                // exit the current shell
                exit(EXIT_SUCCESS);
            }
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
        exit(EXIT_FAILURE);
    }
}

size_t trim(char *buf, size_t buf_len, const char *str)
{
    if (buf_len == 0)
    {
        return 0;
    }

    // Trim leading space
    const char *start_ptr = str;
    while (isspace((unsigned char)*start_ptr))
    {
        start_ptr++;
    }

    if (*start_ptr == 0)
    {
        // all characters are space
        *buf = 0;
        return 0;
    }

    // Trim trailing space
    const char *end_ptr = start_ptr + strlen(start_ptr) - 1;
    while (end_ptr > start_ptr && isspace((unsigned char)*end_ptr))
    {
        end_ptr--;
    }

    size_t actual_len = end_ptr - start_ptr + 1;

    // truncate the string if the buffer size is smaller than the actual size
    size_t copy_size = (actual_len < buf_len - 1) ? actual_len : (buf_len - 1);

    memcpy(buf, start_ptr, copy_size);

    // set null terminator
    buf[copy_size] = 0;

    return copy_size;
}

char *trim_inplace(char *str)
{
    char *start_ptr = str;
    while (isspace((unsigned char)*start_ptr))
    {
        start_ptr++;
    }

    if (*start_ptr == 0)
    {
        // all characters are space
        return start_ptr;
    }

    char *end_ptr = start_ptr + strlen(start_ptr) - 1;
    while (end_ptr > start_ptr && isspace((unsigned char)*end_ptr))
    {
        end_ptr--;
    }

    // set null terminator
    end_ptr[1] = '\0';
    return start_ptr;
}

void test_trim(void)
{
    const size_t buf_len = 10;
    char buf[buf_len];

    assert(trim(buf, buf_len, "hello") == 5);
    assert(strcmp(buf, "hello") == 0);

    assert(trim(buf, buf_len, "foo  ") == 3);
    assert(strcmp(buf, "foo") == 0);

    assert(trim(buf, buf_len, "  world") == 5);
    assert(strcmp(buf, "world") == 0);

    assert(trim(buf, buf_len, " bar ") == 3);
    assert(strcmp(buf, "bar") == 0);

    assert(trim(buf, buf_len, "    ") == 0);
    assert(strcmp(buf, "") == 0);

    // buffer length is 10, which can only contains 9 characters.
    assert(trim(buf, buf_len, "   hello world   ") == 9);
    assert(strcmp(buf, "hello wor") == 0);
}

char *to_mut_string(const char *str)
{
    int len = strlen(str);
    char *mut_str = malloc(len + 1);
    memcpy(mut_str, str, len);
    return mut_str;
}

void test_trim_inplace(void)
{
    char *mut_str;

    mut_str = to_mut_string("hello");
    assert(strcmp(trim_inplace(mut_str), "hello") == 0);
    free(mut_str);

    mut_str = to_mut_string("foo  ");
    assert(strcmp(trim_inplace(mut_str), "foo") == 0);
    free(mut_str);

    mut_str = to_mut_string("  world");
    assert(strcmp(trim_inplace(mut_str), "world") == 0);
    free(mut_str);

    mut_str = to_mut_string(" bar ");
    assert(strcmp(trim_inplace(mut_str), "bar") == 0);
    free(mut_str);

    mut_str = to_mut_string("    ");
    assert(strcmp(trim_inplace(mut_str), "") == 0);
    free(mut_str);
}