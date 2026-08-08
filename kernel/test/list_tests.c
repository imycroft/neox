#include "test.h"

#include "list.h"

#include "util.h"

/*
 * Maximum number of iterations while traversing a list.
 * Used to detect corrupted circular links.
 */
#define LIST_MAX_ITERATIONS 1024

struct test_node
{
    uint32_t value;

    struct list_node node;
};

/*
 * Count the number of nodes in a list.
 */
static uint32_t list_count(const struct list *list)
{
    const struct list_node *node;
    uint32_t count;

    if (list == NULL)
        return 0;

    count = 0;
    node = list->head.next;

    while (node != &list->head)
    {
        count++;

        if (count > LIST_MAX_ITERATIONS)
            return count;

        node = node->next;
    }

    return count;
}

/*
 * Validate the integrity of a list.
 */
static bool list_validate(const struct list *list)
{
    const struct list_node *node;
    uint32_t iterations;

    if (list == NULL)
        return false;

    if (list->head.next == NULL)
        return false;

    if (list->head.prev == NULL)
        return false;

    iterations = 0;
    node = &list->head;

    do
    {
        if (node->next == NULL)
            return false;

        if (node->prev == NULL)
            return false;

        if (node->next->prev != node)
            return false;

        if (node->prev->next != node)
            return false;

        node = node->next;

        iterations++;

        if (iterations > LIST_MAX_ITERATIONS)
            return false;

    } while (node != &list->head);

    return true;
}

/* Test functions */

/************************************************
 * Verify:
 *
 * - A newly initialized list is empty.
 * - The head node points to itself.
 *
 * This validates:
 *
 * - list_init()
 ************************************************/
static void test_list_init(void)
{
    struct list list;

    list_init(&list);

    TEST_ASSERT_EQ(list.head.next, &list.head);
    TEST_ASSERT_EQ(list.head.prev, &list.head);

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 0);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - list_empty() correctly detects an empty list.
 *
 * This validates:
 *
 * - list_empty()
 ************************************************/
static void test_list_empty(void)
{
    struct list list;

    list_init(&list);

    TEST_ASSERT_TRUE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 0);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Inserting a node before the list head inserts
 *   it at the end of the list.
 *
 * This validates:
 *
 * - list_insert_before()
 * - First insertion into an empty list.
 ************************************************/
