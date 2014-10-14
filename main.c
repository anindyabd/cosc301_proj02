#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>

#include "list.h"

/* 
 * Tokenizes the input string using spaces as delimiters.
 */
char** tokenify_by_space(const char *s) {
    char *strcopy = strdup(s); 
    char *token = strtok(strcopy, " \n\t");
    int str_arr_size = sizeof(token);
    while (token != NULL) {
        token = strtok(NULL, " \n\t");
        str_arr_size += sizeof(token);
    }
    free(strcopy);
    char *another_copy = strdup(s);
    char *another_token = strtok(another_copy, " \n\t");
    char **strarr = malloc(str_arr_size);
    int count = 0;
    while (another_token != NULL) {
        strarr[count] = strdup(another_token);
        another_token = strtok(NULL, " \n\t");
        count++;
    }
    free(another_copy);
    strarr[count] = NULL;   
    return strarr; 
}

/*
 * Tokenizes the input string using semicolons as delimiters.
 */

char **tokenify_by_semicolon(const char *s) {
    char *strcopy = strdup(s); 
    char *token = strtok(strcopy, ";");
    int str_arr_size = sizeof(token);
    while (token != NULL) {
        token = strtok(NULL, ";");
        str_arr_size += sizeof(token);
    }
    free(strcopy);
    char *another_copy = strdup(s);
    char *another_token = strtok(another_copy, ";");
    char **strarr = malloc(str_arr_size);
    int count = 0;
    while (another_token != NULL) {
        strarr[count] = strdup(another_token);
        another_token = strtok(NULL, ";");
        count++;
    }
    free(another_copy);
    strarr[count] = NULL;   
    return strarr;    
}

/* 
 * Frees up the heap space taken up by array of tokens.
*/
void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
}

/* 
 * Replaces the '#' sign with a null byte. 
 */
char *replace_pound(char *str) {
    char *newstr = malloc(strlen(str) + 1);
    int j = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] != '#') {
            newstr[j] = str[i];
            j++;
        }
        else {
            str[i] = '\0';
            break;
        }
    }
    newstr[j] = '\0';
    return newstr;
}

char handle_mode(char **command, char *mode) { 
    if (strcmp(command[0], "mode") == 0) {
        if (command[1] == NULL) {
            printf("%s\n", mode);
            return mode[0];
        }
        else if (strcmp(command[1], "parallel") == 0 || strcmp(command[1], "p") == 0) {
            return 'p';
        }
        else if (strcmp(command[1], "sequential") == 0 || strcmp(command[1], "s") == 0) {
            return 's';
        }
    }
    return mode[0];
}

char handle_exit(char **command) {
    if (strcmp(command[0], "exit") == 0 && command[1] == NULL) {
        return 'e';
    }
    return 'n';
}

int is_mode(char **command) {
    if (strcmp(command[0], "mode") == 0) {
        if (command[1] == NULL) { 
            return 1;
        }
        else if (strcmp(command[1], "sequential") == 0 || strcmp(command[1], "s") == 0) {
            return 1;
        } 
        else if (strcmp(command[1], "parallel") == 0 || strcmp(command[1], "p") == 0) {
            return 1;
        }
    }
    return 0;
}

char *prepend_path(char **command, Node *head) {
    struct stat statresult;
    char *tempcommand = malloc(1024*sizeof(char));
    memset (tempcommand,'\0', 1024);
    strcpy (tempcommand, command[0]);
    int rv = stat(command[0], &statresult);
    if (rv < 0) {
        while (head != NULL && rv != 0) {
            memset (tempcommand, '\0', 1024);
            strcat(tempcommand, head->data);
            strcat(tempcommand, "/");
            strcat(tempcommand, command[0]);
            rv = stat(tempcommand, &statresult);
            head = head ->next;    
        }
    }
    return tempcommand;
}

