#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

/* your list function definitions */

void list_clear(Node *list) {
    while (list != NULL) {
        struct node *tmp = list;
        list = list->next;
        free(tmp);
    }
}

void list_append(const char *data, const char *additional_data, Node **head) {
    Node *newnode = malloc(sizeof(Node));
    strcpy (newnode->data, data);
    strcpy(newnode->additional_data, additional_data);
    newnode->next = NULL;
    struct node *curr = *head;
    if (curr == NULL) {
        *head = newnode;
        return;
    }
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = newnode;
    newnode->next = NULL;
}

void list_delete(const char *name, Node **head) {
    if (*head != NULL && strcasecmp((*head)->data, name) == 0) {
        struct node *tmp = *head;
        *head = (*head)->next;
        free(tmp);
    }
    else {
        struct node *curr = *head;  
        while (curr != NULL && curr->next != NULL) {
            if (strcasecmp((curr->next)->data, name) == 0) {
                struct node *tmp = curr->next;
                curr->next = tmp->next;
                free(tmp);
                break;
            }
            curr = curr->next;  
        }
    }
}

int list_matches(const char *name, const struct node *head) {
    const struct node *curr = head;
    while (curr != NULL) {
        if (strcasestr(curr->data, name) != NULL) {
            return 1;
        }
        curr = curr->next;
    }
    return 0;
    
}



