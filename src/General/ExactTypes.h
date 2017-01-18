#ifndef EXACTTYPES_H
#define EXACTTYPES_H

#include <stdint.h>

#ifdef _M_IX86
#define LITTLE_ENDIAN
#else
#define BIG_ENDIAN
#endif

#ifdef LITTLE_ENDIAN
// Optimized for little-endian
inline uint16_t letohs(uint16_t value)  { return value; }
inline uint32_t letohl(uint32_t value)  { return value; }
inline uint64_t letohll(uint64_t value) { return value; }
inline uint16_t htoles(uint16_t value)  { return value; }
inline uint32_t htolel(uint32_t value)  { return value; }
inline uint64_t htolell(uint64_t value) { return value; }

#else
// Big-endian or unknown little-endian
inline uint64_t letohll(uint64_t value)
{
	return (((uint64_t)((uint8_t*)&value)[7]) << 56) | (((uint64_t)((uint8_t*)&value)[6]) << 48) |
	       (((uint64_t)((uint8_t*)&value)[5]) << 40) | (((uint64_t)((uint8_t*)&value)[4]) << 32) |
	       (((uint64_t)((uint8_t*)&value)[3]) << 24) | (((uint64_t)((uint8_t*)&value)[2]) << 16) |
	       (((uint64_t)((uint8_t*)&value)[1]) <<  8) | (((uint64_t)((uint8_t*)&value)[0]) <<  0);
}

inline uint32_t letohl(uint32_t value)
{
	return (((uint32_t)((uint8_t*)&value)[3]) << 24) | (((uint32_t)((uint8_t*)&value)[2]) << 16)|
	       (((uint32_t)((uint8_t*)&value)[1]) <<  8) | (((uint32_t)((uint8_t*)&value)[0]) <<  0);
}

inline uint16_t letohs(uint16_t value)
{
	return (((uint16_t)((uint8_t*)&value)[1]) << 8) | (((uint16_t)((uint8_t*)&value)[0]) << 0);
}

inline uint64_t htolell(uint64_t value)
{
	uint64_t tmp;
	((uint8_t*)&tmp)[0] = (uint8_t)(value >>  0);
	((uint8_t*)&tmp)[1] = (uint8_t)(value >>  8);
	((uint8_t*)&tmp)[2] = (uint8_t)(value >> 16);
	((uint8_t*)&tmp)[3] = (uint8_t)(value >> 24);
	((uint8_t*)&tmp)[4] = (uint8_t)(value >> 32);
	((uint8_t*)&tmp)[5] = (uint8_t)(value >> 40);
	((uint8_t*)&tmp)[6] = (uint8_t)(value >> 48);
	((uint8_t*)&tmp)[7] = (uint8_t)(value >> 56);
	return tmp;
}

inline uint32_t htolel(uint32_t value)
{
	uint32_t tmp;
	((uint8_t*)&tmp)[0] = (uint8_t)(value >>  0);
	((uint8_t*)&tmp)[1] = (uint8_t)(value >>  8);
	((uint8_t*)&tmp)[2] = (uint8_t)(value >> 16);
	((uint8_t*)&tmp)[3] = (uint8_t)(value >> 24);
	return tmp;
}

inline uint16_t htoles(uint16_t value)
{
	uint16_t tmp;
	((uint8_t*)&tmp)[0] = (uint8_t)(value >> 0);
	((uint8_t*)&tmp)[1] = (uint8_t)(value >> 8);
	return tmp;
}
#endif

#endif