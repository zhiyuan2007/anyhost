#include "dig_atomic.h"
#include "dig_wire_name.h"
#include "zip_utils.h"

#define MASK_U16            0xffff
#define COMPRESS_MASK_U8    0xc0
#define COMPRESS_MASK_U16   0xc000

struct wire_name
{
    uint8_t     label_count;
    uint8_t     name_len; /* len of "1a1b2cn" is 7 so the smallest name_len is 2 except root */
    atomic_t    ref_count;
    const void  *name_ptr; /* if name_ptr exist, it points to domain */
    uint8_t *raw_name;
    uint8_t *label_array;
    /*
     *following the head is 
     * content
     * label_len_array[label_count]
     */
};

#define WIRE_NAME_STRUCT_SIZE(name) (sizeof(wire_name_t) + name->raw_name - (uint8_t *)(name + 1) + name->label_count + name->name_len)

static wire_name_t root_name = 
{
    0,
    0,
    {1},
    NULL,
    NULL,
    NULL
};

static const char str_root[2] = {'.', 0};

static wire_name_t *
wire_name_create2(const uint8_t *begin_pos, 
                    uint8_t label_count, 
                    const uint8_t *label_len_array, 
                    uint32_t wire_len);

static wire_name_t *
wire_name_create3(const uint8_t *head_data,
                    const uint8_t *tail_data,
                    uint8_t head_label_count,
                    uint8_t tail_label_count,
                    const uint8_t *head_label_array,
                    const uint8_t *tail_label_array,
                    uint8_t head_len,
                    uint8_t tail_len);
static int
get_uncompressed_raw_name_buf(buffer_t *raw_buf, 
                    buffer_t *name_buf);




wire_name_t *
wire_name_from_wire(buffer_t *buf)
{
    ASSERT(buf && buffer_position(buf) <= buffer_actual_capacity(buf), "invalid buf to create wire name\n");
    if (buffer_position(buf) == buffer_actual_capacity(buf))
    {
        return NULL;
    }

    uint8_t uncompressed_data[MAX_WIRE_NAME_LEN] = {0};
    buffer_t uncompressed_buf;
    buffer_create_from(&uncompressed_buf, uncompressed_data, sizeof(uncompressed_data));
    if (get_uncompressed_raw_name_buf(buf, &uncompressed_buf))
        return NULL;

    buffer_set_position(&uncompressed_buf, 0);
    uint32_t before_pos = (uint32_t)buffer_position(&uncompressed_buf);
    const uint8_t *begin_pos = buffer_current(&uncompressed_buf);
    uint8_t label_len_array[MAX_LABEL_COUNT];
    uint8_t label_count = 0;

    uint8_t *pos = (uint8_t *)begin_pos;
    if (0 == *pos)
    {
        if (buffer_position(&uncompressed_buf) < buffer_actual_capacity(&uncompressed_buf) - 1)
            buffer_skip(&uncompressed_buf, 1);
        return (wire_name_t *)&root_name;
    }

    while (*pos)
    {
        if (*pos > MAX_LABEL_LEN)
        {
            goto CREATE_FAILED;
        }

        if ((buffer_position(&uncompressed_buf) + 1 + *pos) > (buffer_actual_capacity(&uncompressed_buf) - 1))
        {
            goto CREATE_FAILED;
        }

        label_len_array[MAX_LABEL_COUNT - 1 - label_count] = *pos;
        buffer_skip(&uncompressed_buf, *pos + 1);
        pos = buffer_current(&uncompressed_buf);
        ++label_count;
    }

    uint32_t now_position = (uint32_t)buffer_position(&uncompressed_buf);


    //no last zero
    uint32_t wire_len = now_position - before_pos;
    if (wire_len > (MAX_WIRE_NAME_LEN - 1))
    {
        goto CREATE_FAILED;
    }

    return wire_name_create2(begin_pos, label_count, &(label_len_array[MAX_LABEL_COUNT - label_count]), wire_len);

CREATE_FAILED :
    buffer_set_position(buf, before_pos);
    return NULL;
}

