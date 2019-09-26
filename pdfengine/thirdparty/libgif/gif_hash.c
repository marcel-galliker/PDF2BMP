

#include <stdio.h>
#include <string.h>
#include "gif_lib.h"
#include "gif_hash.h"
#include "gif_lib_private.h"

#ifdef	DEBUG_HIT_RATE
static long NumberOfTests = 0,
	    NumberOfMisses = 0;
#endif	

static int KeyItem(UINT32 Item);

GifHashTableType *_InitHashTable(void)
{
    GifHashTableType *HashTable;

    if ((HashTable = (GifHashTableType *) malloc(sizeof(GifHashTableType)))
	== NULL)
	return NULL;

    _ClearHashTable(HashTable);

    return HashTable;
}

void _ClearHashTable(GifHashTableType *HashTable)
{
    memset(HashTable -> HTable, 0xFF, HT_SIZE * sizeof(UINT32));
}

void _InsertHashTable(GifHashTableType *HashTable, UINT32 Key, int Code)
{
    int HKey = KeyItem(Key);
    UINT32 *HTable = HashTable -> HTable;

#ifdef DEBUG_HIT_RATE
	NumberOfTests++;
	NumberOfMisses++;
#endif 

    while (HT_GET_KEY(HTable[HKey]) != 0xFFFFFL) {
#ifdef DEBUG_HIT_RATE
	    NumberOfMisses++;
#endif 
	HKey = (HKey + 1) & HT_KEY_MASK;
    }
    HTable[HKey] = HT_PUT_KEY(Key) | HT_PUT_CODE(Code);
}

int _ExistsHashTable(GifHashTableType *HashTable, UINT32 Key)
{
    int HKey = KeyItem(Key);
    UINT32 *HTable = HashTable -> HTable, HTKey;

#ifdef DEBUG_HIT_RATE
	NumberOfTests++;
	NumberOfMisses++;
#endif 

    while ((HTKey = HT_GET_KEY(HTable[HKey])) != 0xFFFFFL) {
#ifdef DEBUG_HIT_RATE
	    NumberOfMisses++;
#endif 
	if (Key == HTKey) return HT_GET_CODE(HTable[HKey]);
	HKey = (HKey + 1) & HT_KEY_MASK;
    }

    return -1;
}

static int KeyItem(UINT32 Item)
{
    return ((Item >> 12) ^ Item) & HT_KEY_MASK;
}

#ifdef	DEBUG_HIT_RATE

void HashTablePrintHitRatio(void)
{
    printf("Hash Table Hit Ratio is %ld/%ld = %ld%%.\n",
	NumberOfMisses, NumberOfTests,
	NumberOfMisses * 100 / NumberOfTests);
}
#endif	
