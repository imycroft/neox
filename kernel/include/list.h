#ifndef LIST_H
#define LIST_H

struct list_node
{
    struct list_node *prev;
    struct list_node *next;
};

struct list
{
    struct list_node head;
};

// API

/*
 * Initialize a list node.
 *
 * A node must be initialized before it can be
 * inserted into a list.
 */
void list_node_init(struct list_node *node);

void list_init(struct list *list);

bool list_empty(const struct list *list);

void list_push_front(struct list *list,
                     struct list_node *node);

void list_push_back(struct list *list,
                    struct list_node *node);

void list_insert_before(struct list_node *pos,
                        struct list_node *node);

void list_insert_after(struct list_node *pos,
                       struct list_node *node);

void list_remove(struct list_node *node);

struct list_node *list_front(struct list *list);

struct list_node *list_back(struct list *list);

struct list_node *list_next(struct list_node *node);

struct list_node *list_prev(struct list_node *node);

#endif