static void test_list_insert_before_single(void)
{
    struct list list;
    struct test_node node = {0};

    list_init(&list);
    node.value = 1;


    list_insert_before(&list.head, &node.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 1);

    TEST_ASSERT_EQ(list.head.next, &node.node);
    TEST_ASSERT_EQ(list.head.prev, &node.node);

    TEST_ASSERT_EQ(node.node.next, &list.head);
    TEST_ASSERT_EQ(node.node.prev, &list.head);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Inserting two nodes before the list head
 *   appends them in order.
 *
 * This validates:
 *
 * - list_insert_before()
 * - Insertion into a non-empty list.
 ************************************************/
static void test_list_insert_before_two(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;


    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    TEST_ASSERT_EQ(list.head.next, &a.node);
    TEST_ASSERT_EQ(list.head.prev, &b.node);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = list.head.prev;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = node->prev;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Inserting a node after the list head inserts
 *   it at the front of the list.
 *
 * This validates:
 *
 * - list_insert_after()
 * - First insertion into an empty list.
 ************************************************/
static void test_list_insert_after_single(void)
{
    struct list list;
    struct test_node node = {0};

    list_init(&list);

    node.value = 1;

    list_insert_after(&list.head, &node.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 1);

    TEST_ASSERT_EQ(list.head.next, &node.node);
    TEST_ASSERT_EQ(list.head.prev, &node.node);

    TEST_ASSERT_EQ(node.node.next, &list.head);
    TEST_ASSERT_EQ(node.node.prev, &list.head);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Inserting two nodes after the list head
 *   prepends them in reverse insertion order.
 *
 * This validates:
 *
 * - list_insert_after()
 * - Insertion into a non-empty list.
 ************************************************/
static void test_list_insert_after_two(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;

    list_insert_after(&list.head, &a.node);
    list_insert_after(&list.head, &b.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    TEST_ASSERT_EQ(list.head.next, &b.node);
    TEST_ASSERT_EQ(list.head.prev, &a.node);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = list.head.prev;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = node->prev;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Inserting a node before an existing node
 *   correctly links it in the middle of the list.
 *
 * This validates:
 *
 * - list_insert_before()
 * - Middle insertion.
 ************************************************/
static void test_list_insert_before_middle(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &c.node);

    list_insert_before(&c.node, &b.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 3);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   3);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Inserting a node after an existing node
 *   correctly links it in the middle of the list.
 *
 * This validates:
 *
 * - list_insert_after()
 * - Middle insertion.
 ************************************************/
static void test_list_insert_after_middle(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &c.node);

    list_insert_after(&a.node, &b.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 3);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   3);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Removing the only node from a list restores
 *   the empty list state.
 *
 * This validates:
 *
 * - list_remove()
 * - Removing the last node.
 ************************************************/
static void test_list_remove_single(void)
{
    struct list list;
    struct test_node node = {0};

    list_init(&list);

    node.value = 1;

    list_insert_before(&list.head, &node.node);

    list_remove(&node.node);

    TEST_ASSERT_TRUE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 0);

    TEST_ASSERT_EQ(list.head.next, &list.head);
    TEST_ASSERT_EQ(list.head.prev, &list.head);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Removing the first node correctly promotes
 *   the second node to the front of the list.
 *
 * This validates:
 *
 * - list_remove()
 * - Removing the first node.
 ************************************************/
static void test_list_remove_front(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);

    list_remove(&a.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   3);

    TEST_ASSERT_EQ(list.head.prev, &c.node);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Removing the last node correctly promotes
 *   the previous node to the back of the list.
 *
 * This validates:
 *
 * - list_remove()
 * - Removing the last node.
 ************************************************/
static void test_list_remove_back(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);

    list_remove(&c.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    TEST_ASSERT_EQ(list.head.next, &a.node);
    TEST_ASSERT_EQ(list.head.prev, &b.node);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    TEST_ASSERT_NULL(c.node.prev);
    TEST_ASSERT_NULL(c.node.next);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Removing a node from the middle of the list
 *   correctly links its neighbors together.
 *
 * This validates:
 *
 * - list_remove()
 * - Middle node removal.
 ************************************************/
static void test_list_remove_middle(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);

    list_remove(&b.node);

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    TEST_ASSERT_EQ(list.head.next, &a.node);
    TEST_ASSERT_EQ(list.head.prev, &c.node);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   3);

    TEST_ASSERT_NULL(b.node.prev);
    TEST_ASSERT_NULL(b.node.next);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Removing all nodes one by one restores the
 *   list to its initial empty state.
 *
 * This validates:
 *
 * - list_remove()
 * - Consecutive removals.
 ************************************************/
static void test_list_remove_all(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);

    list_remove(&a.node);

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    list_remove(&b.node);

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 1);

    list_remove(&c.node);

    TEST_ASSERT_TRUE(list_empty(&list));
    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 0);

    TEST_ASSERT_EQ(list.head.next, &list.head);
    TEST_ASSERT_EQ(list.head.prev, &list.head);

    TEST_ASSERT_NULL(a.node.prev);
    TEST_ASSERT_NULL(a.node.next);

    TEST_ASSERT_NULL(b.node.prev);
    TEST_ASSERT_NULL(b.node.next);

    TEST_ASSERT_NULL(c.node.prev);
    TEST_ASSERT_NULL(c.node.next);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - A removed node can be inserted into a list
 *   again after being detached.
 *
 * This validates:
 *
 * - list_remove()
 * - Reusing detached nodes.
 ************************************************/
static void test_list_remove_reinsert(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);

    list_remove(&a.node);

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 1);

    list_insert_before(&list.head, &a.node);

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Removing every node from the front eventually
 *   restores an empty list.
 *
 * This validates:
 *
 * - list_remove()
 * - Repeated front removal.
 ************************************************/
static void test_list_remove_front_until_empty(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);

    while (!list_empty(&list))
    {
        list_remove(list.head.next);

        TEST_ASSERT_TRUE(list_validate(&list));
    }

    TEST_ASSERT_TRUE(list_empty(&list));
    TEST_ASSERT_EQ(list_count(&list), 0);

    TEST_ASSERT_EQ(list.head.next, &list.head);
    TEST_ASSERT_EQ(list.head.prev, &list.head);

    TEST_ASSERT_NULL(a.node.prev);
    TEST_ASSERT_NULL(a.node.next);

    TEST_ASSERT_NULL(b.node.prev);
    TEST_ASSERT_NULL(b.node.next);

    TEST_ASSERT_NULL(c.node.prev);
    TEST_ASSERT_NULL(c.node.next);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - A node removed from one list can be inserted
 *   into another list.
 *
 * This validates:
 *
 * - list_remove()
 * - Moving nodes between lists.
 ************************************************/