wire_name_t *
wire_name_from_text(const char *src_str)
{
    ASSERT(src_str, "null str to create wire name\n");
    int len = strlen(src_str);
    if (0 == len || (len > MAX_WIRE_NAME_LEN - 1))
        return NULL;

    if (1 == len)
    {
        if (!strcmp(src_str, "."))
            return (wire_name_t *)&root_name;
    }

    uint8_t data_buf[MAX_WIRE_NAME_LEN + 1] = {0};
    uint8_t label_len_array_buf[MAX_LABEL_COUNT] = {0};
    uint8_t label_count = 0;
    uint32_t wire_len = 0;

    strncpy((char *)(data_buf + 1), src_str, strlen(src_str));
    uint8_t *cur_pos = data_buf + 1;
    uint8_t *len_pos = data_buf;
    uint32_t cur_len = 0;
    bool is_dot_end = false;
    uint32_t label_len = 0;
    while (*cur_pos && (cur_len < len))
    {
        if ('.' == *cur_pos)
        {
            if (is_dot_end)
                return NULL;
            label_len = (uint32_t)(cur_pos - len_pos - 1);
            if (label_len > MAX_LABEL_LEN)
                return NULL;
            *len_pos = (uint8_t)label_len;
            label_len_array_buf[MAX_LABEL_COUNT - label_count - 1] = *len_pos;
            len_pos = cur_pos;
            ++label_count;
            is_dot_end = true;
        }
        else
            is_dot_end = false;

        ++cur_pos;
        ++cur_len;
    }

    if (!is_dot_end)
    {
        label_len = (uint32_t)(cur_pos - len_pos - 1);
        if (label_len > MAX_LABEL_LEN)
            return NULL;

        *len_pos = (uint8_t)label_len;
        label_len_array_buf[MAX_LABEL_COUNT - label_count - 1] = *len_pos;
        ++label_count;
    }

    if (is_dot_end)
        wire_len = cur_pos - data_buf - 1;
    else
        wire_len = cur_pos - data_buf;

    if ((wire_len > (MAX_WIRE_NAME_LEN - 1)) || (label_count > MAX_LABEL_COUNT))
        return NULL;

    return wire_name_create2(data_buf, label_count, &(label_len_array_buf[MAX_LABEL_COUNT - label_count]), wire_len);

}

wire_name_t *
wire_name_clone(const wire_name_t *src)
{
    atomic_inc(&(((wire_name_t *)src)->ref_count));
    return (wire_name_t *)src;
}

wire_name_t *
wire_name_copy(const wire_name_t *src)
{
    if (wire_name_is_root(src))
        return (wire_name_t *)&root_name;
    wire_name_t *name_copy = NULL;
    int wire_name_struct_size = WIRE_NAME_STRUCT_SIZE(src);
    DIG_CALLOC(name_copy, 1, wire_name_struct_size);
    if (!name_copy)
        return NULL;
    memcpy(name_copy, src, wire_name_struct_size);
    atomic_set(&(name_copy->ref_count), 1);
    name_copy->raw_name = (uint8_t *)(name_copy + 1) +
                                ((uint8_t *)(src->raw_name) - (uint8_t *)(src + 1));
    name_copy->label_array = name_copy->raw_name + src->name_len;
    return name_copy;
}

void
wire_name_delete(wire_name_t *name)
{
    ASSERT(name, "invalid name to delete\n");
    if (wire_name_is_root(name))
        return;
    if (atomic_dec_and_test(&(name->ref_count)))
        free(name);
}

inline uint8_t
wire_name_get_len(const wire_name_t *src)
{
    ASSERT(src, "null wire name to get len\n");
    return (src->name_len + 1);
}

inline uint8_t
wire_name_get_label_count(const wire_name_t *src)
{
    ASSERT(src, "null wire name to get label count\n");
    return src->label_count;
}

inline const void *
wire_name_get_ptr(const wire_name_t *src)
{
    ASSERT(src, "null wire name to get name ptr\n");
    return src->name_ptr;
}

inline void
wire_name_set_prt(wire_name_t *src, const void *ptr)
{
    ASSERT(src, "null wire name to set ptr\n");
    src->name_ptr = ptr;
}

