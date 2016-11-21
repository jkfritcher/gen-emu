#ifndef _GEN_EMU_H_
#define _GEN_EMU_H_


typedef signed char					sint8_t;
typedef signed short				sint16_t;
typedef signed int					sint32_t;
typedef signed long long			sint64_t;

typedef unsigned char				uint8_t;
typedef unsigned short				uint16_t;
typedef unsigned int				uint32_t;
typedef unsigned long long			uint64_t;

typedef volatile signed char		vsint8_t;
typedef volatile signed short		vsint16_t;
typedef volatile signed int			vsint32_t;
typedef volatile signed long long	vsint64_t;

typedef volatile unsigned char		vuint8_t;
typedef volatile unsigned short		vuint16_t;
typedef volatile unsigned int		vuint32_t;
typedef volatile unsigned long long	vuint64_t;

extern uint8_t debug;
extern uint8_t vdp_debug;
extern uint8_t quit;
extern uint8_t dump;

/* From misc.s */
extern uint16_t endswaps(uint16_t);
extern uint32_t endswapl(uint32_t);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAPBYTES16(x) __builtin_bswap16((x))
#define SWAPBYTES32(x) __builtin_bswap32((x))
#define SWAP_WORDS(x) ((((x) & 0xffff0000) >> 16) | (((x) & 0xffff) << 16))
#else
#define SWAPBYTES16(x) (x)
#define SWAPBYTES32(x) (x)
#define SWAP_WORDS(x) (x)
#endif  /* __BYTE_ORDER__ */

#define min(a, b) (a) < (b) ? (a) : (b)

#endif /* _GEN_EMU_H_ */
