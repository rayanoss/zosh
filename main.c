#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define ZOSH_RL_BUFSIZE 1024

int zosh_cd(char **args);
int zosh_exit(char **args);
char *zosh_read_line(void);
char **zosh_split_line(char *line);
int zosh_execute(char **args);
int zosh_launch(char **args, char *output_file, char *input_file, int background, int redirect_output, int redirect_append);
void zosh_loop(void);

char *builtin_str[] = {
    "cd",
    "exit"};

int (*builtin_func[])(char **) = {
    &zosh_cd,
    &zosh_exit};

int zosh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int zosh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "zosh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("zosh");
        }
    }
    return 1;
}

int zosh_exit(char **args)
{
    (void)args;
    return 0;
}

int zosh_redirect_ouput(char **args, char **output_file)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "syntax error: missing filename after '>'\n");
                return -1;
            }
            *output_file = args[i + 1];
            args[i] = NULL;
            return 1;
        }
    }

    return 0;
}

int zosh_redirect_append(char **args, char **output_file)
{

    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], ">>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "syntax error: missing filename after '>>'\n");
                return -1;
            }
            *output_file = args[i + 1];
            args[i] = NULL;
            return 1;
        }
    }

    return 0;
}

int zosh_background(char **args)
{
    int i = 0;
    while (args[i] != NULL)
    {
        i++;
    }

    if (i > 0 && strcmp(args[i - 1], "&") == 0)
    {
        args[i - 1] = NULL;
        return 1;
    }

    return 0;
}
int zosh_redirect_input(char **args, char **input_file)
{

    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "syntax error: missing filename after '<'\n");
                return -1;
            }
            *input_file = args[i + 1];
            args[i] = NULL;
            return 1;
        }
    }

    return 0;
}

#define ZOSH_PIPE_BUFSIZE 64

struct pipe_info
{
    int *position;
    int count;
};

struct pipe_info zosh_pipe(char **args)
{
    int bufsize = ZOSH_PIPE_BUFSIZE;
    struct pipe_info info;
    info.position = malloc(bufsize * sizeof(int));
    info.count = 0;

    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            info.position[info.count++] = i;
            args[i] = NULL;
        }
    }

    return info;
}

void zosh_loop(void)
{
    char *line;
    char **args;
    int status;
    char cwd[1024];

    do
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("zosh:%s> ", cwd);
        }
        else
        {
            printf("zosh> ");
        }
        fflush(stdout);

        line = zosh_read_line();
        args = zosh_split_line(line);
        status = zosh_execute(args);
        free(line);
        free(args);
    } while (status);
}

char *zosh_read_line(void)
{
    int bufsize = ZOSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);

    int c;

    if (!buffer)
    {
        fprintf(stderr, "zosh: allocation error \n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = c;
        }

        position++;

        if (position >= bufsize)
        {
            bufsize += ZOSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                fprintf(stderr, "zosh: allocation error \n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define ZOSH_TOK_BUFSIZE 64
#define ZOSH_TOK_DELIM " \t\r\n\a"

char **zosh_split_line(char *line)
{
    int bufsize = ZOSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "zosh: allocation error \n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, ZOSH_TOK_DELIM);

    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += ZOSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "zosh: allocation error \n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, ZOSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int zosh_launch(char **args, char *output_file, char *input_file, int background, int redirect_output, int redirect_append)
{
    pid_t pid;
    pid = fork();

    if (pid == 0)
    {
        if (background)
            setpgid(0, 0);
        if (redirect_append)
        {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, 1);
            close(fd);
        }

        if (redirect_output)
        {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, 1);
            close(fd);
        }

        if (input_file != NULL)
        {
            int fd = open(input_file, O_RDONLY);
            if (fd == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, 0);
            close(fd);
        }

        if (execvp(args[0], args) == -1)
        {
            perror("Something went wrong");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0)
    {
        perror("zosh");
    }
    else
    {
        if (background)
        {
            printf("[1] %d\n", pid);
        }
        else
        {
            wait(NULL);
        }
    }

    return 1;
}

int zosh_execute_pipes(char **args, struct pipe_info *pipe_info, int background)
{
    char **cmds[pipe_info->count + 1];
    int pipes[pipe_info->count][2];
    pid_t last_pid = 0;

    cmds[0] = args;
    for (int i = 0; i < pipe_info->count; i++)
    {
        cmds[i + 1] = args + pipe_info->position[i] + 1;
    }

    for (int i = 0; i < pipe_info->count; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe");
            return 1;
        }
    }

    int num_cmds = pipe_info->count + 1;

    for (int i = 0; i < num_cmds; i++)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("fork");
            return 1;
        }
        else if (pid == 0)
        {
            if (background)
            {
                setpgid(0, 0);
            }

            if (i > 0)
            {
                dup2(pipes[i - 1][0], 0);
            }

            if (i < pipe_info->count)
            {
                dup2(pipes[i][1], 1);
            }

            for (int j = 0; j < pipe_info->count; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(cmds[i][0], cmds[i]);
            exit(1);
        }
        else
        {
            if (i == num_cmds - 1)
            {
                last_pid = pid;
            }
        }
    }

    for (int i = 0; i < pipe_info->count; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (background)
    {
        printf("[1] %d\n", last_pid);
    }
    else
    {
        for (int i = 0; i < num_cmds; i++)
        {
            wait(NULL);
        }
    }

    free(pipe_info->position);

    return 1;
}

int zosh_execute(char **args)
{
    char *output_file = NULL;
    char *input_file = NULL;

    if (args[0] == NULL)
    {
        return 1;
    }

    for (int i = 0; i < zosh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }

    int redirect_output = zosh_redirect_ouput(args, &output_file);
    int redirect_append = zosh_redirect_append(args, &output_file);
    int redirect_input = zosh_redirect_input(args, &input_file);

    if (redirect_output == -1 || redirect_append == -1 || redirect_input == -1)
    {
        return 1;
    }

    int background = zosh_background(args);
    struct pipe_info pipes = zosh_pipe(args);

    if (pipes.count > 0)
    {
        return zosh_execute_pipes(args, &pipes, background);
    }
    else
    {
        return zosh_launch(args, output_file, input_file, background, redirect_output, redirect_append);
    }
}

void handle_sigint(int sig)
{
    (void)sig;
    printf("\n");
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("zosh:%s> ", cwd);
    }
    else
    {
        printf("zosh> ");
    }
    fflush(stdout);
}

int main(void)
{
    signal(SIGINT, handle_sigint);

    zosh_loop();

    return EXIT_SUCCESS;
}