wire_name_t *
wire_name_cut(const wire_name_t *long_name, const wire_name_t *short_name)
{
    ASSERT(long_name && short_name, "invalid name to wire name cut\n");
    if (!wire_name_contains(long_name, short_name))
        return NULL;

    if (wire_name_is_root(short_name))
        return wire_name_clone(long_name);
    else if (wire_name_is_equal(long_name, short_name))
        return NULL;

    //uint8_t long_len = long_name->name_len;
    //uint8_t short_len = short_name->name_len;

    //uint8_t cut_name_len = long_len - short_len;
    ////uint8_t cut_name_len = long_len - short_len  + 1;
    //uint8_t label_count = long_name->label_count - short_name->label_count;

    //return wire_name_create2(long_name->raw_name, label_count, 
    //                          long_name->label_array + short_name->label_count, cut_name_len);
    wire_name_t *prefix_name = NULL;
    wire_name_split(long_name, long_name->label_count - short_name->label_count,
            &prefix_name, NULL);
    return prefix_name;
}

wire_name_t *
wire_name_concat(const wire_name_t *head, const wire_name_t *tail)
{
    ASSERT(head && tail, "invalid name to wire name concat\n");

    uint8_t head_len = head->name_len;
    uint8_t tail_len = tail->name_len;
    uint8_t head_label_count = head->label_count;
    uint8_t tail_label_count = tail->label_count;

    uint32_t total_len = (uint32_t)head_len + (uint32_t)tail_len;
    if (total_len > (MAX_WIRE_NAME_LEN - 1))
        return NULL;
    uint32_t total_label_count = (uint32_t)head_label_count + (uint32_t)tail_label_count;
    if (total_label_count > MAX_LABEL_COUNT)
        return NULL;
    return wire_name_create3(head->raw_name, tail->raw_name, head->label_count, tail->label_count, head->label_array, tail->label_array, head_len, tail_len);
}

wire_name_t *
wire_name_parent(const wire_name_t *child_name)
{
    uint8_t child_label_count = child_name->label_count;
    if (0 == child_label_count)
        return NULL;
    else if (1 == child_label_count)
        return (wire_name_t *)&root_name;
    else
    {
        uint8_t head_len = (child_name->label_array)[child_label_count - 1] + 1;
        uint8_t parent_len = child_name->name_len - head_len;
        return wire_name_create2(child_name->raw_name + head_len, child_name->label_count - 1,
                                child_name->label_array, parent_len);
    }
}

wire_name_t *
wire_name_to_parent(wire_name_t *child_name)
{
    if (&root_name == child_name)
        return NULL;
    if (atomic_read(&(child_name->ref_count)) > 1)
        ASSERT(0, "name cloned can not be modified\n");

    uint8_t label_count = wire_name_get_label_count(child_name);
    if (label_count == 1)
    {
        wire_name_delete(child_name);
        return &root_name;
    }

    uint8_t last_label_len = (child_name->label_array)[label_count - 1];
    child_name->raw_name += last_label_len + 1;
    child_name->name_len -= last_label_len + 1;
    child_name->label_count -= 1;
    return child_name;
}

void
wire_name_split(const wire_name_t *src, uint8_t begin_label_count, 
                wire_name_t **prefix_part_name,
                wire_name_t **suffix_part_name)
{
    ASSERT(src, "null wire name to split\n");
    uint8_t label_count = src->label_count;
    ASSERT(begin_label_count <= label_count, "split begin label count exceed source max label count number\n");

    wire_name_t *pre_name = NULL;
    wire_name_t *suf_name = NULL;

    if (0 == begin_label_count)
    {
        pre_name = NULL;
        suf_name = wire_name_clone(src);
    }
    else if (label_count == begin_label_count)
    {
        pre_name = wire_name_clone(src);
        suf_name = NULL;
    }
    else
    {
        uint8_t i = 0;
        uint8_t prefix_len = 0;
        while (true)
        {
            if (i >= begin_label_count)
                break;
            prefix_len += (src->label_array)[label_count - 1 - i] + 1;
            ++i;
        }

        if (prefix_part_name)
        {
            pre_name = wire_name_create2(src->raw_name, 
                                        begin_label_count, 
                                        &((src->label_array)[label_count - begin_label_count]),
                                        prefix_len);
        }

        if (suffix_part_name)
        {
            suf_name = wire_name_create2(src->raw_name + prefix_len,
                                    label_count - begin_label_count,
                                    src->label_array,
                                    src->name_len - prefix_len);
        }
    }

    if (prefix_part_name)
        *prefix_part_name = pre_name;
    else
    {
        if (pre_name)
            wire_name_delete(pre_name);
    }

    if (suffix_part_name)
        *suffix_part_name = suf_name;
    else
    {
        if (suf_name)
            wire_name_delete(suf_name);
    }
}

