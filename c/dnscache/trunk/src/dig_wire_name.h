#ifndef _H_DIG_WIRENAME_H_
#define _H_DIG_WIRENAME_H_

#include "dig_buffer.h"

#define MAX_WIRE_NAME_LEN   255
#define MAX_LABEL_COUNT     127
#define MAX_LABEL_LEN       63


typedef struct wire_name wire_name_t;

/*
 * create wire name from string format
 * return: null if failed(label too long, length too long, label too many)
 */
wire_name_t *
wire_name_from_text(const char *src_str);

/*
 * create wire name from compressed or uncompressed raw data stored in buf
 * return: null if failed(decompressing error, label too long, length too long, label too many) and buf position
 *          will point to the orignal, otherwise buf position will skip several bytes(name length maximum)
 */
wire_name_t *
wire_name_from_wire(buffer_t *buf);

/*
 * clone a wire name
 * return: return a wire name as same as src without memory allocation(increase reference count)
 */
wire_name_t *
wire_name_clone(const wire_name_t *src);

/*
 * deep copy a wire name
 * return: a wire name equal to src with memory allocation 
 */
wire_name_t *
wire_name_copy(const wire_name_t *src);

/*
 * decrease name reference count and if ref count is zero, delete the space of name
 * return: NULL
 */
void
wire_name_delete(wire_name_t *name);

/*
 * get name length(no last zero)
 * return: name length(uint8_t)
 */
uint8_t
wire_name_get_len(const wire_name_t *src);

/*
 * get name label count
 * return: name label count(uint8_t)
 */
uint8_t
wire_name_get_label_count(const wire_name_t *src);

/*
 * get name related pointer
 * return: an obscure pointer
 */
const void *
wire_name_get_ptr(const wire_name_t *src);

/*
 * set name related pointer
 * return: NULL
 */
void
wire_name_set_prt(wire_name_t *src, const void *ptr);

/*
 * cut long name by a short name
 * return: if long name does not contain short name, return NULL;
 *         if short name is equal to long name, return NULL;
 *         if short name is root, return clone name of long name;
 *         else return the prefix name.
 */
wire_name_t *
wire_name_cut(const wire_name_t *long_name, const wire_name_t *short_name);

/*
 * concatenate two names together
 * return: if the name after concatenation is too long or too many label count, return NULL;
 *         else return the concatenation
 */
wire_name_t *
wire_name_concat(const wire_name_t *prefix_name, const wire_name_t *suffix_name);

/*
 * generate a new name which is the closest parent of child name
 * return: if the label count of child name is zero(root), return NULL;
 *         if the label count of child name is one, return root name;
 *         else return a malloced closest parent name of child name
 */
wire_name_t *
wire_name_parent(const wire_name_t *child_name);

/*
 * for performance improvement
 * return the parent name of child name(no memory allocated, return child name pointer but modify the internal pointer 
 *      to make it look like parent name)
 * return: if child name is root, return NULL;
 *         if the label count of child name is one, delete the child name and return root name
 *         else return the parent name of child name
 */
wire_name_t *
wire_name_to_parent(wire_name_t *child_name);

/*
 * split src name into two parts
 * return: NULL
 * parameter: src: the orignal name which will be splited
 *            begin_label_count: should less than or equal to label count of name and more than or equal to zero
 *                               if is equal to label count of src name, prefix is null and suffix is clone name of src
 *                               if is zero, prefix is clone name of src and suffix is null
 *            prefix_part_name: if is not null, be set the prefix name before begin label count of src
 *            suffix_part_name: if is not null, be set the suffix name after begin label count of src
 */
void
wire_name_split(const wire_name_t *src, uint8_t begin_label_count, 
                wire_name_t **prefix_part_name,
                wire_name_t **suffix_part_name);

/*
 * compare functions
 */
typedef enum
{
    SUBNAME = 0,
    SUPERNAME,
    EQUALNAME,
    COMMONANCESTOR
} name_relation_t;

/*
 * compare two wire name
 * return: if name1 less than name2, return -1;
 *         if name1 equal to name2, return 0;
 *         if name1 more than name2, return 1
 * parameters: name1
 *             name2
 *             relation: if is not null, be set the relation of name1 and name2
 *             common_label_count: if is not null, be set the common label count of name1 and name2
 */
int
wire_name_full_cmp(const wire_name_t *name1, 
                    const wire_name_t *name2,
                    name_relation_t *relation,
                    uint8_t *common_label_count);

/*
 * compare two wire name
 * return: if name1 > name2, return 1
 *         if name1 = name2, return 0
 *         if name1 < name2, return -1
 */
int
wire_name_compare(const wire_name_t *name1, const wire_name_t *name2);

/*
 * only compare the label in certain index of two wire name(index order from right to left)
 * return: if label of name1 > the one of name2, return 1
 *         if label of name1 = the one of name2, return 0
 *         if label of name1 < the one of name2, return -1
 */
int
wire_name_label_compare(const wire_name_t *name1, const wire_name_t *name2, int label_index);

/*
 * return: if name1 = name2, return true
 *         else return false
 */
bool
wire_name_is_equal(const wire_name_t *name1, const wire_name_t *name2);

/*
 * return: if long name contains short name, return true;
 *         else return false
 */
bool
wire_name_contains(const wire_name_t *long_name, const wire_name_t *short_name);

/*
 * return: if name is root, return true;
 *         else return false
 */
bool
wire_name_is_root(const wire_name_t *name);

/*
 * retrun: if name is wildcard, return true;
 *         else return false
 */
bool
wire_name_is_wildcard(const wire_name_t *name);

/*
 * dump wire name raw data to uncompressed buf
 * return: if succeed, return 0;
 *         else(buffer length is not enough), return -1;
 */
uint32_t
wire_name_to_wire(const wire_name_t *name, buffer_t *buf);

/*
 * dump wire name to string format
 * return: if succeed, return 0;
 *         else(des buf length is not enough), return -1
 */
uint32_t
wire_name_to_text(const wire_name_t *name, buffer_t *buf);

/*
 * dump wire name to compressed format
 * return: if succedd, return SUCCESS;
 *         else(render buffer is not enough), return FAILED
 */
void
wire_name_to_string(const wire_name_t *name, char *domain_name);
/*
 * get the hash value of name
 * return: the hash value
 */
uint32_t
wire_name_hash(const wire_name_t *name);
#endif