static void test_list_move_between_lists(void)
{
    struct list list1;
    struct list list2;

    struct test_node a = {0};

    struct list_node *node;

    list_init(&list1);
    list_init(&list2);

    a.value = 1;

    list_insert_before(&list1.head, &a.node);

    TEST_ASSERT_TRUE(list_validate(&list1));
    TEST_ASSERT_TRUE(list_validate(&list2));

    list_remove(&a.node);

    TEST_ASSERT_TRUE(list_empty(&list1));
    TEST_ASSERT_TRUE(list_validate(&list1));

    list_insert_before(&list2.head, &a.node);

    TEST_ASSERT_TRUE(list_validate(&list2));
    TEST_ASSERT_EQ(list_count(&list2), 1);

    node = list2.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    TEST_ASSERT_EQ(list2.head.prev, &a.node);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Nodes can be removed in arbitrary order
 *   without corrupting the list.
 *
 * This validates:
 *
 * - list_remove()
 * - Pointer integrity after multiple removals.
 ************************************************/
static void test_list_remove_random_order(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};
    struct test_node d = {0};
    struct test_node e = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;
    d.value = 4;
    e.value = 5;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);
    list_insert_before(&list.head, &d.node);
    list_insert_before(&list.head, &e.node);

    list_remove(&c.node);
    list_remove(&a.node);
    list_remove(&e.node);

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 2);

    node = list.head.next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = node->next;

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   4);

    TEST_ASSERT_EQ(list.head.prev, &d.node);

    TEST_ASSERT_NULL(a.node.prev);
    TEST_ASSERT_NULL(a.node.next);

    TEST_ASSERT_NULL(c.node.prev);
    TEST_ASSERT_NULL(c.node.next);

    TEST_ASSERT_NULL(e.node.prev);
    TEST_ASSERT_NULL(e.node.next);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - A list can contain a large number of nodes.
 *
 * This validates:
 *
 * - list_insert_before()
 * - Large list integrity.
 ************************************************/
static void test_list_large(void)
{
    struct list list;

    struct test_node nodes[100] = {0};

    struct list_node *node;

    uint32_t i;

    list_init(&list);

    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        nodes[i].value = i;

        list_insert_before(&list.head,
                           &nodes[i].node);
    }

    TEST_ASSERT_FALSE(list_empty(&list));

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list),
                   ARRAY_SIZE(nodes));

    node = list.head.next;

    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        TEST_ASSERT_EQ(container_of(node,
                                    struct test_node,
                                    node)->value,
                       i);

        node = node->next;
    }

    TEST_ASSERT_EQ(node, &list.head);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - A list can be traversed backwards.
 *
 * This validates:
 *
 * - list_prev()
 * - Backward traversal.
 ************************************************/
static void test_list_reverse_traversal(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);

    TEST_ASSERT_TRUE(list_validate(&list));

    node = list_back(&list);

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   3);

    node = list_prev(node);

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    node = list_prev(node);

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = list_prev(node);

    TEST_ASSERT_EQ(node, &list.head);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Forward traversal visits every node exactly
 *   once in insertion order.
 *
 * This validates:
 *
 * - list_front()
 * - list_next()
 * - Forward traversal.
 ************************************************/
static void test_list_forward_traversal(void)
{
    struct list list;

    struct test_node nodes[10] = {0};

    struct list_node *node;

    uint32_t i;

    list_init(&list);

    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        nodes[i].value = i;

        list_insert_before(&list.head,
                           &nodes[i].node);
    }

    TEST_ASSERT_TRUE(list_validate(&list));

    node = list_front(&list);

    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        TEST_ASSERT_EQ(container_of(node,
                                    struct test_node,
                                    node)->value,
                       i);

        node = list_next(node);
    }

    TEST_ASSERT_EQ(node, &list.head);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Backward traversal visits every node exactly
 *   once in reverse insertion order.
 *
 * This validates:
 *
 * - list_back()
 * - list_prev()
 * - Backward traversal.
 ************************************************/