int
wire_name_full_cmp(const wire_name_t *name1,
                    const wire_name_t *name2,
                    name_relation_t *relation,
                    uint8_t *common_label_count)
{
    ASSERT(name1 && name2 && relation && common_label_count, "invalid wire names to compare\n");

    if (wire_name_is_root(name1) && wire_name_is_root(name2))
    {
        *relation = EQUALNAME;
        *common_label_count = 0;
        return 0;
    }
    else if (wire_name_is_root(name1))
    {
        *relation = SUPERNAME;
        *common_label_count = 0;
        return -1;
    }
    else if (wire_name_is_root(name2))
    {
        *relation = SUBNAME;
        *common_label_count = 0;
        return 1;
    }

    uint8_t name1_label_count = name1->label_count;
    uint8_t name2_label_count = name2->label_count;
    uint8_t min_label_count = MIN(name1_label_count,name2_label_count);
    uint8_t i = 0;
    const uint8_t *label1 = name1->raw_name + name1->name_len;
    const uint8_t *label2 = name2->raw_name + name2->name_len;
    for (; i < min_label_count; ++i)
    {
        int num = (0 == i) ? 0 : 1;
        label1 = label1 - (name1->label_array)[i] - num;
        label2 = label2 - (name2->label_array)[i] - num;
        uint8_t label1_len = (name1->label_array)[i];
        uint8_t label2_len = (name2->label_array)[i];
        uint8_t min_label_len = MIN(label1_len, label2_len);

        int ret = strncasecmp((char *)label1,(char *)label2, min_label_len);
        if (!(0 == ret && label1_len == label2_len))
        {
            if (ret != 0)
                ret = ((ret < 0) ? -1 : 1);
            else if (label1_len < label2_len)
                ret = -1;
            else if (label1_len > label2_len)
                ret = 1;
            else
                ASSERT(0, "internal logic error in wire name full cmp\n");

            *relation = COMMONANCESTOR;
            *common_label_count = i;
            return ret;
        }
    }

    if (name1_label_count == name2_label_count)
    {
        *relation = EQUALNAME;
        *common_label_count = name1_label_count;
        return 0;
    }
    else if (name1_label_count < name2_label_count)
    {
        *relation = SUPERNAME;
        *common_label_count = name1_label_count;
        return -1;
    }
    else
    {
        *relation = SUBNAME;
        *common_label_count = name2_label_count;
        return 1;
    }
}

inline int
wire_name_compare(const wire_name_t *name1, const wire_name_t *name2)
{
    name_relation_t relation = EQUALNAME;
    uint8_t common_label_count = 0;
    return wire_name_full_cmp(name1, name2, &relation, &common_label_count);
}

int
wire_name_label_compare(const wire_name_t *name1, const wire_name_t *name2, int label_index)
{
    /*
    char name_buf_1[MAX_WIRE_NAME_LEN];
    char name_buf_2[MAX_WIRE_NAME_LEN];
    buffer_t buffer_1;
    buffer_t buffer_2;
    buffer_create_from(&buffer_1, name_buf_1, MAX_WIRE_NAME_LEN);
    buffer_create_from(&buffer_2, name_buf_2, MAX_WIRE_NAME_LEN);
    wire_name_to_text(name1, &buffer_1); 
    wire_name_to_text(name2, &buffer_2); 
    printf("<<<<<<compare name %s with %s for label %d\n", name_buf_1, name_buf_2, label_index);
    */

    ASSERT(label_index >= 0 && label_index < name1->label_count && label_index < name2->label_count, 
            "invalid label index to compare two name\n");

    uint8_t label_len1 = (name1->label_array)[label_index];
    uint8_t label_len2 = (name2->label_array)[label_index];
    uint8_t cmp_len = MIN(label_len1, label_len2);

    const uint8_t *label1 = name1->raw_name + name1->name_len - (name1->label_array)[0];
    const uint8_t *label2 = name2->raw_name + name2->name_len - (name2->label_array)[0];
    uint8_t i = 0;
    for (; i < label_index; ++i)
    {
        label1 = label1 - (name1->label_array)[i + 1] - 1;
        label2 = label2 - (name2->label_array)[i + 1] - 1;
    }

    int ret = strncasecmp((char *)label1, (char *)label2, cmp_len);
    if (!(0 == ret && label_len1 == label_len2))
    {
        if (ret != 0)
            return (ret < 0) ? -1 : 1;
        else if (label_len1 < label_len2)
            return -1;
        else if (label_len1 > label_len2)
            return 1;
        else
            ASSERT(0, "internal logic error in wire name label compare\n");
    }

    return 0;
}

