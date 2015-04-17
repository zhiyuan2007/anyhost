/*
 * buffer.h -- generic memory buffer.
 *
 * The buffer module implements a generic buffer.  The API is based on
 * the java.nio.Buffer interface.
 */

#ifndef _H_DIG_BUFFER_H_
#define _H_DIG_BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "zip_utils.h"

typedef struct buffer
{
	// The current position used for reading/writing
	size_t   _position;
    // The read limit
	size_t   _limit;
    // The amount of data the buffer can write limit
    size_t   _reservered_capacity;
    // The amount of data the buffer actually contains
	size_t   _actual_capacity;
    // The data contained in the buffer
	uint8_t *_data;
	// If the buffer is fixed it cannot be resized
	unsigned _fixed : 1;
} buffer_t;

/*
 * the buffer is dynamic increase, and init size equal to init_size
 */
void
buffer_create(buffer_t *buffer, size_t init_size);
/* 
 * Create a buffer with the specified data.  The data is not copied
 * and no memory allocations are done.  The buffer is fixed and cannot
 * be resized using buffer_reserve(). 
 */
void 
buffer_create_from(buffer_t *buffer, void *data, size_t size);

/* 
 * Clear the buffer and make it ready for writing.  The buffer's limit
 * is set to the capacity and the position is set to 0. 
 */
void 
buffer_clear(buffer_t *buffer);

/* 
 * Make the buffer ready for reading the data that has been written to
 * the buffer.  The buffer's limit is set to the current position and
 * the position is set to 0. 
 * no use
 */
void 
buffer_flip(buffer_t *buffer);

/*
 * free buffer data
 * Requirement: buffer is not fixed
 */
static inline void
buffer_cleanup(buffer_t *buffer)
{
    //ASSERT(0 == buffer->_fixed, "fixed buffer can not cleanup\n");
	free(buffer->_data);
}

/* 
 * Make the buffer ready for re-reading the data.  The buffer's
 * position is reset to 0
 */
static inline void 
buffer_rewind(buffer_t *buffer)
{
    buffer->_position = 0;
}

/*
 * get buffer data pointer
 */
static inline uint8_t *
buffer_data(buffer_t *buffer)
{
    return buffer->_data;
}

/*
 * get buffer position
 */
static inline size_t
buffer_position(buffer_t *buffer)
{
	return buffer->_position;
}

/*
 * Set the buffer's position to MARK.  The position must be less than
 * or equal to the buffer's limit
 */
static inline bool
buffer_set_position(buffer_t *buffer, size_t mark)
{
	if (mark > buffer->_reservered_capacity)
        return false;

	buffer->_position = mark;
    return true;
}

/* 
 * Change the buffer's position by COUNT bytes.  The position must not
 * be moved behind the buffer's limit or before the beginning of the
 * buffer
 */
static inline bool
buffer_skip(buffer_t *buffer, ssize_t count)
{
    if (buffer->_position + count > buffer->_reservered_capacity)
        return false;
	buffer->_position += count;
    if (buffer->_limit < buffer->_position)
        buffer->_limit = buffer->_position;

    return true;
}

static inline bool
buffer_rollback(buffer_t *buffer, ssize_t count)
{
    if (buffer->_position - count < 0)
        return false;
    buffer->_position -= count;
    return true;
}

/*
 * get buffer read limit
 */
static inline size_t
buffer_limit(buffer_t *buffer)
{
	return buffer->_limit;
}

/*
 * Change the buffer's limit.  If the buffer's position is greater
 * than the new limit the position is set to the limit
 */
static inline bool
buffer_set_limit(buffer_t *buffer, size_t limit)
{
	if (limit > buffer->_reservered_capacity)
        return false;

	buffer->_limit = limit;
	if (buffer->_position > buffer->_limit)
		buffer->_position = buffer->_limit;

    return true;
}


static inline size_t
buffer_reservered_capacity(buffer_t *buffer)
{
	return buffer->_reservered_capacity;
}

static inline size_t
buffer_actual_capacity(buffer_t *buffer)
{
    return buffer->_actual_capacity;
}

static inline void
buffer_set_reservered_size(buffer_t *buffer, size_t reservered_size)
{
    //ASSERT(reservered_size <= buffer_actual_capacity(buffer), "can not set reservered size more than actual size\n");
    buffer->_reservered_capacity = reservered_size;
}

/*
 * make actual capacity equal to reserved capacity
 */
static inline void
buffer_capacity_alignment(buffer_t *buffer)
{
    buffer->_reservered_capacity = buffer->_actual_capacity;
}

