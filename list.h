#ifndef __LIST_H__
#define __LIST_H__

/* your list data structure declarations */

typedef struct node {
    char data[128]; 
    char additional_data[128];
    struct node *next;
} Node;

/* your function declarations associated with the list */
void list_append(const char *data, const char *additional_data, Node **head);
void list_clear(Node *list);
void list_delete(const char *name, struct node **head);
int list_matches(const char *name, const struct node *head);

#endif // __LIST_H__
