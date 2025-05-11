#pragma once

/*
 * Usage examples:
 *
 * list_node_t *pos, *tmp;
 * list_for_each_safe(pos, tmp, &cpu->run_queue) {
 *     proc_t *p = list_entry(pos, proc_t, q_node);
 *     ...
 * }
 *
 * // Add to head or tail
 * list_add(&p->q_node, &cpu->run_queue);
 * // or
 * list_add_tail(&p->q_node, &cpu->run_queue);
 *
 * // Remove from queue
 * list_del(&p->q_node);
 *
 */

typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
} list_node_t;

#define LIST_INIT(name) { &(name), &(name) }

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr);         \
    (ptr)->prev = (ptr);         \
} while (0)

// Add node after head
static inline void
list_add(list_node_t *new, list_node_t *head)
{
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

// Add node after tail
static inline void
list_add_tail(list_node_t *new, list_node_t *head)
{
    new->next = head;
    new->prev = head->prev;
    head->prev->next = new;
    head->prev = new;
}

static inline void
list_del(list_node_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = entry->prev = NULL;
}

static inline int
list_empty(const list_node_t *head)
{
    return head->next == head;
}

static inline int
list_in_queue(list_node_t *node)
{
    return node->next && node->prev;
}


// Iterate over list
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

// Iterate over list safely during deletion
#define list_for_each_safe(pos, tmp, head) \
    for (pos = (head)->next, tmp = pos->next; pos != (head); \
         pos = tmp, tmp = pos->next)

// Get the containing struct from node
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)NULL)->member)))
