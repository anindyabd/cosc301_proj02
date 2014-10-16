/*
    Anindya Guha
 */
#define _GNU_SOURCE
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
 * Tokenizes the input string using the given delimiters.
 */
char** tokenify(const char *s, char *delimiters) {
    char *strcopy = strdup(s); 
    char *token = strtok(strcopy, delimiters);
    int str_arr_size = sizeof(token);
    while (token != NULL) {
        token = strtok(NULL, delimiters);
        str_arr_size += sizeof(token);
    }
    free(strcopy);
    char *another_copy = strdup(s);
    char *another_token = strtok(another_copy, delimiters);
    char **strarr = malloc(str_arr_size);
    int count = 0;
    while (another_token != NULL) {
        strarr[count] = strdup(another_token);
        another_token = strtok(NULL, delimiters);
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

/* 
 * Handles changing the mode and printing the current mode.
 */
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

/*
 * Returns 'e' if the command being inspected is 'exit'.
 */
char handle_exit(char **command) {
    if (strcmp(command[0], "exit") == 0 && command[1] == NULL) {
        return 'e';
    }
    return 'n';
}

/*
 * Checks if the command is 'mode' or one of the mode changing commands.
 */
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

int handle_parallel_builtins(char **command, Node **paused_list, Node **cpidlist, char *mode) {
    if (strcmp(command[0], "jobs") != 0 && strcmp(command[0], "pause") != 0 && strcmp(command[0], "resume") != 0) {
        return 0;
    }
    else if (strcmp(mode, "parallel") != 0) {
        printf("This command is only supported in parallel mode.\n");
        return 0;
    }
    char return_int = 0;
    if (strcmp(command[0], "jobs") == 0 && command[1] == NULL) {
        return_int = 1;
        Node *tempcpidlist = *cpidlist;
        char *status = "Running";
        while (tempcpidlist != NULL) {
            
            if (list_matches(tempcpidlist->data, *paused_list)) {
                status = "Paused";
            }
            printf("Process %s (%s): %s \n", tempcpidlist->data, tempcpidlist->additional_data, status);
            tempcpidlist = tempcpidlist->next;
        }
    }
    else if(strcmp(command[0], "pause") == 0 && command[1] != NULL) {
        if (isdigit(command[0])) {
            return_int = 1;
            pid_t pid = atoi(command[1]);
            if (kill(pid, SIGSTOP) == 0) {
                list_append(command[1], "", paused_list);
            }
            else {
                fprintf(stderr, "pause failed: %s\n", strerror(errno));
            }
        }
    }
    else if(strcmp(command[0], "resume") == 0 && command[1] != NULL) {
        if (isdigit(command[0])) {
            return_int = 1;
            pid_t pid = atoi(command[1]);
            if (!list_matches(command[1], *paused_list)) {
                printf("You can't continue a process that's not paused!");
            
            }
            else {
                if (kill(pid, SIGCONT) == 0) {
                    list_append(command[1], "", paused_list);
            }
                else {
                    fprintf(stderr, "resume failed: %s\n", strerror(errno));
                }
            }
            
        }
    }
    return return_int;
}


/*
 * Prepends the paths in shell-config to the current command until a valid command is found  
 * or there are no more paths to prepend.
 */
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

/* 
 * Runs commands in sequential mode.
 */
char sequential_mode(char **pointers_to_commands, Node *head) {
    char *mode = "sequential";
    pid_t cpid, w;
    char **command;
    char return_char = 's';
    int i = 0;
    while (pointers_to_commands[i] != NULL) {
        command = tokenify(pointers_to_commands[i], " \n\t");
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
        handle_parallel_builtins(command, NULL, NULL, mode);
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
/*
 * Displays the prompt, which is the current working directory followed by '$'.
 */
void display_prompt() {
    char prompt[1024];
    memset(prompt, '\0', 1024);
    if (getcwd(prompt, 1024) == NULL) { 
        memset(prompt, '\0', 1024);
        strcpy(prompt, "cmd ");    
    }   
    int len = strlen(prompt);
    prompt[len + 1] = ' ';
    prompt[len] = '$';
    printf("%s", prompt);
    fflush(stdout);
}


/* 
 * Runs commands in parallel mode, with support for background processes. Things get a little messy (read: very messy) here.  
 */
char parallel_mode(char **pointers_to_commands, Node *head, Node **paused_list, Node **cpidlist) {

    while (1) {
        int i = 0;
        char *mode = "parallel";
        pid_t cpid;
        char **command;
        char return_char = 'p';
        while (pointers_to_commands[i] != NULL) {
            command = tokenify(pointers_to_commands[i], " \n\t");
            if (command[0] == NULL) {
                i++;
                free_tokens(command);
                continue;
            }
            if (handle_exit(command) != 'n') {
                if (*cpidlist == NULL && pointers_to_commands[i+1] == NULL) {
                    return_char = 'e';
                }
                else {
                    printf("There are processes still running. You cannot exit yet. \n");
                }
                free_tokens(command);
                i++;
                continue;
            }
            if (is_mode(command)) {
                return_char = handle_mode(command, mode);
                if (return_char == 's') {
                    if (*cpidlist != NULL || pointers_to_commands[i+1] != NULL) {
                        printf("There are processes running. You cannot switch modes yet.\n");
                        return_char = 'p';
                    }
                }
                free_tokens(command);
                i++;
                continue;
            }
            if (handle_parallel_builtins(command, paused_list, cpidlist, mode) == 1) {
                i++;
                free_tokens(command);
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
            else {
                char whole_command[128];
                memset(whole_command, '\0', 128);
                strcat(whole_command, command[0]);
                if (command[1] != NULL) {
                    strcat(whole_command, " ");
                    strcat(whole_command, command[1]);
                }
                char cpidstr[128];
                sprintf(cpidstr, "%d", cpid);
                list_append(cpidstr, whole_command, cpidlist);
            }
            i++;
            free_tokens(command);
        }

        if (return_char != 'p') {
            return return_char;
        }
        
        struct pollfd pfd[1];
        pfd[0].fd = 0;
        pfd[0].events = POLLIN;
        pfd[0].revents = 0;

        display_prompt();     
        int some_process_completed = 0;
        while (1) {
            int status;
            int rv = poll(&pfd[0], 1, 1000);
            Node *to_delete_list = NULL;
            Node *tempcpidlist = *cpidlist;
            pid_t w;
            if (rv == 0) {
                some_process_completed = 0;
                while (tempcpidlist != NULL) {
                    w = atoi(tempcpidlist->data);
                    // I know that the ideal way is to use a macro such as WIFEXITED on status, but it wasn't working for me.
                    // status always had the value 0. So I'm doing this instead to check for process completion.
                    if (waitpid(w, &status, WUNTRACED|WNOHANG) == -1) { 
                        list_append(tempcpidlist->data, "", &to_delete_list);
                        printf("\nProcess %s completed.\n", tempcpidlist->data); 
                        some_process_completed = 1;
                    }
                    tempcpidlist = tempcpidlist->next;                 
                }
                while (to_delete_list != NULL) {
                    list_delete(to_delete_list->data, cpidlist);
                    to_delete_list = to_delete_list->next;
                }
                if (some_process_completed == 1) {
                    display_prompt();
                }
            }
            else if (rv > 0) {
                char buffer[1024];
                if (fgets(buffer, 1024, stdin) == NULL) {

                }
                else {
                    char *newbuffer = replace_pound(buffer);
                    newbuffer[strlen(newbuffer)-1] = '\0'; 
                    pointers_to_commands = tokenify(newbuffer, ";");
                    free(newbuffer);
                    break;
                }
            }
            else {
                printf("there was some kind of error: %s \n", strerror(errno));
            }
        }
    }
}



int main(int argc, char **argv) {

    display_prompt(); 

    FILE *datafile = NULL;
    datafile = fopen("shell-config", "r");
    Node *head = NULL;
    if (datafile != NULL) {
        char line[1024];
        while (fgets(line, 1024, datafile) != NULL) {
            int slen = strlen(line);
            line[slen-1] = '\0';
            list_append(line, "", &head);
        }
        fclose(datafile);
    }
    char curr_mode = 's';
    char buffer[1024];
    Node *paused_list = NULL;
    Node *cpidlist = NULL;
    while (fgets(buffer, 1024, stdin) != NULL) {
        char *newbuffer = replace_pound(buffer);
        newbuffer[strlen(newbuffer)-1] = '\0'; 
        char **pointers_to_commands = tokenify(newbuffer, ";");
        free(newbuffer);
        if (pointers_to_commands[0] == NULL) {
            display_prompt();
            continue;
        }
        if (curr_mode == 's') {
            curr_mode = sequential_mode(pointers_to_commands, head); 
        }
        else if (curr_mode == 'p') {
            
            curr_mode = parallel_mode(pointers_to_commands, head, &paused_list, &cpidlist);
        } 
        if (curr_mode == 'e') {
            free_tokens(pointers_to_commands);
            list_clear(head);
            exit(0);
        }
        display_prompt();
        free_tokens(pointers_to_commands);
    }
    list_clear(head);
    list_clear(paused_list);
    list_clear(cpidlist);
    printf("\n");
    return 0;
}

