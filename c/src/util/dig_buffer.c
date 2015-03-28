/*
 * buffer.c -- generic memory buffer .
 *
 * Copyright (c) 2001-2006, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "dig_buffer.h"

void
buffer_create(buffer_t *buffer, size_t init_size)
{
    buffer->_position = 0;
    buffer->_limit = init_size;
    buffer->_data = NULL;
    buffer->_fixed = 0;
    buffer_set_capacity(buffer, init_size);
}
void
buffer_create_from(buffer_t *buffer, void *data, size_t size)
{
    buffer->_position = 0;
    buffer->_limit = size;
    buffer->_actual_capacity = size;
    buffer->_reservered_capacity = size;
    buffer->_data = (uint8_t *) data;
    buffer->_fixed = 1;
}

void
buffer_clear(buffer_t *buffer)
{
    buffer->_position = 0;
    buffer->_limit = buffer->_reservered_capacity;
}

void
buffer_flip(buffer_t *buffer)
{
	buffer->_limit = buffer->_position;
	buffer->_position = 0;
}


bool
buffer_set_capacity(buffer_t *buffer, size_t capacity)
{
	if (buffer->_position > capacity)
        return false;

    buffer->_data = realloc(buffer->_data, capacity);
	buffer->_actual_capacity = capacity;
	buffer->_reservered_capacity = capacity;
    return true;
}

void
buffer_reserve(buffer_t *buffer, size_t amount)
{
	if (buffer->_reservered_capacity < buffer->_position + amount)
    {
		size_t new_capacity = buffer->_actual_capacity * 3 / 2;
		if (new_capacity < buffer->_position + amount)
			new_capacity = buffer->_position + amount;

		buffer_set_capacity(buffer, new_capacity);
	}
	buffer->_limit = buffer->_actual_capacity;
}

uint32_t
buffer_printf(buffer_t *buffer, const char *format, ...)
{
	va_list args;
	int written;
	size_t remaining;

	remaining = buffer_remaining(buffer);
	va_start(args, format);
	written = vsnprintf((char *) buffer_current(buffer), remaining,
			    format, args);
	va_end(args);
	if (written >= 0 && (size_t) written >= remaining)
        return 0;

	buffer->_position += written;
	return (uint32_t)written;
}

void
buffer_dump(buffer_t *buffer)
{
    int i;

    for (i = 0; i < buffer->_limit; i++)
    {
        if (i % 20 == 0)
            printf("\n");
        printf("%02x", buffer->_data[i]);
    }
    printf("\n");
}

void
mem_dump(char *mem, int len)
{
    buffer_t buf;

    buffer_create_from(&buf, mem, len);
    buffer_set_limit(&buf, len);
    buffer_dump(&buf);
}

bool
buffer_add_capacity(buffer_t *buffer, size_t add_capacity)
{
    int num = (buffer_position(buffer) + add_capacity) / buffer_reservered_capacity(buffer);
    return buffer_set_capacity(buffer, buffer_reservered_capacity(buffer) * (num + 1));
}
