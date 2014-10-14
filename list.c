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

void list_append(const char *data, Node **head) {
    Node *newnode = malloc(sizeof(Node));
    strcpy (newnode->data, data);
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




