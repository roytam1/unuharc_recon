/* Exact-width integers for the UHARC ports.
 *
 * Linux LP64 makes unsigned long 64 bits; Windows LLP64 makes it 32.
 * The decoders keep x86's 32-bit wrap, so the storage type has to be
 * uint32_t on both.  MSVC before 2010 has no <stdint.h>; unsigned
 * __intN is the same width there.  The function bodies stay C89:
 * declarations at the top of the block, UHA_INLINE instead of inline,
 * no designated initializers.
 */
#ifndef UHA_TYPES_H
#define UHA_TYPES_H

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef signed __int8    int8_t;
typedef signed __int16   int16_t;
typedef signed __int32   int32_t;
typedef signed __int64   int64_t;
#define UHA_INLINE __inline
#define UHA_I64(x) x##I64
#define UHA_U64(x) x##UI64
#else
#include <stdint.h>
#define UHA_INLINE inline
#define UHA_I64(x) x##LL
#define UHA_U64(x) x##ULL
#endif

#endif /* UHA_TYPES_H */