bool
wire_name_is_equal(const wire_name_t *name1, const wire_name_t *name2)
{
    if (name1->name_len != name2->name_len)
        return false;

    if (name1->label_count != name2->label_count)
        return false;

    return (strncasecmp((char *)(name1->raw_name), (char *)(name2->raw_name), name1->name_len) == 0) &&
                (memcmp(name1->label_array, name2->label_array, name1->label_count) == 0);
}

bool
wire_name_contains(const wire_name_t *long_name, const wire_name_t *short_name)
{
    uint8_t short_len = short_name->name_len;
    uint8_t long_len = long_name->name_len;
    if (long_len < short_len)
        return false;

    uint8_t long_label_count = long_name->label_count;
    uint8_t short_label_count = short_name->label_count;
    if (long_label_count < short_label_count)
        return false;

    if (wire_name_is_root(short_name))
        return true;

    const uint8_t *short_data_in_longer_pos = long_name->raw_name + long_len - short_len;
    return strncasecmp((char *)short_data_in_longer_pos, (char *)(short_name->raw_name), short_len) == 0;
}

bool
wire_name_is_root(const wire_name_t *name)
{
    ASSERT(name, "invalid name to call wire name is root\n");
    return 0 == name->label_count;
}

inline bool
wire_name_is_wildcard(const wire_name_t *name)
{
    ASSERT(name, "invalid name to call wire name is wildcard\n");
    return (name->raw_name)[1] == '*';
}

uint32_t
wire_name_to_wire(const wire_name_t *name, buffer_t *buf)
{
    ASSERT(name && buf, "invalid name or buf to call wire name to wire\n");

    if (!buffer_capacity_is_enough(buf, name->name_len + 1))
        return 0;
    
    if (!wire_name_is_root(name))
        buffer_write(buf, name->raw_name, name->name_len);
    buffer_write_u8(buf, (uint8_t)0);

    return ((uint32_t)(name->name_len) + 1);
}

uint32_t
wire_name_to_text(const wire_name_t *name, buffer_t *buf)
{
    ASSERT(name && buf, "invalid parameters for wire name to text\n");

    if (!buffer_capacity_is_enough(buf, 0 == name->name_len ? 2 : name->name_len))
        return 0;

    if (wire_name_is_root(name))
    {
        buffer_write_u8(buf, str_root[0]);
        buffer_write_u8(buf, str_root[1]);
        buffer_rollback(buf, 1);
        return 2;
    }

    uint8_t *des_buf = (uint8_t *)buffer_current(buf);
    memcpy(des_buf, name->raw_name + 1, name->name_len - 1);
    uint8_t *p = des_buf + name->name_len - 1;
    int i = 0;
    while (1)
    {
        p = p - (name->label_array)[i] - 1;
        if (p > des_buf)
        {
            ++i;
            *p = '.';
        }
        else
            break;
    }

    buffer_skip(buf, name->name_len - 1);
    buffer_write_u8(buf, '.');
    buffer_write_u8(buf, 0);
    buffer_rollback(buf, 1);
    return name->name_len;
}

wire_name_to_string(const wire_name_t *name, char *domain_name)
{
    buffer_t buf;
    buffer_create_from(&buf, domain_name, 512);
    wire_name_to_text(name, &buf);
}
uint32_t
wire_name_hash(const wire_name_t *name)
{
    uint8_t length = name->name_len;
    uint32_t hash_value = 0;
    uint8_t *cur_pos = name->raw_name;

    while (length > 0)
    {
        hash_value += (hash_value << 3) + (uint32_t)*cur_pos;
        ++cur_pos;
        --length;
    }

    return hash_value; 
}

