#ifndef TYPES_H
#define TYPES_H

#ifdef _MSC_VER
typedef __int8				int8_t;
typedef __int16				int16_t;
typedef __int32				int32_t;
typedef __int64				int64_t;
typedef unsigned __int8		uint8_t;
typedef unsigned __int16	uint16_t;
typedef unsigned __int32	uint32_t;
typedef unsigned __int64	uint64_t;
const uint8_t	UINT8_MAX  = 0xFFI8;
const uint16_t	UINT16_MAX = 0xFFFFI16;
const uint32_t	UINT32_MAX = 0xFFFFFFFFI32;
const uint64_t	UINT64_MAX = 0xFFFFFFFFFFFFFFFFI64;
#else
#include <stdint.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

inline uint64_t letohll(uint64_t value)
{
	return (((uint8_t*)&value)[7] << 56) | (((uint8_t*)&value)[6] << 48) |
	       (((uint8_t*)&value)[5] << 40) | (((uint8_t*)&value)[4] << 32) |
	       (((uint8_t*)&value)[3] << 24) | (((uint8_t*)&value)[2] << 16) |
	       (((uint8_t*)&value)[1] <<  8) | (((uint8_t*)&value)[0] <<  0);
}

inline uint32_t letohl(uint32_t value)
{
	return (((uint8_t*)&value)[3] << 24) | (((uint8_t*)&value)[2] << 16)|
	       (((uint8_t*)&value)[1] <<  8) | (((uint8_t*)&value)[0] <<  0);
}

inline uint16_t letohs(uint16_t value)
{
	return (((uint8_t*)&value)[1] << 8) | (((uint8_t*)&value)[0] << 0);
}

inline uint64_t htolell(uint64_t value)
{
	uint64_t tmp;
	((uint8_t*)&tmp)[0] = (uint8_t)(value >> 56);
	((uint8_t*)&tmp)[1] = (uint8_t)(value >> 48);
	((uint8_t*)&tmp)[2] = (uint8_t)(value >> 40);
	((uint8_t*)&tmp)[3] = (uint8_t)(value >> 32);
	((uint8_t*)&tmp)[4] = (uint8_t)(value >> 24);
	((uint8_t*)&tmp)[5] = (uint8_t)(value >> 16);
	((uint8_t*)&tmp)[6] = (uint8_t)(value >>  8);
	((uint8_t*)&tmp)[7] = (uint8_t)(value >>  0);
	return tmp;
}

inline uint32_t htolel(uint32_t value)
{
	uint32_t tmp;
	((uint8_t*)&tmp)[0] = (uint8_t)(value >> 24);
	((uint8_t*)&tmp)[1] = (uint8_t)(value >> 16);
	((uint8_t*)&tmp)[2] = (uint8_t)(value >>  8);
	((uint8_t*)&tmp)[3] = (uint8_t)(value >>  0);
	return tmp;
}

inline uint16_t htoles(uint16_t value)
{
	uint16_t tmp;
	((uint8_t*)&tmp)[0] = (uint8_t)(value >> 8);
	((uint8_t*)&tmp)[1] = (uint8_t)(value >> 0);
	return tmp;
}

#endif