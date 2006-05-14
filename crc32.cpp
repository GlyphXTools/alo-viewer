#include "crc32.h"

static unsigned long lookupTable[256];
static bool          validTable = false;

static void initTable()
{
	for (int i = 0; i < 256; i++)
    {
		unsigned long crc = i;
        for (int j = 0; j < 8; j++)
		{
			crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
		}
        lookupTable[i] = crc & 0xFFFFFFFF;
	}
	validTable = true;
}

unsigned long crc32(const char *data, size_t size)
{
	if (!validTable)
	{
		initTable();
	}

	unsigned long crc = 0xFFFFFFFF;
	for (size_t j = 0; j < size; j++)
	{
		crc = ((crc >> 8) & 0x00FFFFFF) ^ lookupTable[ (crc ^ data[j]) & 0xFF ];
	}
	return crc ^ 0xFFFFFFFF;
}