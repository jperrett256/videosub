#ifndef COMMON_H_INCLUDE
#define COMMON_H_INCLUDE

#include <stdint.h>

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef i8      b8;
typedef i16     b16;
typedef i32     b32;

typedef float   f32;
typedef double  f64;

#define KILOBYTES(x)    (x)         *1024LL
#define MEGABYTES(x)    KILOBYTES(x)*1024LL

#define internal static
#define global_variable static
#define local_persist static

typedef struct str8 str8;
struct str8
{
    char * str;
    i64 size;
};

#define str8_lit(x) (str8) { .str = (char *) (x), .size = sizeof(x) - 1 }

#endif /* COMMON_H_INCLUDE */