/* 
 * Change the buffer's capacity.  The data is reallocated so any 
 * pointers to the data may become invalid.  The buffer's limit is set
 * to the buffer's new capacity
 */
bool
buffer_set_capacity(buffer_t *buffer, size_t capacity);

static inline bool
buffer_capacity_is_enough(buffer_t *buffer, int insert_size)
{
    return ((buffer->_position + insert_size) <= buffer->_reservered_capacity);
}

/*
 * Ensure BUFFER can contain at least AMOUNT more bytes.  The buffer's 
 * capacity is increased if necessary using buffer_set_capacity(). 
 * The buffer's limit is always set to the (possibly increased)
 * capacity
 */
void 
buffer_reserve(buffer_t *buffer, size_t amount);

/*
 * Return a pointer to the data at the indicated position
 */
static inline uint8_t *
buffer_at(const buffer_t *buffer, size_t at)
{
	if (at > buffer->_limit)
        return NULL;

	return buffer->_data + at;
}

/* 
 * Return a pointer to the beginning of the buffer (the data at
 * position 0)
 */
static inline uint8_t *
buffer_begin(buffer_t *buffer)
{
	return buffer_at(buffer, 0);
}

/* Return a pointer to the end of the buffer (the data at the buffer's
   limit). */
static inline uint8_t *
buffer_end(buffer_t *buffer)
{
	return buffer_at(buffer, buffer->_limit);
}

/* Return a pointer to the data at the buffer's current position. */
static inline uint8_t *
buffer_current(const buffer_t *buffer)
{
	return buffer_at(buffer, buffer->_position);
}

/* The number of bytes remaining between the indicated position and
   the limit. */
static inline size_t
buffer_remaining_at(const buffer_t *buffer, size_t at, size_t count)
{
	if (at + count > buffer->_limit)
        return -1;

	return buffer->_limit - at;
}

/* The number of bytes remaining between the buffer's position and
   limit. */
static inline size_t
buffer_remaining(const buffer_t *buffer)
{
	return buffer_remaining_at(buffer, buffer->_position, 0);
}

/* 
 * Check if the buffer has at least COUNT more bytes available.
 * Before reading or writing the caller needs to ensure enough space
 * is available! 
 */
static inline bool
buffer_available_at(buffer_t *buffer, size_t at, size_t count)
{
    if (at + count > buffer->_reservered_capacity)
    {
        if (buffer->_fixed) return false;
        else return buffer_set_capacity(buffer, ((at + count) * 2));
    }
	return true;
}

static inline bool
buffer_available(buffer_t *buffer, size_t count)
{
	return buffer_available_at(buffer, buffer->_position, count);
}

static inline bool
buffer_available2write(buffer_t *buffer)
{
    return (buffer->_reservered_capacity - buffer->_position) > 0;
}

static inline uint32_t
buffer_write_at(buffer_t *buffer, size_t at, const void *data, size_t count)
{
	if (!buffer_available_at(buffer, at, count))
        return 0;

	memcpy(buffer->_data + at, data, count);
    if (buffer->_limit < (at + count))
        buffer->_limit = (at + count);
        
    return (uint32_t)count;
}

static inline uint32_t
buffer_write(buffer_t *buffer, const void *data, size_t count)
{
	uint32_t write_len = buffer_write_at(buffer, buffer->_position, data, count);

	buffer->_position += write_len;
    return write_len;
}

static inline uint32_t
buffer_write_string_at(buffer_t *buffer, size_t at, const char *str)
{
	return buffer_write_at(buffer, at, str, strlen(str));
}

static inline uint32_t
buffer_write_string(buffer_t *buffer, const char *str)
{
	return buffer_write(buffer, str, strlen(str));
}

static inline uint32_t
buffer_write_u8_at(buffer_t *buffer, size_t at, uint8_t data)
{
	if (!buffer_available_at(buffer, at, sizeof(data)))
        return 0;

	buffer->_data[at] = data;
    return (uint32_t)sizeof(uint8_t);
}

static inline uint32_t
buffer_write_u8(buffer_t *buffer, uint8_t data)
{
	uint32_t write_len = buffer_write_u8_at(buffer, buffer->_position, data);
	buffer->_position += write_len;
    if (buffer->_limit < buffer->_position)
        buffer->_limit = buffer->_position;
    return write_len;
}

