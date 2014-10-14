#ifndef __LIST_H__
#define __LIST_H__

/* your list data structure declarations */

typedef struct node {
    char data[128]; 
    struct node *next;
} Node;

/* your function declarations associated with the list */
void list_append(const char *data, Node **head);
void list_clear(Node *list);

#endif // __LIST_H__
