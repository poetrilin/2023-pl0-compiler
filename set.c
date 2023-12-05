#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "set.h"

/**
 * Function to unite two sets.貌似没有去重?
 * @param s1 The first set.
 * @param s2 The second set.
 * @return The union of the two sets.
 */
symset uniteset(symset s1, symset s2)
{
    symset s;
    snode* p;

    // Move past the dummy node in each set
    s1 = s1->next;
    s2 = s2->next;

    // Create a new set and a pointer to the last node
    s = p = (snode*) malloc(sizeof(snode));

    // Merge the two sets in ascending order
    while (s1 && s2)
    {
        p->next = (snode*) malloc(sizeof(snode));
        p = p->next;
        if (s1->elem < s2->elem)
        {
            p->elem = s1->elem;
            s1 = s1->next;
        }
        else
        {
            p->elem = s2->elem;
            s2 = s2->next;
        }
    }

    // Copy remaining elements from set s1
    while (s1)
    {
        p->next = (snode*) malloc(sizeof(snode));
        p = p->next;
        p->elem = s1->elem;
        s1 = s1->next;
    }

    // Copy remaining elements from set s2
    while (s2)
    {
        p->next = (snode*) malloc(sizeof(snode));
        p = p->next;
        p->elem = s2->elem;
        s2 = s2->next;
    }

    // Set the last node's next pointer to NULL
    p->next = NULL;

    return s;
} // uniteset

/**
 * Function to insert an element into a set.
 * @param s The set.
 * @param elem The element to insert.
 */
void setinsert(symset s, int elem)
{
    snode* p = s;
    snode* q;

    // Find the correct position to insert the element in ascending order
    while (p->next && p->next->elem < elem)
    {
        p = p->next;
    }

    // Insert the new element
    q = (snode*) malloc(sizeof(snode));
    q->elem = elem;
    q->next = p->next;
    p->next = q;
} // setinsert

/**
 * Function to create a set with the given elements.
 * @param elem The first element.
 * @param ... The variable number of elements, terminated with SYM_NULL.
 * @return The created set.
 */
symset createset(int elem, .../* SYM_NULL */)
{
    va_list list;
    symset s;

    // Create a new set and initialize it
    s = (snode*) malloc(sizeof(snode));
    s->next = NULL;

    // Use variable arguments to insert elements into the set
    va_start(list, elem);
    while (elem)
    {
        setinsert(s, elem);
        elem = va_arg(list, int);
    }
    va_end(list);

    return s;
} // createset

/**
 * Function to destroy a set and free its memory.
 * @param s The set to destroy.
 */
void destroyset(symset s)
{
    snode* p;

    // Free each node in the set
    while (s)
    {
        p = s;
        p->elem = -1000000; 
        s = s->next;
        free(p);
    }
} // destroyset

/**
 * Function to check if an element is in the set.
 * @param elem The element to check.
 * @param s The set.
 * @return 1 if the element is in the set, 0 otherwise.
 */
int inset(int elem, symset s)
{
    // Move past the dummy node in the set
    s = s->next;

    // Find the element in the set
    while (s && s->elem < elem)
        s = s->next;

    // Check if the element is found
    if (s && s->elem == elem)
        return 1;
    else
        return 0;
} // inset

// EOF set.c
