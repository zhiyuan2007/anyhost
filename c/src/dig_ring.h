#ifndef  DIG_RING_H
#define  DIG_RING_H

/*
 * for offsetof()
 */
#include <stddef.h>

/*
 * A ring is a kind of doubly-linked list that can be manipulated
 * without knowing where its head is.
 */

/*
 * A struct on a ring contains a field linking it to the other
 * elements in the ring, e.g.
 *
 *      struct my_item_t {
 *           RING_ENTRY(my_item_t) link;
 *          int foo;
 *          char *bar;
 *      };
 *
 * A struct may be put on more than one ring if it has more than one
 *  RING_ENTRY field.
 */
#define  RING_ENTRY(elem)						\
    struct {								\
	struct elem *next;						\
	struct elem *prev;						\
    }

/*
 * Each ring is managed via its head, which is a struct declared like this:
 *
 *      RING_HEAD(my_ring_t, my_item_t);
 *      struct my_ring_t ring, *ringp;
 *
 * This struct looks just like the element link struct so that we can
 * be sure that the typecasting games will work as expected.
 *
 * The first element in the ring is next after the head, and the last
 * element is just before the head.
 */
#define  RING_HEAD(head, elem)					\
    struct head {							\
	struct elem *next;						\
	struct elem *prev;						\
    }

/*
 * The head itself isn't an element, but in order to get rid of all
 * the special cases when dealing with the ends of the ring, we play
 * typecasting games to make it look like one. The sentinel is the
 * magic pointer value that occurs before the first and after the last
 * elements in the ring, computed from the address of the ring's head.
 *
 * Note that for strict C standards compliance you should put the
 *  RING_ENTRY first in struct elem unless the head is always part
 * of a larger object with enough earlier fields to accommodate the
 * offsetof() computed below. You can usually ignore this caveat.
 */
#define  RING_SENTINEL(hp, elem, link)				\
    (struct elem *)((char *)(hp) - offsetof(struct elem, link))

/*
 * Accessor macros. Use these rather than footling inside the
 * structures directly so that you can more easily change to a
 * different flavour of list from BSD's <sys/queue.h>.
 */
#define  RING_FIRST(hp)	(hp)->next
#define  RING_LAST(hp)	(hp)->prev
#define  RING_NEXT(ep, link)	(ep)->link.next
#define  RING_PREV(ep, link)	(ep)->link.prev

/*
 * Empty rings and singleton elements.
 */
#define  RING_INIT(hp, elem, link) do {				\
	 RING_FIRST((hp)) =  RING_SENTINEL((hp), elem, link);	\
	 RING_LAST((hp))  =  RING_SENTINEL((hp), elem, link);	\
    } while (0)

#define  RING_EMPTY(hp, elem, link)					\
    ( RING_FIRST((hp)) ==  RING_SENTINEL((hp), elem, link))

#define  RING_ELEM_INIT(ep, link) do {				\
	 RING_NEXT((ep), link) = (ep);				\
	 RING_PREV((ep), link) = (ep);				\
    } while (0)

/*
 * Adding elements.
 */
#define  RING_SPLICE_BEFORE(lep, ep1, epN, link) do {			\
	 RING_NEXT((epN), link) = (lep);				\
	 RING_PREV((ep1), link) =  RING_PREV((lep), link);		\
	 RING_NEXT( RING_PREV((lep), link), link) = (ep1);		\
	 RING_PREV((lep), link) = (epN);				\
    } while (0)

#define  RING_SPLICE_AFTER(lep, ep1, epN, link) do {			\
	 RING_PREV((ep1), link) = (lep);				\
	 RING_NEXT((epN), link) =  RING_NEXT((lep), link);		\
	 RING_PREV( RING_NEXT((lep), link), link) = (epN);		\
	 RING_NEXT((lep), link) = (ep1);				\
    } while (0)

#define  RING_INSERT_BEFORE(lep, nep, link)				\
	 RING_SPLICE_BEFORE((lep), (nep), (nep), link)

#define  RING_INSERT_AFTER(lep, nep, link)				\
	 RING_SPLICE_AFTER((lep), (nep), (nep), link)

/*
 * These macros work when the ring is empty: inserting before the head
 * or after the tail of an empty ring using the macros above doesn't work.
 */