static inline uint32_t
buffer_write_u16_at(buffer_t *buffer, size_t at, uint16_t data)
{
    if (!buffer_available_at(buffer, at, sizeof(data)))
        return 0;

    *(uint16_t *)(buffer->_data + at) = data;
    return (uint32_t)sizeof(uint16_t);
}

static inline uint32_t
buffer_write_u16(buffer_t *buffer, uint16_t data)
{
	uint32_t write_len = buffer_write_u16_at(buffer, buffer->_position, data);
	buffer->_position += write_len;
    if (buffer->_limit < buffer->_position)
        buffer->_limit = buffer->_position;
    return write_len;
}

static inline uint32_t
buffer_write_u32_at(buffer_t *buffer, size_t at, uint32_t data)
{
    if (!buffer_available_at(buffer, at, sizeof(data)))
        return 0;

    *(uint32_t *)(buffer->_data + at) = data;
    return (uint32_t)sizeof(uint32_t);
}

static inline uint32_t
buffer_write_u32(buffer_t *buffer, uint32_t data)
{
	uint32_t write_len = buffer_write_u32_at(buffer, buffer->_position, data);
	buffer->_position += write_len;
    if (buffer->_limit < buffer->_position)
        buffer->_limit = buffer->_position;
    return write_len;
}

static inline uint32_t
buffer_read_at(buffer_t *buffer, size_t at, void *data, size_t count)
{
	int ret = buffer_remaining_at(buffer, at, count);
    if (-1 == ret)
        return 0;

	memcpy(data, buffer->_data + at, count);
    return (uint32_t)count;
}

static inline uint32_t
buffer_read(buffer_t *buffer, void *data, size_t count)
{
	uint32_t read_len = buffer_read_at(buffer, buffer->_position, data, count);

	buffer->_position += read_len;
    return read_len;
}

static inline uint32_t
buffer_read_u8_at(buffer_t *buffer, size_t at, uint8_t *u8)
{
    int ret = buffer_remaining_at(buffer, at, sizeof(uint8_t));
    if (-1 == ret)
        return 0;

	*u8 = buffer->_data[at];
    return (uint32_t)sizeof(uint8_t);
}

static inline uint32_t
buffer_read_u8(buffer_t *buffer, uint8_t *u8)
{
	uint32_t read_len = buffer_read_u8_at(buffer, buffer->_position, u8);
    
	buffer->_position += read_len;
	return read_len;
}

static inline uint8_t
buffer_read_u8_ex(buffer_t *buffer)
{
    uint8_t ret = *(buffer->_data + buffer->_position);
    buffer->_position += sizeof(uint8_t);
    return ret;
}

static inline uint16_t
buffer_read_u16_ex(buffer_t *buffer)
{
    uint16_t ret = *(uint16_t *)(buffer->_data + buffer->_position);
    buffer->_position += sizeof(uint16_t);
    return ret;
}

static inline uint32_t
buffer_read_u32_ex(buffer_t *buffer)
{
    uint32_t ret = *(uint32_t *)(buffer->_data + buffer->_position);
    buffer->_position += sizeof(uint32_t);
    return ret;
}


static inline uint32_t
buffer_read_u16_at(buffer_t *buffer, size_t at, uint16_t *u16)
{
    int ret = buffer_remaining_at(buffer, at, sizeof(uint16_t));
    if (-1 == ret)
        return 0;

	*u16 = *(uint16_t *)(buffer->_data + at);
    return (uint32_t)sizeof(uint16_t);
}

static inline uint32_t
buffer_read_u16(buffer_t *buffer, uint16_t *u16)
{
    uint32_t read_len = buffer_read_u16_at(buffer, buffer->_position, u16);

	buffer->_position += read_len;
	return read_len;
}

static inline uint32_t
buffer_read_u32_at(buffer_t *buffer, size_t at, uint32_t *u32)
{
    int ret = buffer_remaining_at(buffer, at, sizeof(uint32_t));
    if (-1 == ret)
        return 0;

	*u32 = *(uint32_t *)(buffer->_data + at);
    return (uint32_t)sizeof(uint32_t);
}

static inline uint32_t
buffer_read_u32(buffer_t *buffer, uint32_t *u32)
{
    uint32_t read_len = buffer_read_u32_at(buffer, buffer->_position, u32);
    
	buffer->_position += read_len;
	return read_len;
}

uint32_t
buffer_printf(buffer_t *buffer, const char *format, ...);

void
buffer_dump(buffer_t *buffer);

void
mem_dump(char *buf, int len);

bool
buffer_add_capacity(buffer_t *buffer, size_t add_capacity);

#endif /* _H_DIG_BUFFER_H_ */
