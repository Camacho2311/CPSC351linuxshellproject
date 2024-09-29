/*
 * CPSC 351: Operating Systems
 * Students: Alberto Camacho, Logan Brenner
 * Project: Linux Shell Project
 * Date: September 29, 2024
 * 
 * Description: 
 * This project implements a simple Linux shell capable of handling basic 
 * commands, built-in commands (cd, help, mkdir, !!, etc.), and piping 
 * between commands. The shell allows for command-line input and provides 
 * feedback through custom processing.
 * 
 * Collaboration: 
 * - ChatGPT for code assistance, clarification, and implementation suggestions.
 * - GeeksforGeeks for reference on Unix system calls and shell functionalities.
 * 
 * This project was created for educational purposes as part of the CPSC 351 
 * course, focusing on the fundamental concepts of operating systems and 
 * process management.
 */

// C Program to design a shell in Linux
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>

#define MAXCOM 1000 // max number of letters to be supported
#define MAXLIST 100 // max number of commands to be supported

// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")

// Greeting shell during startup
void init_shell()
{
    clear();
    printf("\n********************************");
    printf("\n  Welcome to Our Custom Shell!");
    printf("\n********************************");
    
    char* username = getenv("USER");
    printf("\nUser: @%s", username);
    printf("\nType 'help' to see available commands.");
    
    sleep(1); // Short pause for effect
    clear();  // Clear the screen before starting
}


// Function to take input
int takeInput(char* str)
{
    char* buf;

    buf = readline("\n>>> ");
    if (strlen(buf) != 0) {
        add_history(buf);
        strcpy(str, buf);
        return 0;
    } else {
        return 1;
    }
}

// Function to print Current Directory.
void printDir()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\nDir: %s", cwd);
}

// Function where the system command is executed
void execArgs(char** parsed)
{
    // Forking a child
    pid_t pid = fork(); 

    if (pid == -1) {
        printf("\nFailed forking child..");
        return;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command..");
        }
        exit(0);
    } else {
        // waiting for child to terminate
        wait(NULL); 
        return;
    }
}

// Function where the piped system commands is executed
void execArgsPiped(char** parsed, char** parsedpipe)
{
    // 0 is read end, 1 is write end
    int pipefd[2]; 
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork");
        return;
    }

    if (p1 == 0) {
        // Child 1 executing..
        // It only needs to write at the write end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    } else {
        // Parent executing
        p2 = fork();

        if (p2 < 0) {
            printf("\nCould not fork");
            return;
        }

        // Child 2 executing..
        // It only needs to read at the read end
        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        } else {
            // parent executing, waiting for two children
            wait(NULL);
            wait(NULL);
        }
    }
}

// Help command builtin
void openHelp()
{
    puts("\n***WELCOME TO OUR CUSTOM SHELL HELP***"
         "\n- Designed by our project group for educational purposes."
         "\n- This shell includes basic UNIX command handling and custom features."
         "\n\nList of Commands supported:"
         "\n> cd      - Change the current directory."
         "\n> ls      - List directory contents."
         "\n> exit    - Exit the shell."
         "\n> help    - Display this help message."
         "\n> hello   - Greet the user."
         "\n> pipe handling - Supports piping between commands (e.g., `command1 | command2`)."
         "\n> space handling - Replaces spaces in commands with the word 'SPACE' for custom processing."
         "\n\nFeel free to explore these commands as part of the project and adapt them as needed!");

    return;
}


// Function to execute builtin commands
int ownCmdHandler(char** parsed, char* lastCommand)
{
    int NoOfOwnCmds = 5, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;

    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";
    ListOfOwnCmds[2] = "help";
    ListOfOwnCmds[3] = "hello";
    ListOfOwnCmds[4] = "!!";

    for (i = 0; i < NoOfOwnCmds; i++) {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }

    switch (switchOwnArg) {
    case 1:
        printf("\nGoodbye\n");
        exit(0);
    case 2:
        if (parsed[1] == NULL) {
        // If no argument is provided, change to home directory
        char* home = getenv("HOME");
        if (home != NULL) {
            chdir(home);
        } else {
            printf("\ncd: HOME environment variable not set.\n");
        }
    } else {
        // Change to the specified directory
        if (chdir(parsed[1]) != 0) {
            perror("cd");
        }
    }
    return 1;
    case 3:
        openHelp();
        return 1;
    case 4:
        username = getenv("USER");
        printf("\nHello %s!\nWelcome to our group's custom shell project."
               "\nFeel free to explore the commands we've implemented."
               "\nUse 'help' to see a list of supported commands.\n",
               username);
        return 1;
    case 5:  // Case for `!!`
        if (strlen(lastCommand) == 0) {
            printf("\nNo previous command to run.\n");
        } else {
            printf("\nRunning last command: %s\n", lastCommand);
            strcpy(parsed[0], lastCommand);  // Replace current command with the last command
            return 2;  // Treat it like a regular command
        }
        return 1;
    default:
        break;
    }

    return 0;  // Return 0 if it's not a built-in command
}



