#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *line);
int lsh_execute(char **args);

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_builtins(void)
{
    return sizeof(builtin_str) / sizeof(builtin_str[0]);
}

int main(void)
{
    lsh_loop();
    return EXIT_SUCCESS;
}

void lsh_loop(void)
{
    char *line;
    char **args;
    int status = 1;

    do {
        printf("> ");
        fflush(stdout);

        line = lsh_read_line();

        if (line == NULL) {
            break;
        }

        args = lsh_split_line(line);

        if (args == NULL) {
            free(line);
            continue;
        }

        status = lsh_execute(args);

        free(line);
        free(args);

    } while (status);
}

char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc((size_t)bufsize);
    int c;

    if (buffer == NULL) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            if (c == EOF && position == 0) {
                free(buffer);
                return NULL;
            }

            buffer[position] = '\0';
            return buffer;
        }

        buffer[position++] = (char)c;

        if (position >= bufsize - 1) {
            bufsize += LSH_RL_BUFSIZE;

            char *temp = realloc(buffer, (size_t)bufsize);

            if (temp == NULL) {
                free(buffer);
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }

            buffer = temp;
        }
    }
}

char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;

    char **tokens = malloc((size_t)bufsize * sizeof(char *));

    if (tokens == NULL) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(line, LSH_TOK_DELIM);

    while (token != NULL) {
        tokens[position++] = token;

        if (position >= bufsize - 1) {
            bufsize += LSH_TOK_BUFSIZE;

            char **temp = realloc(
                tokens,
                (size_t)bufsize * sizeof(char *)
            );

            if (temp == NULL) {
                free(tokens);
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }

            tokens = temp;
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }

    tokens[position] = NULL;

    return tokens;
}

int lsh_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();

    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }

        exit(EXIT_FAILURE);

    } else if (pid < 0) {
        perror("lsh");

    } else {
        do {
            if (waitpid(pid, &status, WUNTRACED) == -1) {
                perror("lsh");
                return 1;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int lsh_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

int lsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else if (chdir(args[1]) != 0) {
        perror("lsh");
    }

    return 1;
}

int lsh_help(char **args)
{
    (void)args;

    printf("Simple C Shell\n");
    printf("Built-in commands:\n");

    for (int i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("External programs can also be executed.\n");

    return 1;
}

int lsh_exit(char **args)
{
    (void)args;
    return 0;
}

