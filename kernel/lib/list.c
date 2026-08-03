#include "types.h"
#include "list.h"
#include "assert.h"
struct list_node *list_front(struct list *list)
{
    if (list == NULL)
        return NULL;

    return list->head.next;
}

struct list_node *list_back(struct list *list)
{
    if (list == NULL)
        return NULL;

    return list->head.prev;
}

struct list_node *list_next(struct list_node *node)
{
    if (node == NULL)
        return NULL;

    return node->next;
}

struct list_node *list_prev(struct list_node *node)
{
    if (node == NULL)
        return NULL;

    return node->prev;
}

void list_node_init(struct list_node *node)
{
    ASSERT(node != NULL);

    node->prev = NULL;
    node->next = NULL;
}

void list_init(struct list *list)
{
    ASSERT(list != NULL);

    list->head.next = &list->head;
    list->head.prev = &list->head;
}

bool list_empty(const struct list *list)
{
    if (list == NULL)
        return true;

    return list->head.next == &list->head;
}

void list_insert_before(struct list_node *pos,
                        struct list_node *node)
{
    ASSERT(pos != NULL);
    ASSERT(node != NULL);

    /*
     * A node must not already belong to a list.
     */
    ASSERT(node->prev == NULL);
    ASSERT(node->next == NULL);

    node->prev = pos->prev;
    node->next = pos;

    pos->prev->next = node;
    pos->prev = node;
}

void list_insert_after(struct list_node *pos,
                       struct list_node *node)
{
    ASSERT(pos != NULL);
    ASSERT(node != NULL);

    /*
     * A node must not already belong to a list.
     */
    ASSERT(node->prev == NULL);
    ASSERT(node->next == NULL);

    node->prev = pos;
    node->next = pos->next;

    pos->next->prev = node;
    pos->next = node;
}

void list_push_front(struct list *list,
                     struct list_node *node)
{
    ASSERT(list != NULL);
    ASSERT(node != NULL);

    list_insert_after(&list->head, node);
}

void list_push_back(struct list *list,
                    struct list_node *node)
{
    ASSERT(list != NULL);
    ASSERT(node != NULL);

    list_insert_before(&list->head, node);
}

void list_remove(struct list_node *node)
{
    ASSERT(node != NULL);

    /*
     * The node must belong to a list.
     */
    ASSERT(node->prev != NULL);
    ASSERT(node->next != NULL);

    /* Safely update the forward pointer of the previous node. */
    node->prev->next = node->next;

    /* Safely update the backward pointer of the next node. */
    node->next->prev = node->prev;

    /* Isolate the removed node. */
    node->prev = NULL;
    node->next = NULL;
}