char sequential_mode(char **pointers_to_commands, Node *head) {
    char *mode = "sequential";
    pid_t cpid, w;
    char **command;
    char return_char = 's';
    int i = 0;
    while (pointers_to_commands[i] != NULL) {
        command = tokenify_by_space(pointers_to_commands[i]);
        if (command[0] == NULL) {
            i++;
            free_tokens(command);
            continue;
        } 
        if (handle_exit(command) != 'n') {
            free_tokens(command);
            return 'e';
        }
        if (return_char != 'e' && is_mode(command)) {
            return_char = handle_mode(command, mode);
            i++;
            free_tokens(command);
            continue;
        }
        cpid = fork();
        if (cpid == 0) {
            char *tempcommand;
            tempcommand = prepend_path(command, head);
            command[0] = tempcommand;
            if (execv(command[0], command) < 0) {
                fprintf(stderr, "execv failed: %s\n", strerror(errno));
                printf("That's not a valid command! \n");
                free_tokens(command);
                exit(EXIT_FAILURE);
            }
            free(tempcommand);
            }
        else {
            int status = 0;
            w = wait(&status);
            i++;
        }
        free_tokens(command);

    }
    return return_char;
}

char parallel_mode(char **pointers_to_commands, Node *head) {
    char *mode = "parallel";
    pid_t cpid, w;
    char **command;
    char return_char = 'p';
    int count = 0;
    int k = 0;
    while (pointers_to_commands[k] != NULL) {
        count++;
        k++;
    }
    int cpidlist[count];
    memset(cpidlist, -1, count); 
    int i = 0;
    while (pointers_to_commands[i] != NULL) {
        command = tokenify_by_space(pointers_to_commands[i]);
        if (command[0] == NULL) {
            i++;
            free_tokens(command);
            continue;
        }
        if (handle_exit(command) != 'n') {
            return_char ='e';
            free_tokens(command);
            i++;
            continue;
        }
        if (return_char != 'e' && is_mode(command)) {
            return_char = handle_mode(command, mode);
            free_tokens(command);
            i++;
            continue;
        }
        else if (return_char == 'e' && is_mode(command)) {
            handle_mode(command, mode);
            free_tokens(command);
            i++;
            continue;
        }
        cpid = fork();
        if (cpid == 0) {
            char *tempcommand;
            tempcommand = prepend_path(command, head);
            command[0]= tempcommand;
            if (execv(command[0], command) < 0) {
                fprintf(stderr, "execv failed: %s\n", strerror(errno));
                printf("That's not a valid command! \n");
                free_tokens(command);
                exit(EXIT_FAILURE);
            }        
        }
        i++;
        free_tokens(command);
    }     
    int status;
    int j = 0;
    while (pointers_to_commands[j] != NULL) {
        w = waitpid(-1, &status, 0);
        j++;
    }
    return return_char;
}

int main(int argc, char **argv) {

    char prompt[1024];
    memset(prompt, '\0', 1024);
    getcwd(prompt, 1024);
    int len = strlen(prompt);
    prompt[len + 1] = ' ';
    prompt[len] = '$';
    printf("%s", prompt);
    fflush(stdout);

    FILE *datafile = NULL;
    datafile = fopen("shell-config", "r");
    Node *head = NULL;
    if (datafile != NULL) {
        char line[1024];
        while (fgets(line, 1024, datafile) != NULL) {
            int slen = strlen(line);
            line[slen-1] = '\0';
            list_append(line, &head);
        }
    }
    fclose(datafile);
    char curr_mode = 's';
    char buffer[1024];
    while (fgets(buffer, 1024, stdin) != NULL) {
        char *newbuffer = replace_pound(buffer);
        newbuffer[strlen(newbuffer)-1] = '\0'; 
        char **pointers_to_commands = tokenify_by_semicolon(newbuffer);
        free(newbuffer);
        if (pointers_to_commands[0] == NULL) {
            printf("%s", prompt);
            fflush(stdout); 
            continue;
        }
        if (curr_mode == 's') {
            curr_mode = sequential_mode(pointers_to_commands, head); 
        }
        else if (curr_mode == 'p') {
            curr_mode = parallel_mode(pointers_to_commands, head);
        } 
        if (curr_mode == 'e') {
            free_tokens(pointers_to_commands);
            list_clear(head);
            exit(0);
        }
        printf("%s", prompt);
        fflush(stdout);
        free_tokens(pointers_to_commands);
    }
    list_clear(head);
    printf("\n");
    return 0;
}

