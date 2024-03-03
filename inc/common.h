#ifndef JDP_COMMON_INCLUDE
#define JDP_COMMON_INCLUDE

#include <stdint.h>
#include <stdbool.h>

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef i8          b8;
typedef i16         b16;
typedef i32         b32;

typedef float       f32;
typedef double      f64;

#define KILOBYTES(x)    ((x)*1024LL)
#define MEGABYTES(x)    (KILOBYTES(x)*1024LL)
#define GIGABYTES(x)    (MEGABYTES(x)*1024LL)
#define TERABYTES(x)    (GIGABYTES(x)*1024LL)

#define array_count(a)      (sizeof(a) / sizeof((a)[0]))
#define align_pow_2(x, b)   (((x) + ((b) - 1)) & (~((b) - 1)))

#define internal        static
#define global_variable static
#define local_persist   static

#define ARENA_COMMIT_SIZE       KILOBYTES(64)
#define COMMON_TEMP_BUF_LEN     KILOBYTES(4)


void * get_temp_buffer(void);
void * get_temp_buffer_alt(void);

#define dbg_print(format, ...)                                                          \
    do {                                                                                \
        stbsp_snprintf(get_temp_buffer(), COMMON_TEMP_BUF_LEN, format, __VA_ARGS__);    \
        OutputDebugStringA(get_temp_buffer());                                          \
    } while (0);

#define dbg_print_utf8(format, ...)                                                                                             \
    do {                                                                                                                        \
        stbsp_snprintf(get_temp_buffer(), COMMON_TEMP_BUF_LEN, format, __VA_ARGS__);                                            \
        int chars_written = MultiByteToWideChar(CP_UTF8, 0, get_temp_buffer(), -1, get_temp_buffer_alt(), COMMON_TEMP_BUF_LEN); \
        assert(chars_written != 0);                                                                                             \
        OutputDebugStringW(get_temp_buffer_alt());                                                                              \
    } while (0);

#define dbg_wprint(format, ...)                                                         \
    do {                                                                                \
        wchar_t * str_buf = (wchar_t *) get_temp_buffer();                              \
        size_t str_buf_len = COMMON_TEMP_BUF_LEN / sizeof(wchar_t);                     \
        _snwprintf(str_buf, str_buf_len, format, __VA_ARGS__);                          \
        OutputDebugStringW(str_buf);                                                    \
    } while (0);


typedef struct string_t string_t;
struct string_t
{
    char * ptr;
    i64 size;
};

#define string_lit(x) (string_t) { .ptr = (char *) (x), .size = sizeof(x) - 1 }
#define string_varg(s)  (int)(s).size, (s).ptr


typedef struct arena_t arena_t;
struct arena_t
{
    void * start;
    u64 pos;
    u64 committed;
    u64 reserved;
};

arena_t arena_alloc(u64 size);
void arena_free(arena_t * arena);
void * arena_push(arena_t * arena, u64 amount);
#define arena_push_array(arena, type, count) ((type *) arena_push(arena, sizeof(type) * (count)))


#ifdef JDP_COMMON_IMPLEMENTATION

void * get_temp_buffer()
{
    static char buffer[COMMON_TEMP_BUF_LEN];

    return (void *) buffer;
}

void * get_temp_buffer_alt()
{
    static char buffer[COMMON_TEMP_BUF_LEN];

    return (void *) buffer;
}


arena_t arena_alloc(u64 size)
{
    void * start = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);

    arena_t arena = {0};
    arena.start = start;
    arena.pos = 0;
    arena.committed = 0;
    arena.reserved = size;

    return arena;
}

void arena_free(arena_t * arena)
{
    BOOL success = VirtualFree(arena->start, 0, MEM_RELEASE);
    assert(success);

    arena->start = NULL;
}

void * arena_push(arena_t * arena, u64 amount)
{
    u64 old_pos = arena->pos;
    assert(old_pos == align_pow_2(old_pos, 8));
    u64 new_pos = align_pow_2(old_pos + amount, 8);
    assert(new_pos >= old_pos);

    if (new_pos > arena->reserved)
    {
        assert(!"arena ran out of reserved memory");
    }

    while (new_pos > arena->committed)
    {
        void * success = VirtualAlloc((u8 *) arena->start + arena->committed, ARENA_COMMIT_SIZE, MEM_COMMIT, PAGE_READWRITE);
        assert(success);
        arena->committed += ARENA_COMMIT_SIZE;
    }

    arena->pos = new_pos;

    return (u8 *) arena->start + old_pos;
}

#endif /* JDP_COMMON_IMPLEMENTATION */
#endif /* JDP_COMMON_INCLUDE */
