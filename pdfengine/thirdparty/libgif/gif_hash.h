

#ifndef _GIF_HASH_H_
#define _GIF_HASH_H_

#define HT_SIZE			8192	   
#define HT_KEY_MASK		0x1FFF			      
#define HT_KEY_NUM_BITS		13			      
#define HT_MAX_KEY		8191	
#define HT_MAX_CODE		4095	

#define HT_GET_KEY(l)	(l >> 12)
#define HT_GET_CODE(l)	(l & 0x0FFF)
#define HT_PUT_KEY(l)	(l << 12)
#define HT_PUT_CODE(l)	(l & 0x0FFF)

typedef struct GifHashTableType {
    UINT32 HTable[HT_SIZE];
} GifHashTableType;

GifHashTableType *_InitHashTable(void);
void _ClearHashTable(GifHashTableType *HashTable);
void _InsertHashTable(GifHashTableType *HashTable, UINT32 Key, int Code);
int _ExistsHashTable(GifHashTableType *HashTable, UINT32 Key);

#endif 
