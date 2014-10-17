/*
    Anindya Guha
    10/16/2014
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
 * Checks if the str is a valid int. Returns 0 if false, 1 if true.
 */
int valid_int(char *str){

    if (str == NULL || str[0] == '\0' || (isdigit(str[0]) == 0 && str[0] != '-')) {
        return 0;
    }
    for(int i = 1; i < strlen(str); i++){
        if (isdigit(str[i]) == 0) {
            return 0;
        } 
    }
    return 1;
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
 * Checks if the command is 'mode' or one of the mode changing commands.
 * Necessary so that I only call handle_mode() if the command is a mode command.
 */
int is_mode(char **command) {
    if (strcmp(command[0], "mode") == 0) {
        if (command[1] == NULL) { 
            return 1;
        }
        else if (strcmp(command[1], "sequential") == 0 || strcmp(command[1], "s") == 0) {
            return 2;
        } 
        else if (strcmp(command[1], "parallel") == 0 || strcmp(command[1], "p") == 0) {
            return 3;
        }
    }
    return 0;
}

/* 
 * Handles changing the mode (by returning the char representing the new (future) mode) and printing the current mode.
 */
char handle_mode(char **command, char curr_mode, char future_mode) { 
    if (is_mode(command) == 1) {
        if (curr_mode == 'p') {
            printf("parallel\n");
        }
        else if (curr_mode == 's') {
            printf("sequential\n");
        }
    }
    if (is_mode(command) == 2) {
        future_mode = 's';
    }
    if (is_mode(command) == 3) {
        future_mode = 'p';
    }
    return future_mode;
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
 * Handles the builtin methods (jobs, pause PID and resume PID) to support background processes in parallel mode.
 * Returns 1 if the command is one of the three builtins, 0 if not.
 */

int handle_parallel_builtins(char **command, Node **paused_list, Node **cpidlist, char mode) {
    if (strcmp(command[0], "jobs") != 0 && strcmp(command[0], "pause") != 0 && strcmp(command[0], "resume") != 0) {
        return 0;
    }
    else if (mode != 'p') {
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
        if (valid_int(command[1])) {
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
        if (valid_int(command[1]    )) {
            return_int = 1;
            pid_t pid = atoi(command[1]);
            if (!list_matches(command[1], *paused_list)) {
                printf("Either that's not a process, or you're trying to continue a process that's not paused!\n");
            
            }
            else {
                if (kill(pid, SIGCONT) == 0) {
                    list_delete(command[1], paused_list);
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
    char mode = 's';
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
            return_char = handle_mode(command, mode, return_char);
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
 * Runs commands in parallel mode, with support for background processes.  
 */
char parallel_mode(char **pointers_to_commands, Node *head, Node **paused_list, Node **cpidlist) {

    int count = 0; // counts the no. of times we've been in the loop; 
                    // if more than 1, we need to free
                    // pointers_to_commands, which is different from what we got from main.
    while (1) {
        count++;
        char mode = 'p';
        pid_t cpid;
        char **command;
        char return_char = 'p';
        int i = 0;

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
                return_char = handle_mode(command, mode, return_char);
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
        if (count > 1) {
            free_tokens(pointers_to_commands);
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
            int rv = poll(&pfd[0], 1, 800);
            Node *to_delete_list = NULL;
            Node *tempcpidlist = *cpidlist;
            pid_t w;
            if (rv == 0) {
                some_process_completed = 0;
                while (tempcpidlist != NULL) {
                    w = atoi(tempcpidlist->data);
                    // I know that the ideal way to check for child process death is to use a macro such as WIFEXITED on status, 
                    // but it wasn't working for me.
                    // status always had the value 0. So I'm doing this instead to check for process completion.
                    if (waitpid(w, &status, WUNTRACED|WNOHANG) == -1) { 
                        list_append(tempcpidlist->data, "", &to_delete_list);
                        printf("\nProcess %s (%s) completed.\n", tempcpidlist->data, tempcpidlist->additional_data); 
                        some_process_completed = 1;
                    }
                    tempcpidlist = tempcpidlist->next;                 
                }
                Node *curr = to_delete_list;
                while (curr != NULL) {
                    list_delete(curr->data, cpidlist);
                    curr = curr->next;
                }
                list_clear(to_delete_list);
                if (some_process_completed == 1) {
                    display_prompt();

                }
            }
            else if (rv > 0) {
                char buffer[1024];
                if (fgets(buffer, 1024, stdin) == NULL) {
                    if (*cpidlist != NULL) {
                        printf("There are processes still running. You can't exit now.\n");
                        display_prompt();
                    }
                    else {
                        return_char = 'e';
                        return return_char;
                    }
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
            list_clear(paused_list);
            list_clear(cpidlist);
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