static void test_list_backward_traversal(void)
{
    struct list list;

    struct test_node nodes[10] = {0};

    struct list_node *node;

    int32_t i;

    list_init(&list);

    for (i = 0; i < (int32_t)ARRAY_SIZE(nodes); i++)
    {
        nodes[i].value = i;

        list_insert_before(&list.head,
                           &nodes[i].node);
    }

    TEST_ASSERT_TRUE(list_validate(&list));

    node = list_back(&list);

    for (i = ARRAY_SIZE(nodes) - 1; i >= 0; i--)
    {
        TEST_ASSERT_EQ(container_of(node,
                                    struct test_node,
                                    node)->value,
                       (uint32_t)i);

        node = list_prev(node);
    }

    TEST_ASSERT_EQ(node, &list.head);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Removing every even-indexed node leaves
 *   the remaining nodes correctly linked.
 *
 * This validates:
 *
 * - list_remove()
 * - List integrity after sparse removals.
 ************************************************/
static void test_list_remove_alternate(void)
{
    struct list list;

    struct test_node nodes[10] = {0};

    struct list_node *node;

    uint32_t i;
    uint32_t expected;

    list_init(&list);

    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        nodes[i].value = i;

        list_insert_before(&list.head,
                           &nodes[i].node);
    }

    for (i = 0; i < ARRAY_SIZE(nodes); i += 2)
    {
        list_remove(&nodes[i].node);
    }

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 5);

    node = list_front(&list);

    for (expected = 1; expected < ARRAY_SIZE(nodes); expected += 2)
    {
        TEST_ASSERT_EQ(container_of(node,
                                    struct test_node,
                                    node)->value,
                       expected);

        node = list_next(node);
    }

    TEST_ASSERT_EQ(node, &list.head);

    for (i = 0; i < ARRAY_SIZE(nodes); i += 2)
    {
        TEST_ASSERT_NULL(nodes[i].node.prev);
        TEST_ASSERT_NULL(nodes[i].node.next);
    }

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Nodes can be removed and reinserted in a
 *   different order.
 *
 * This validates:
 *
 * - list_remove()
 * - list_insert_before()
 * - List integrity after reordering.
 ************************************************/
static void test_list_reorder(void)
{
    struct list list;

    struct test_node a = {0};
    struct test_node b = {0};
    struct test_node c = {0};

    struct list_node *node;

    list_init(&list);

    a.value = 1;
    b.value = 2;
    c.value = 3;

    list_insert_before(&list.head, &a.node);
    list_insert_before(&list.head, &b.node);
    list_insert_before(&list.head, &c.node);

    list_remove(&b.node);

    list_insert_before(&list.head, &b.node);

    TEST_ASSERT_TRUE(list_validate(&list));
    TEST_ASSERT_EQ(list_count(&list), 3);

    node = list_front(&list);

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   1);

    node = list_next(node);

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   3);

    node = list_next(node);

    TEST_ASSERT_EQ(container_of(node,
                                struct test_node,
                                node)->value,
                   2);

    TEST_ASSERT_EQ(list_back(&list), &b.node);

    test_pass();
}

/************************************************
 * Verify:
 *
 * - Accessing the front and back of an empty list
 *   returns the list head.
 *
 * This validates:
 *
 * - list_front()
 * - list_back()
 * - Empty list access.
 ************************************************/
static void test_list_empty_access(void)
{
    struct list list;

    list_init(&list);

    TEST_ASSERT_TRUE(list_empty(&list));

    TEST_ASSERT_EQ(list_front(&list), &list.head);
    TEST_ASSERT_EQ(list_back(&list), &list.head);

    TEST_ASSERT_TRUE(list_validate(&list));

    test_pass();
}

/************************************************
 * Verify:
 *
 * - A node cannot be inserted into a list twice.
 *
 * This validates:
 *
 * - Duplicate insertion detection.
 * TODO : implement an ASSERT_EXPECT() to be able to run this test
 ************************************************/
static void test_list_duplicate_insert(void)
{
    return;
}

// End of Test

static test_entry_t tests[] =
{
    { "List Init",  test_list_init  },
    { "List Empty", test_list_empty },
    { "Insert Before (Single)", test_list_insert_before_single },
    { "Insert Before (Two)", test_list_insert_before_two },
    { "Insert After (Single)", test_list_insert_after_single },
    { "Insert After (Two)", test_list_insert_after_two },
    { "Insert Before (Middle)", test_list_insert_before_middle },
    { "Insert After (Middle)", test_list_insert_after_middle },
    { "Remove (Single)", test_list_remove_single },
    { "Remove (Front)", test_list_remove_front },
    { "Remove (Back)", test_list_remove_back },
    { "Remove (Middle)", test_list_remove_middle },
    { "Remove (All)", test_list_remove_all },
    { "Remove (Reinsert)", test_list_remove_reinsert },
    { "Remove (Front Until Empty)", test_list_remove_front_until_empty },
    { "Move Between Lists", test_list_move_between_lists },
    { "Remove (Random Order)", test_list_remove_random_order },
    { "Large List", test_list_large },
    { "Reverse Traversal", test_list_reverse_traversal },
    { "Forward Traversal", test_list_forward_traversal },
    { "Backward Traversal", test_list_backward_traversal },
    { "Remove (Alternate)", test_list_remove_alternate },
    { "Reorder", test_list_reorder },
    { "Empty Access", test_list_empty_access },
    { "Duplicate Insert", test_list_duplicate_insert },
};

void test_list(void)
{
    uint32_t i;

    test_begin("List");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}