#define  RING_SPLICE_HEAD(hp, ep1, epN, elem, link)			\
	 RING_SPLICE_AFTER( RING_SENTINEL((hp), elem, link),	\
			     (ep1), (epN), link)

#define  RING_SPLICE_TAIL(hp, ep1, epN, elem, link)			\
	 RING_SPLICE_BEFORE( RING_SENTINEL((hp), elem, link),	\
			     (ep1), (epN), link)

#define  RING_INSERT_HEAD(hp, nep, elem, link)			\
	 RING_SPLICE_HEAD((hp), (nep), (nep), elem, link)

#define  RING_INSERT_TAIL(hp, nep, elem, link)			\
	 RING_SPLICE_TAIL((hp), (nep), (nep), elem, link)

/*
 * Concatenating ring h2 onto the end of ring h1 leaves h2 empty.
 */
#define  RING_CONCAT(h1, h2, elem, link) do {				\
	if (! RING_EMPTY((h2), elem, link)) {				\
	     RING_SPLICE_BEFORE( RING_SENTINEL((h1), elem, link),	\
				   RING_FIRST((h2)),			\
				   RING_LAST((h2)), link);		\
	     RING_INIT((h2), elem, link);				\
	}								\
    } while (0)

/*
 * Removing elements. Be warned that the unspliced elements are left
 * with dangling pointers at either end!
 */
#define  RING_UNSPLICE(ep1, epN, link) do {				\
	 RING_NEXT( RING_PREV((ep1), link), link) =			\
		      RING_NEXT((epN), link);				\
	 RING_PREV( RING_NEXT((epN), link), link) =			\
		      RING_PREV((ep1), link);				\
    } while (0)

#define  RING_REMOVE(ep, link)					\
     RING_UNSPLICE((ep), (ep), link)

/*
 * Iteration.
 */
#define  RING_FOREACH(ep, hp, elem, link)				\
    for ((ep)  =  RING_FIRST((hp));					\
	 (ep) !=  RING_SENTINEL((hp), elem, link);			\
	 (ep)  =  RING_NEXT((ep), link))

#define  RING_FOREACH_REVERSE(ep, hp, elem, link)			\
    for ((ep)  =  RING_LAST((hp));					\
	 (ep) !=  RING_SENTINEL((hp), elem, link);			\
	 (ep)  =  RING_PREV((ep), link))

#ifdef  RING_DEBUG
#include <stdio.h>
#define  RING_CHECK_ONE(msg, ptr)					\
	fprintf(stderr, "*** %s %p\n", msg, ptr)
#define  RING_CHECK(hp, elem, link, msg)				\
	 RING_CHECK_ELEM( RING_SENTINEL(hp, elem, link), elem, link, msg)
#define  RING_CHECK_ELEM(ep, elem, link, msg) do {			\
	struct elem *start = (ep);					\
	struct elem *this = start;					\
	fprintf(stderr, "*** ring check start -- %s\n", msg);		\
	do {								\
	    fprintf(stderr, "\telem %p\n", this);			\
	    fprintf(stderr, "\telem->next %p\n",			\
		     RING_NEXT(this, link));				\
	    fprintf(stderr, "\telem->prev %p\n",			\
		     RING_PREV(this, link));				\
	    fprintf(stderr, "\telem->next->prev %p\n",			\
		     RING_PREV( RING_NEXT(this, link), link));	\
	    fprintf(stderr, "\telem->prev->next %p\n",			\
		     RING_NEXT( RING_PREV(this, link), link));	\
	    if ( RING_PREV( RING_NEXT(this, link), link) != this) {	\
		fprintf(stderr, "\t*** this->next->prev != this\n");	\
		break;							\
	    }								\
	    if ( RING_NEXT( RING_PREV(this, link), link) != this) {	\
		fprintf(stderr, "\t*** this->prev->next != this\n");	\
		break;							\
	    }								\
	    this =  RING_NEXT(this, link);				\
	} while (this != start);					\
	fprintf(stderr, "*** ring check end\n");			\
    } while (0)
#else
#define  RING_CHECK_ONE(msg, ptr)
#define  RING_CHECK(hp, elem, link, msg)
#define  RING_CHECK_ELEM(ep, elem, link, msg)
#endif

#endif /* ! RING_H */
