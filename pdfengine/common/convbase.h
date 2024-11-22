#ifndef CONVBASE_H
#define CONVBASE_H

#define PDF_RESOLUTION_12		12.0f
#define PDF_RESOLUTION_72		72.0f
#define PDF_RESOLUTION_96		96.0f
#define PDF_RESOLUTION_120		120.0f
#define PDF_RESOLUTION_150		150.0f
#define PDF_RESOLUTION_300		300.0f
#define PDF_RESOLUTION_600		600.0f

#define MAX_NAME_SIZE       1024

#define     SUCCESS             0
#define     INVOLVING_EXIST     2
#define     ENCLOSING_EXIST     4
#define     CROSSING_EXIST      8
#define     RECTANGLE_INVOLVING 16
#define     RECTANGLE_CROSSING  32
#define     UNKNOWN_ERROR       64

typedef enum
{
    FCS_NEVER       = 0x00,
    FCS_ADDED,
    FCS_WAITING,
	FCS_SKIPPED,
    FCS_CONV_ERRORED,
	FCS_FILE_ERRORED,
	FCS_STOPED,
    FCS_FINISHED
} FILE_CONV_STATUS;

typedef enum
{
	CRS_STARTED,
	CRS_SKIPPED,
	CRS_CONVERSION_ERROR,
	CRS_NOTEXISTFILE_ERROR,
	CRS_STOPED,
	CRS_SUCCESSED
} CONVERT_RESULT_STATUS;

typedef struct
{
    wchar_t fileName[MAX_NAME_SIZE];
    wchar_t srcPath[MAX_NAME_SIZE];
    wchar_t dstPath[MAX_NAME_SIZE];
    int pageCount;
    char selectPages[MAX_NAME_SIZE];
	int startNumber;
	int startPageNumber;
    unsigned char status;
	unsigned char finishStatus;
} ConvFileInfo;

typedef struct  
{
	float resolutionX;
	float resolutionY;
	float sizeWidth;
	float sizeHeight;
	float pY[5];
	int convertmode;
	int convertsplit;
	int	threadCnt;
	int pageNo;
} ConvOptions;

// bound box info
typedef struct
{
	int left;
	int top;
	int right;
	int bottom;
} INT_RECT;

typedef struct tagF_RECT
{
	float left;
	float top;
	float right;
	float bottom;
} F_RECT;

// size info
typedef struct
{
	int width;
	int height;
} INT_SIZE;

typedef struct
{
	// font info
	char fontname[32];
	float fontsize;
	bool bold;
	bool italic;
	bool underlined;
	bool monospace;
	bool serif;
	bool smallcaps;
	unsigned char fontcolor_r;
	unsigned char fontcolor_g;
	unsigned char fontcolor_b;


} STYLE_INFO;

typedef struct
{
	// bound box info
	INT_RECT box;

	// text info
	int text_len;
	wchar_t *szText;
	INT_RECT *boxlist;

	// style info
	STYLE_INFO style_info;

	// link info
	bool bLink;
	char *szLink;

	int flag; // 1 : deleted, 2 : crossed
} SPAN_INFO;

typedef struct
{
	// bound box info
	INT_RECT box;

	// span info
	int span_len;
	SPAN_INFO *spanlist;

	int flag; // 1 : deleted, 2 : crossed

} TEXT_INFO;

extern bool g_bStopThread;

extern float g_fConvPercent[];
extern float g_fConvPercentMin[];
extern float g_fConvPercentMax[];

int getConvPercent();


#endif // CONVBASE_H