static wire_name_t *
wire_name_create2(const uint8_t *begin_pos, 
                    uint8_t label_count, 
                    const uint8_t *label_len_array, 
                    uint32_t wire_len)
{
    wire_name_t *name = NULL;
    DIG_CALLOC(name, 1, sizeof(wire_name_t) + label_count + wire_len);
    if (!name)
        return NULL;
    atomic_set(&(name->ref_count), 1);

    name->name_len = (uint8_t)wire_len;
    name->label_count = label_count;
    memcpy(name + 1, begin_pos, wire_len);
    //*((uint8_t*)(name + 1) + wire_len - 1) = 0;
    memcpy((uint8_t *)(name + 1) + wire_len, label_len_array, label_count);
    name->raw_name = (uint8_t *)(name + 1);
    name->label_array = name->raw_name + wire_len;
    return name;
}

static wire_name_t *
wire_name_create3(const uint8_t *head_data,
                    const uint8_t *tail_data,
                    uint8_t head_label_count,
                    uint8_t tail_label_count,
                    const uint8_t *head_label_array,
                    const uint8_t *tail_label_array,
                    uint8_t head_len,
                    uint8_t tail_len)
{
    wire_name_t *name = NULL;
    DIG_CALLOC(name, 1, sizeof(wire_name_t) + head_label_count + tail_label_count + head_len + tail_len);
    if (!name)
        return NULL;
    atomic_set(&(name->ref_count), 1);

    name->name_len = head_len + tail_len;
    name->label_count = head_label_count + tail_label_count;
    memcpy(name + 1, head_data, head_len);
    memcpy((uint8_t *)(name + 1) + head_len, tail_data, tail_len);
    memcpy((uint8_t *)(name + 1) + head_len + tail_len, tail_label_array, tail_label_count);
    memcpy((uint8_t *)(name + 1) + head_len + tail_len + tail_label_count, head_label_array, head_label_count);

    name->raw_name = (uint8_t *)(name + 1);
    name->label_array = name->raw_name + name->name_len;
    return name;
}
//
static int
get_uncompressed_raw_name_buf(buffer_t *raw_buf, buffer_t *name_buf)
{
#define CHECK_WRITE_LEN(len) do {\
    if (0 == len) goto DECOMPRESS_FAILED; \
} while (0)

#define CHECK_READ_LEN(buf, read_len) do {\
    if (!buffer_capacity_is_enough(buf, read_len))  { \
        goto DECOMPRESS_FAILED; }\
} while (0)

    size_t raw_before_position = buffer_position(raw_buf);
    size_t raw_mid_position = 0;
    size_t name_before_position = buffer_position(name_buf);
    uint32_t write_len = 0;
    uint32_t total_len = 0;
    bool done = false;

    while (!done)
    {
        CHECK_READ_LEN(raw_buf, sizeof(uint8_t));
        uint8_t c = buffer_read_u8_ex(raw_buf);
        if (c <= MAX_LABEL_LEN)
        {
            //uncompressed
            CHECK_READ_LEN(raw_buf, c);
            write_len = buffer_write(name_buf, buffer_current(raw_buf) - 1, c + 1);
            CHECK_WRITE_LEN(write_len);
            total_len += write_len;
            buffer_skip(raw_buf, c);
            if (total_len > MAX_WIRE_NAME_LEN) 
            {
                goto DECOMPRESS_FAILED;
            }
            if (0 == c)
                done = true;
        }
        else if ((c & COMPRESS_MASK_U8) == COMPRESS_MASK_U8)
        {
            //compressed
            uint8_t pos_head = c & ~COMPRESS_MASK_U8;
            CHECK_READ_LEN(raw_buf, sizeof(uint8_t));
            uint8_t pos_tail = buffer_read_u8_ex(raw_buf);
            uint16_t pos = (uint16_t)pos_head * 256 + (uint16_t)pos_tail;
            if (pos >= buffer_position(raw_buf))
            {
                goto DECOMPRESS_FAILED;
            }

            total_len += 2;
            raw_mid_position = buffer_position(raw_buf);
            buffer_set_position(raw_buf, pos);
            if (get_uncompressed_raw_name_buf(raw_buf, name_buf))
                goto DECOMPRESS_FAILED;

            buffer_set_position(raw_buf, raw_mid_position);
            done = true;
        }
        else
        {
            goto DECOMPRESS_FAILED;
        }
    }

    return 0;

DECOMPRESS_FAILED:
    buffer_set_position(raw_buf, raw_before_position);
    buffer_set_position(name_buf, name_before_position);
    return -1;
}

#define CHECK_WRITE_RET(status) do { \
    if (FAILED == status) return FAILED;\
} while (0)
