#ifndef _ATOMIC_H
#define _ATOMIC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Atomic type.
 */

typedef struct {
    volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i)  { (i) }

/**
 * Read atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v) ((v)->counter)

/**
 * Set atomic variable
 * @param v pointer of type atomic_t
 * @param i required value
 */
#define atomic_set(v,i) (((v)->counter) = (i))

/**
 * Add to the atomic variable
 * @param i integer value to add
 * @param v pointer of type atomic_t
 * Return previous value of v
 */
static inline int atomic_add( int i, atomic_t *v )
{
    return __sync_fetch_and_add(&v->counter, i);
}

/**
 * Subtract the atomic variable
 * @param i integer value to subtract
 * @param v pointer of type atomic_t
 * return previous value of v
 *
 * Atomically subtracts @i from @v.
 */
static inline int atomic_sub( int i, atomic_t *v )
{
    return __sync_fetch_and_sub(&v->counter, i);
}

/**
 * Subtract value from variable and test result
 * @param i integer value to subtract
 * @param v pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline bool atomic_sub_and_test( int i, atomic_t *v )
{
    return 0 == (__sync_sub_and_fetch(&v->counter, i));
}

/**
 * Increment atomic variable
 * @param v pointer of type atomic_t
 * return previous value of v
 *
 * Atomically increments @v by 1.
 */
static inline int atomic_inc( atomic_t *v )
{
    return __sync_fetch_and_add(&v->counter, 1);
}

/**
 * @brief decrement atomic variable
 * @param v: pointer of type atomic_t
 * return previous value of v
 *
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline int atomic_dec( atomic_t *v )
{
    return __sync_fetch_and_sub(&v->counter, 1);
}

/**
 * @brief Decrement and test
 * @param v pointer of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline bool atomic_dec_and_test( atomic_t *v )
{
    return 0 == (__sync_sub_and_fetch(&v->counter, 1));
}

/**
 * @brief Increment and test
 * @param v pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline bool atomic_inc_and_test( atomic_t *v )
{
    return 0 == (__sync_add_and_fetch(&v->counter, 1));
}

/**
 * @brief add and test if negative
 * @param v pointer of type atomic_t
 * @param i integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline int atomic_add_negative( int i, atomic_t *v )
{
    return (__sync_add_and_fetch(&v->counter, i)) < 0;
}

/**
 * @brief compare and swap
 * @param v pointer of type atomic_t
 *
 * If the current value of @b v is @b oldval,
 * then write @b newval into @b v.
 * Return the previous value of v
 */
static inline int atomic_cas( atomic_t *v, int oldval, int newval )
{
    //return __sync_bool_compare_and_swap(&v->counter, oldval, newval);
    return __sync_val_compare_and_swap(&v->counter, oldval, newval);
}

#endif