// Project requirement 3 (print PIPE)
// function for finding pipe
int parsePipe(char* str, char** strpiped)
{
    int i;
    for (i = 0; i < 2; i++) {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
            break;
    }

    // If a pipe is found
    if (strpiped[1] != NULL) {
        printf(" PIPE ");
        return 1; // returns one if a pipe is found.
    } else {
        return 0; // returns zero if no pipe is found.
    }
}


// function for parsing command words
void parseSpace(char* str, char** parsed)
{
    int i;

    for (i = 0; i < MAXLIST; i++) {
        parsed[i] = strsep(&str, " ");

        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
}



//Project requirement 1 & 2 (ECHO/SPACE)
int processString(char* str, char** parsed, char** parsedpipe, char* lastCommand)
{
    char* strpiped[2];
    int piped = 0;

    piped = parsePipe(str, strpiped);

    // Parse the command with or without a pipe
    if (piped) {
        parseSpace(strpiped[0], parsed);
        parseSpace(strpiped[1], parsedpipe);
    } else {
        parseSpace(str, parsed);
    }

    // Count the number of words in the parsed command
    int i = 0;
    while (parsed[i] != NULL) {
        i++;
    }

    // Print the input based on whether there's a pipe
    if (piped) {
        printf("\nProcessed Input: ");
        for (int j = 0; parsed[j] != NULL; j++) {
            if (j > 0) {
                printf(" SPACE ");
            }
            printf("%s", parsed[j]);
        }
        printf(" PIPE ");
        for (int j = 0; parsedpipe[j] != NULL; j++) {
            if (j > 0) {
                printf(" SPACE ");
            }
            printf("%s", parsedpipe[j]);
        }
        printf("\n");
    } else {
        printf("\nProcessed Input: ");
        for (int j = 0; j < i; j++) {
            if (j > 0) {
                printf(" SPACE ");
            }
            printf("%s", parsed[j]);
        }
        printf("\n");
    }

    // Check if the last word is "ECHO"
    if (i > 0 && strcmp(parsed[i - 1], "ECHO") == 0) {
        // If ECHO is the last word, print each word on a new line, excluding "ECHO"
        for (int j = 0; j < i - 1; j++) {
            printf("%s\n", parsed[j]);
        }
        return 0; // Do not execute commands after echoing
    }

    // Now handle built-in commands (pass lastCommand for the `!!` functionality)
    if (ownCmdHandler(parsed, lastCommand)) {
        return 0; // Built-in command was executed, no further action needed
    }

    // If it's not a built-in command, return 1 or 2 based on whether there's a pipe
    return 1 + piped; // 1 for a simple command, 2 if a pipe is found
}



int main()
{
    char inputString[MAXCOM], lastCommand[MAXCOM] = ""; // Add lastCommand variable
    char *parsedArgs[MAXLIST];
    char *parsedArgsPiped[MAXLIST];
    int execFlag = 0;
    init_shell();

    while (1) {
        // print shell line
        printDir();

        // take input
        if (takeInput(inputString))
            continue;

        // Store the input as the last command if it's not empty and not `!!`
        if (strcmp(inputString, "!!") != 0 && strlen(inputString) > 0) {
            strcpy(lastCommand, inputString);
        }

        // process
        execFlag = processString(inputString, parsedArgs, parsedArgsPiped, lastCommand);


        // execFlag returns zero if there is no command or it is a built-in command,
        // 1 if it is a simple command,
        // 2 if it includes a pipe.

        // execute
        if (execFlag == 1)
            execArgs(parsedArgs);

        if (execFlag == 2)
            execArgsPiped(parsedArgs, parsedArgsPiped);
    }
    return 0;
}

