#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

bool background;
int fmode = 0;
struct Proccs
{

    int arrayID;
    int status;
};

/* getting input from user, by getline function, assuming the length of the
command line is not exceding 2048 chars*/
char *getInput(char *commandLine, size_t line_length)
{
    char *modifiedCommandLine;

    printf("\n:");
    fflush(stdout);
    if (commandLine != NULL)
    {
        free(commandLine);
    }
    commandLine = (char *)malloc(line_length * sizeof(char));
    getline(&commandLine, &line_length, stdin);

    modifiedCommandLine = malloc(strlen(commandLine) - 1);
    strncpy(modifiedCommandLine, commandLine, strlen(commandLine) - 1);
    modifiedCommandLine[strlen(commandLine) - 1] = '\0';

    return modifiedCommandLine;
}
/*tokenizing command line char array using strtok_r and making sure it's gettingarguments with space 
    between them so I could further use it in execvp() function*/
char **tokenizeCommandLine(char *commandLine, char **arguments, int *count_args)
{
    int i;
    char *saveptr;
    char *token;
    token = strtok_r(commandLine, " ", &saveptr);
    arguments = calloc(512, sizeof(char *));
    i = 0;
    while (token != NULL)
    {

        arguments[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(arguments[i], token);

        i++;
        (*count_args)++;
        token = strtok_r(NULL, " ", &saveptr);
    }
    return arguments;
}

/*This is the same tokenizing command excexpt I would use it for the different function
in my main to make sure there is expansion var
line char array using strtok_r and making sure it's gettingarguments with space 
between them so I could further use it in execvp() function*/
char **tokenizeExistingArr(char *commandLine, char **arguments2, int *count_args)
{
    int i;
    char *saveptr;
    char *token;
    token = strtok_r(commandLine, " ", &saveptr);

    arguments2 = calloc(512, sizeof(char *));
    i = 0;
    while (token != NULL)
    {

        if (strcmp(token, ">") != 0 && strcmp(token, "<") != 0)
        {
            arguments2[i] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(arguments2[i], token);
            i++;
            (*count_args)++;
        }
        else
        {

            return arguments2;
        }

        token = strtok_r(NULL, " ", &saveptr);
    }
    return arguments2;
}
/*In this tokenizing function I would make sure that my arguments that I will use doesn't
have ambersand symbol &
line char array using strtok_r and making sure it's gettingarguments with space 
between them so I could further use it in execvp() function*/
char **tokenizeExistingArr2(char *commandLine, char **arguments3, int *count_args)
{
    int i;
    char *saveptr;
    char *token;
    token = strtok_r(commandLine, " ", &saveptr);

    arguments3 = calloc(512, sizeof(char *));
    i = 0;
    while (token != NULL)
    {

        if (strcmp(token, "&") != 0)
        {
            arguments3[i] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(arguments3[i], token);
            i++;
            (*count_args)++;
        }
        else
        {

            return arguments3;
        }

        token = strtok_r(NULL, " ", &saveptr);
    }
    return arguments3;
}
/* This is basically redirection function which redirects command with > output and < input symbols*/
void Redirection(int *status, char **arguments, int num_args)
{

    int insign = 0;
    int outsign = 0;
    int i = 0;

    for (i = 0; i < num_args; i++)
    {
        if (strcmp(arguments[i], ">") == 0)
        {
            outsign++;
        }
        else if (strcmp(arguments[i], "<") == 0)
        {
            insign++;
        }
    }

    if (strcmp(arguments[insign], "<") == 0)
    {

        int sourceFD = open(arguments[2], O_RDONLY);
        if (sourceFD == -1)
        {
            printf("cannot open %s for input\n", arguments[2]);
            fflush(stdout);
            *status = 1;
        }
        else
        {

            *status = 0;
            int result = dup2(sourceFD, 0);
            if (result == -1)
            {
                perror("source dup2()");
                fflush(stdout);
                *status = 1;
            }
            fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
        }
    }

    if (strcmp(arguments[outsign], ">") == 0)
    {

        int targetFD = open(arguments[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (targetFD == -1)
        {
            printf("cannot open %s for output\n", arguments[2]);
            fflush(stdout);
            *status = 1;
        }
        else
        {

            int result = dup2(targetFD, 1);
            if (result == -1)
            {
                perror("target dup2()");
                fflush(stdout);
                *status = 1;
            }
            fcntl(targetFD, F_SETFD, FD_CLOEXEC);
        }
    }
}
/* My main function for checking user's input and assigning correct fucntion to it
as well as switching between background and normal modes for the command*/
void shellCommands(char **arguments, int num_args, bool *progRun, int *Status, struct Proccs *arr, int *x, struct sigaction ignore_action, struct sigaction SIGTSTP_action, struct sigaction ignore_SIGTSTP)
{

    if (strcmp(arguments[0], "exit") == 0)
    {
        kill(getpid(), 1);
        (*progRun) = false;
    }

    if (strcmp(arguments[0], "status") == 0)
    {
        if (WIFEXITED(*Status))
        {
            printf("exit value %d\n", WEXITSTATUS(*Status));
            fflush(stdout);
        }
        else
        {
            printf("terminated by signal %d\n", WTERMSIG(*Status));
            fflush(stdout);
        }
    }

    if (strcmp(arguments[0], "cd") == 0)
    {

        if (num_args > 1)
        {
            char *path = (char *)malloc((int)strlen(arguments[1]) * sizeof(char));

            if (arguments[1][0] == '.' && arguments[1][1] == '/')
            {
                strcpy(path, arguments[1]);
                chdir(path);
            }

            else
            {
                strcpy(path, "./");
                strcat(path, arguments[1]);

                chdir(path);
            }
        }
        else if (num_args == 1)
        {

            chdir(getenv("HOME"));
        }
    }
    int i;

    if (arguments[0][0] == '#')
    {
        for (i = 0; i < num_args; i++)
        {
            printf("%s ", arguments[i]);
            fflush(stdout);
        }
    }

    else
    {
        if (background == false)
        {
            ignore_action.sa_handler = SIG_DFL;
            sigaction(SIGINT, &ignore_action, NULL);
        }
        sigaction(SIGTSTP, &ignore_SIGTSTP, NULL);
        /* code part below is from CANVAS MODULES WEEK 4*/
        int childStatus;
        pid_t childPid = fork();

        if (childPid == -1)
        {
            perror("fork() failed!");
            fflush(stdout);
            exit(1);
        }
        else if (childPid == 0)
        {

            (*Status) = execvp(arguments[0], arguments);
            perror("execlp");
            fflush(stdout);
            exit(2);
        }
        else
        {
            sigaction(SIGTSTP, &SIGTSTP_action, NULL);
            if (background == false)
            {
                childPid = waitpid(childPid, &childStatus, 0);
                *Status = childStatus;
                sigaction(SIGINT, &ignore_action, NULL);
            }

            if (background == true)
            {

                *Status = childStatus;
                printf("background pid is %d\n", childPid);

                arr[*x].arrayID = childPid;
                arr[*x].status = (*Status);
                (*x)++;

                childPid = waitpid(childPid, &childStatus, WNOHANG);
            }
        }
    }

    /*foreground*/
}

/* Expansing var checker function it takes input argument line and atokenized arguments and 
finds expansion vars $$ by using tokens and assingn it to the new created 2 d char array and returning it to the main*/
char **expansionVar(char *input, int pid, int num_args, char **arguments)
{

    char *saveptr;
    int Pidlength = snprintf(NULL, 0, "%d", pid);
    char *convertedPid = malloc((Pidlength + 1) * sizeof(char));
    snprintf(convertedPid, Pidlength + 1, "%d", pid);
    char *expandedInput = malloc((int)strlen(input) * sizeof(char));
    char **expandedInput2 = calloc(512, sizeof(char *));

    char *tmp = malloc((int)strlen(input) * sizeof(char));
    strcpy(tmp, input);
    int i = 0;

    int j = 0;
    char *token;
    char *token2;
    char *saveptr2;

    if (num_args == 1)
    {
        if (strcmp(arguments[0], "$$") != 0)
        {
            strtok_r(tmp, "$$", &saveptr);
            strcpy(expandedInput, tmp);

            strcat(expandedInput, convertedPid);

            expandedInput2[0] = expandedInput;
        }
        else
        {
            strcat(expandedInput, convertedPid);
            expandedInput2[0] = expandedInput;
        }
    }

    if (num_args > 1)
    {

        i = 0;
        j = 0;

        token = strtok_r(input, " ", &saveptr);

        while (token != NULL)
        {

            if (strcmp(token, "$$") != 0)
            {
                for (i = 0; i < (int)strlen(token); i++)
                {
                    if (token[i] == '$' && token[i + 1] == '$')
                    {
                        token2 = strtok_r(token, "$$", &saveptr2);
                        strcpy(expandedInput, token2);
                        strcat(expandedInput, convertedPid);

                        expandedInput2[j] = calloc(strlen(token) + 1, sizeof(char));

                        strcpy(expandedInput2[j], expandedInput);
                        j++;
                    }
                }

                expandedInput2[j] = calloc(strlen(token) + 1, sizeof(char));
                strcpy(expandedInput2[j], token);
                j++;
            }

            if (strcmp(token, "$$") == 0)
            {
                expandedInput2[j] = calloc(strlen(token) + 1, sizeof(char));

                strcpy(expandedInput2[j], convertedPid);
                j++;
            }

            token = strtok_r(NULL, " ", &saveptr);
        }
    }

    return expandedInput2;
}

void handle_SIGTSTP(int signo)
{
    if (!fmode)
    {
        char *message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        fmode = 1;
    }
    else
    {
        char *message = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        fmode = 0;
    }
}

int main(int argc, char *argv[])
{
    size_t line_length = 2048;
    bool programRun = true;
    char **arguments = NULL;
    char **arguments2 = NULL;
    char **arguments3 = NULL;
    char *commandLine = NULL;

    /* List of all vars I'm using for the arguments checks and following calls for function*/
    int Status;
    int count_args;
    int count_args2;
    int count_args3;
    int i;
    char **convertedProcID;
    bool expandExist = false;
    struct sigaction ignore_action = {0};
    struct sigaction SIGTSTP_action = {0};
    struct sigaction ignore_SIGTSTP = {0};
    int saved_stdout = dup(1);
    int saved_stdin = dup(0);
    int x = 0;
    ignore_action.sa_handler = SIG_IGN;
    ignore_SIGTSTP.sa_handler = SIG_DFL;
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    sigaction(SIGINT, &ignore_action, NULL);
    struct Proccs *arrayOfProccs = malloc(512 * sizeof(struct Proccs));
    for (i = 0; i < 512; i++)
    {
        arrayOfProccs[i].arrayID = -1;
    }

    while (programRun)
    {
        expandExist = false;
        background = false;

        for (i = 0; i < 512; i++)
        {
            if (arrayOfProccs[i].arrayID != -1)
            {
                if (waitpid(arrayOfProccs[i].arrayID, &(arrayOfProccs[i].status), WNOHANG) != 0)
                {
                    printf("background pid %d is done: ", arrayOfProccs[i].arrayID);
                    fflush(stdout);
                    arrayOfProccs[i].arrayID = -1;
                    if (WIFEXITED(arrayOfProccs[i].status))
                    {
                        printf("exit value %d\n", WEXITSTATUS(arrayOfProccs[i].status));
                        fflush(stdout);
                    }
                    else
                    {
                        printf("terminated by signal %d\n", WTERMSIG(arrayOfProccs[i].status));
                        fflush(stdout);
                    }
                }
            }
        }

        commandLine = getInput(commandLine, line_length);

        if (commandLine[strlen(commandLine) - 1] == '&')
        {
            if (fmode == 0)
            {
                background = true;
            }
            else
            {
                background = false;
            }
        }

        /* Creating different command lines for the further tokenizing function because they meessed up original command line
after being called, I used each command line for different tokenizing functions*/
        char *commandLine2 = malloc((int)strlen(commandLine) * sizeof(char));
        strcpy(commandLine2, commandLine);

        char *commandLine3 = malloc((int)strlen(commandLine) * sizeof(char));
        strcpy(commandLine3, commandLine);

        char *commandLine4 = malloc((int)strlen(commandLine) * sizeof(char));
        strcpy(commandLine4, commandLine);

        count_args = 0;
        arguments = tokenizeCommandLine(commandLine, arguments, &count_args);

        /* these arguments are skippin < and > symbols */
        count_args2 = 0;
        arguments2 = tokenizeExistingArr(commandLine2, arguments2, &count_args2);

        count_args3 = 0;
        arguments3 = tokenizeExistingArr2(commandLine4, arguments3, &count_args3);

        /* Determining whether there is a $$ var in the command line*/
        int j = 0;
        for (i = 0; i < count_args; i++)
        {
            if (strcmp(arguments[i], "$$") == 0)
            {
                expandExist = true;
            }
            else
            {
                for (j = 0; j < (int)strlen(arguments[i]); j++)
                {
                    if (arguments[i][j] == '$' && arguments[i][j + 1] == '$')
                    {
                        expandExist = true;
                    }
                }
            }
        }

        /* basically if expand exist I would assign a expansion var 2 d array of chars just to later use it
        in my function call with shellcommands and passing convertedProcID there*/
        if (expandExist == true)
        {
            convertedProcID = expansionVar(commandLine3, getpid(), count_args, arguments);
            fflush(stdout);
        }

        if (count_args > 1)
        {
            Redirection(&Status, arguments, count_args);
        }

        if (fmode == 1)
        {
           
            shellCommands(arguments3, count_args3, &programRun, &Status, arrayOfProccs, &x, ignore_action, SIGTSTP_action, ignore_SIGTSTP);
            
            
        }

        if (expandExist == false)
        {
            if (background == true)
            {
                shellCommands(arguments3, count_args3, &programRun, &Status, arrayOfProccs, &x, ignore_action, SIGTSTP_action, ignore_SIGTSTP);
            }

            else
            {
                shellCommands(arguments2, count_args2, &programRun, &Status, arrayOfProccs, &x, ignore_action, SIGTSTP_action, ignore_SIGTSTP);
            }
        }

        if (expandExist == true)
        {
            shellCommands(convertedProcID, count_args, &programRun, &Status, arrayOfProccs, &x, ignore_action, SIGTSTP_action, ignore_SIGTSTP);
        }

        x = 0;
        dup2(saved_stdout, 1);
        dup2(saved_stdin, 0);
    } /*end of while loop*/

    return 0;
}