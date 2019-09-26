

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include "tiffiop.h"
#ifdef JPEG_SUPPORT

#include <setjmp.h>

int TIFFFillStrip(TIFF*, tstrip_t);
int TIFFFillTile(TIFF*, ttile_t);

#ifdef FAR
#undef FAR
#endif

#if defined(__BORLANDC__) || defined(__MINGW32__)
# define XMD_H 1
#endif

#if defined(WIN32) && !defined(__MINGW32__)
# ifndef __RPCNDR_H__            
   typedef unsigned char boolean;
# endif
# define HAVE_BOOLEAN            
#endif

#include "jpeglib.h"
#include "jerror.h"

#if defined(D_MAX_DATA_UNITS_IN_MCU)
#define width_in_blocks width_in_data_units
#endif

#define SETJMP(jbuf)		setjmp(jbuf)
#define LONGJMP(jbuf,code)	longjmp(jbuf,code)
#define JMP_BUF			jmp_buf

typedef struct jpeg_destination_mgr jpeg_destination_mgr;
typedef struct jpeg_source_mgr jpeg_source_mgr;
typedef	struct jpeg_error_mgr jpeg_error_mgr;

typedef	struct {
	union {
		struct jpeg_compress_struct c;
		struct jpeg_decompress_struct d;
		struct jpeg_common_struct comm;
	} cinfo;			
        int             cinfo_initialized;

	jpeg_error_mgr	err;		
	JMP_BUF		exit_jmpbuf;	
	
	jpeg_destination_mgr dest;	
	jpeg_source_mgr	src;		
					
	TIFF*		tif;		
	uint16		photometric;	
	uint16		h_sampling;	
	uint16		v_sampling;
	tsize_t		bytesperline;	
	
	JSAMPARRAY	ds_buffer[MAX_COMPONENTS];
	int		scancount;	
	int		samplesperclump;

	TIFFVGetMethod	vgetparent;	
	TIFFVSetMethod	vsetparent;	
	TIFFPrintMethod printdir;	
	TIFFStripMethod	defsparent;	
	TIFFTileMethod	deftparent;	
					
	void*		jpegtables;	
	uint32		jpegtables_length; 
	int		jpegquality;	
	int		jpegcolormode;	
	int		jpegtablesmode;	

        int             ycbcrsampling_fetched;
	uint32		recvparams;	
	char*		subaddress;	
	uint32		recvtime;	
	char*		faxdcs;		
} JPEGState;

#define	JState(tif)	((JPEGState*)(tif)->tif_data)

static	int JPEGDecode(TIFF*, tidata_t, tsize_t, tsample_t);
static	int JPEGDecodeRaw(TIFF*, tidata_t, tsize_t, tsample_t);
static	int JPEGEncode(TIFF*, tidata_t, tsize_t, tsample_t);
static	int JPEGEncodeRaw(TIFF*, tidata_t, tsize_t, tsample_t);
static  int JPEGInitializeLibJPEG( TIFF * tif,
								   int force_encode, int force_decode );

#define	FIELD_JPEGTABLES	(FIELD_CODEC+0)
#define	FIELD_RECVPARAMS	(FIELD_CODEC+1)
#define	FIELD_SUBADDRESS	(FIELD_CODEC+2)
#define	FIELD_RECVTIME		(FIELD_CODEC+3)
#define	FIELD_FAXDCS		(FIELD_CODEC+4)

static const TIFFFieldInfo jpegFieldInfo[] = {
    { TIFFTAG_JPEGTABLES,	 -3,-3,	TIFF_UNDEFINED,	FIELD_JPEGTABLES,
      FALSE,	TRUE,	"JPEGTables" },
    { TIFFTAG_JPEGQUALITY,	 0, 0,	TIFF_ANY,	FIELD_PSEUDO,
      TRUE,	FALSE,	"" },
    { TIFFTAG_JPEGCOLORMODE,	 0, 0,	TIFF_ANY,	FIELD_PSEUDO,
      FALSE,	FALSE,	"" },
    { TIFFTAG_JPEGTABLESMODE,	 0, 0,	TIFF_ANY,	FIELD_PSEUDO,
      FALSE,	FALSE,	"" },
    
    { TIFFTAG_FAXRECVPARAMS,	 1, 1, TIFF_LONG,	FIELD_RECVPARAMS,
      TRUE,	FALSE,	"FaxRecvParams" },
    { TIFFTAG_FAXSUBADDRESS,	-1,-1, TIFF_ASCII,	FIELD_SUBADDRESS,
      TRUE,	FALSE,	"FaxSubAddress" },
    { TIFFTAG_FAXRECVTIME,	 1, 1, TIFF_LONG,	FIELD_RECVTIME,
      TRUE,	FALSE,	"FaxRecvTime" },
    { TIFFTAG_FAXDCS,		-1, -1, TIFF_ASCII,	FIELD_FAXDCS,
	  TRUE,	FALSE,	"FaxDcs" },
};
#define	N(a)	(sizeof (a) / sizeof (a[0]))

static void
TIFFjpeg_error_exit(j_common_ptr cinfo)
{
	JPEGState *sp = (JPEGState *) cinfo;	
	char buffer[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message) (cinfo, buffer);
	TIFFErrorExt(sp->tif->tif_clientdata, "JPEGLib", "%s", buffer);		
	jpeg_abort(cinfo);			
	LONGJMP(sp->exit_jmpbuf, 1);		
}

static void
TIFFjpeg_output_message(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message) (cinfo, buffer);
	TIFFWarningExt(((JPEGState *) cinfo)->tif->tif_clientdata, "JPEGLib", "%s", buffer);
}

#define	CALLJPEG(sp, fail, op)	(SETJMP((sp)->exit_jmpbuf) ? (fail) : (op))
#define	CALLVJPEG(sp, op)	CALLJPEG(sp, 0, ((op),1))

static int
TIFFjpeg_create_compress(JPEGState* sp)
{
	
	sp->cinfo.c.err = jpeg_std_error(&sp->err);
	sp->err.error_exit = TIFFjpeg_error_exit;
	sp->err.output_message = TIFFjpeg_output_message;

	return CALLVJPEG(sp, jpeg_create_compress(&sp->cinfo.c));
}

static int
TIFFjpeg_create_decompress(JPEGState* sp)
{
	
	sp->cinfo.d.err = jpeg_std_error(&sp->err);
	sp->err.error_exit = TIFFjpeg_error_exit;
	sp->err.output_message = TIFFjpeg_output_message;

	return CALLVJPEG(sp, jpeg_create_decompress(&sp->cinfo.d));
}

static int
TIFFjpeg_set_defaults(JPEGState* sp)
{
	return CALLVJPEG(sp, jpeg_set_defaults(&sp->cinfo.c));
}

static int
TIFFjpeg_set_colorspace(JPEGState* sp, J_COLOR_SPACE colorspace)
{
	return CALLVJPEG(sp, jpeg_set_colorspace(&sp->cinfo.c, colorspace));
}

static int
TIFFjpeg_set_quality(JPEGState* sp, int quality, boolean force_baseline)
{
	return CALLVJPEG(sp,
	    jpeg_set_quality(&sp->cinfo.c, quality, force_baseline));
}

static int
TIFFjpeg_suppress_tables(JPEGState* sp, boolean suppress)
{
	return CALLVJPEG(sp, jpeg_suppress_tables(&sp->cinfo.c, suppress));
}

static int
TIFFjpeg_start_compress(JPEGState* sp, boolean write_all_tables)
{
	return CALLVJPEG(sp,
	    jpeg_start_compress(&sp->cinfo.c, write_all_tables));
}

static int
TIFFjpeg_write_scanlines(JPEGState* sp, JSAMPARRAY scanlines, int num_lines)
{
	return CALLJPEG(sp, -1, (int) jpeg_write_scanlines(&sp->cinfo.c,
	    scanlines, (JDIMENSION) num_lines));
}

static int
TIFFjpeg_write_raw_data(JPEGState* sp, JSAMPIMAGE data, int num_lines)
{
	return CALLJPEG(sp, -1, (int) jpeg_write_raw_data(&sp->cinfo.c,
	    data, (JDIMENSION) num_lines));
}

static int
TIFFjpeg_finish_compress(JPEGState* sp)
{
	return CALLVJPEG(sp, jpeg_finish_compress(&sp->cinfo.c));
}

static int
TIFFjpeg_write_tables(JPEGState* sp)
{
	return CALLVJPEG(sp, jpeg_write_tables(&sp->cinfo.c));
}

static int
TIFFjpeg_read_header(JPEGState* sp, boolean require_image)
{
	return CALLJPEG(sp, -1, jpeg_read_header(&sp->cinfo.d, require_image));
}

static int
TIFFjpeg_start_decompress(JPEGState* sp)
{
	return CALLVJPEG(sp, jpeg_start_decompress(&sp->cinfo.d));
}

static int
TIFFjpeg_read_scanlines(JPEGState* sp, JSAMPARRAY scanlines, int max_lines)
{
	return CALLJPEG(sp, -1, (int) jpeg_read_scanlines(&sp->cinfo.d,
	    scanlines, (JDIMENSION) max_lines));
}

static int
TIFFjpeg_read_raw_data(JPEGState* sp, JSAMPIMAGE data, int max_lines)
{
	return CALLJPEG(sp, -1, (int) jpeg_read_raw_data(&sp->cinfo.d,
	    data, (JDIMENSION) max_lines));
}

static int
TIFFjpeg_finish_decompress(JPEGState* sp)
{
	return CALLJPEG(sp, -1, (int) jpeg_finish_decompress(&sp->cinfo.d));
}

static int
TIFFjpeg_abort(JPEGState* sp)
{
	return CALLVJPEG(sp, jpeg_abort(&sp->cinfo.comm));
}

static int
TIFFjpeg_destroy(JPEGState* sp)
{
	return CALLVJPEG(sp, jpeg_destroy(&sp->cinfo.comm));
}

static JSAMPARRAY
TIFFjpeg_alloc_sarray(JPEGState* sp, int pool_id,
		      JDIMENSION samplesperrow, JDIMENSION numrows)
{
	return CALLJPEG(sp, (JSAMPARRAY) NULL,
	    (*sp->cinfo.comm.mem->alloc_sarray)
		(&sp->cinfo.comm, pool_id, samplesperrow, numrows));
}

static void
std_init_destination(j_compress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;
	TIFF* tif = sp->tif;

	sp->dest.next_output_byte = (JOCTET*) tif->tif_rawdata;
	sp->dest.free_in_buffer = (size_t) tif->tif_rawdatasize;
}

static boolean
std_empty_output_buffer(j_compress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;
	TIFF* tif = sp->tif;

	
	tif->tif_rawcc = tif->tif_rawdatasize;
	TIFFFlushData1(tif);
	sp->dest.next_output_byte = (JOCTET*) tif->tif_rawdata;
	sp->dest.free_in_buffer = (size_t) tif->tif_rawdatasize;

	return (TRUE);
}

static void
std_term_destination(j_compress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;
	TIFF* tif = sp->tif;

	tif->tif_rawcp = (tidata_t) sp->dest.next_output_byte;
	tif->tif_rawcc =
	    tif->tif_rawdatasize - (tsize_t) sp->dest.free_in_buffer;
	
}

static void
TIFFjpeg_data_dest(JPEGState* sp, TIFF* tif)
{
	(void) tif;
	sp->cinfo.c.dest = &sp->dest;
	sp->dest.init_destination = std_init_destination;
	sp->dest.empty_output_buffer = std_empty_output_buffer;
	sp->dest.term_destination = std_term_destination;
}

static void
tables_init_destination(j_compress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;

	
	sp->dest.next_output_byte = (JOCTET*) sp->jpegtables;
	sp->dest.free_in_buffer = (size_t) sp->jpegtables_length;
}

static boolean
tables_empty_output_buffer(j_compress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;
	void* newbuf;

	
	newbuf = _TIFFrealloc((tdata_t) sp->jpegtables,
			      (tsize_t) (sp->jpegtables_length + 1000));
	if (newbuf == NULL)
		ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 100);
	sp->dest.next_output_byte = (JOCTET*) newbuf + sp->jpegtables_length;
	sp->dest.free_in_buffer = (size_t) 1000;
	sp->jpegtables = newbuf;
	sp->jpegtables_length += 1000;
	return (TRUE);
}

static void
tables_term_destination(j_compress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;

	
	sp->jpegtables_length -= sp->dest.free_in_buffer;
}

static int
TIFFjpeg_tables_dest(JPEGState* sp, TIFF* tif)
{
	(void) tif;
	
	if (sp->jpegtables)
		_TIFFfree(sp->jpegtables);
	sp->jpegtables_length = 1000;
	sp->jpegtables = (void*) _TIFFmalloc((tsize_t) sp->jpegtables_length);
	if (sp->jpegtables == NULL) {
		sp->jpegtables_length = 0;
		TIFFErrorExt(sp->tif->tif_clientdata, "TIFFjpeg_tables_dest", "No space for JPEGTables");
		return (0);
	}
	sp->cinfo.c.dest = &sp->dest;
	sp->dest.init_destination = tables_init_destination;
	sp->dest.empty_output_buffer = tables_empty_output_buffer;
	sp->dest.term_destination = tables_term_destination;
	return (1);
}

static void
std_init_source(j_decompress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;
	TIFF* tif = sp->tif;

	sp->src.next_input_byte = (const JOCTET*) tif->tif_rawdata;
	sp->src.bytes_in_buffer = (size_t) tif->tif_rawcc;
}

static boolean
std_fill_input_buffer(j_decompress_ptr cinfo)
{
	JPEGState* sp = (JPEGState* ) cinfo;
	static const JOCTET dummy_EOI[2] = { 0xFF, JPEG_EOI };

	
	WARNMS(cinfo, JWRN_JPEG_EOF);
	
	sp->src.next_input_byte = dummy_EOI;
	sp->src.bytes_in_buffer = 2;
	return (TRUE);
}

static void
std_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	JPEGState* sp = (JPEGState*) cinfo;

	if (num_bytes > 0) {
		if (num_bytes > (long) sp->src.bytes_in_buffer) {
			
			(void) std_fill_input_buffer(cinfo);
		} else {
			sp->src.next_input_byte += (size_t) num_bytes;
			sp->src.bytes_in_buffer -= (size_t) num_bytes;
		}
	}
}

static void
std_term_source(j_decompress_ptr cinfo)
{
	
	
	
	(void) cinfo;
}

static void
TIFFjpeg_data_src(JPEGState* sp, TIFF* tif)
{
	(void) tif;
	sp->cinfo.d.src = &sp->src;
	sp->src.init_source = std_init_source;
	sp->src.fill_input_buffer = std_fill_input_buffer;
	sp->src.skip_input_data = std_skip_input_data;
	sp->src.resync_to_restart = jpeg_resync_to_restart;
	sp->src.term_source = std_term_source;
	sp->src.bytes_in_buffer = 0;		
	sp->src.next_input_byte = NULL;
}

static void
tables_init_source(j_decompress_ptr cinfo)
{
	JPEGState* sp = (JPEGState*) cinfo;

	sp->src.next_input_byte = (const JOCTET*) sp->jpegtables;
	sp->src.bytes_in_buffer = (size_t) sp->jpegtables_length;
}

static void
TIFFjpeg_tables_src(JPEGState* sp, TIFF* tif)
{
	TIFFjpeg_data_src(sp, tif);
	sp->src.init_source = tables_init_source;
}

static int
alloc_downsampled_buffers(TIFF* tif, jpeg_component_info* comp_info,
			  int num_components)
{
	JPEGState* sp = JState(tif);
	int ci;
	jpeg_component_info* compptr;
	JSAMPARRAY buf;
	int samples_per_clump = 0;

	for (ci = 0, compptr = comp_info; ci < num_components;
	     ci++, compptr++) {
		samples_per_clump += compptr->h_samp_factor *
			compptr->v_samp_factor;
		buf = TIFFjpeg_alloc_sarray(sp, JPOOL_IMAGE,
				compptr->width_in_blocks * DCTSIZE,
				(JDIMENSION) (compptr->v_samp_factor*DCTSIZE));
		if (buf == NULL)
			return (0);
		sp->ds_buffer[ci] = buf;
	}
	sp->samplesperclump = samples_per_clump;
	return (1);
}

static int
JPEGSetupDecode(TIFF* tif)
{
	JPEGState* sp = JState(tif);
	TIFFDirectory *td = &tif->tif_dir;

        JPEGInitializeLibJPEG( tif, 0, 1 );

	assert(sp != NULL);
	assert(sp->cinfo.comm.is_decompressor);

	
	if (TIFFFieldSet(tif,FIELD_JPEGTABLES)) {
		TIFFjpeg_tables_src(sp, tif);
		if(TIFFjpeg_read_header(sp,FALSE) != JPEG_HEADER_TABLES_ONLY) {
			TIFFErrorExt(tif->tif_clientdata, "JPEGSetupDecode", "Bogus JPEGTables field");
			return (0);
		}
	}

	
	sp->photometric = td->td_photometric;
	switch (sp->photometric) {
	case PHOTOMETRIC_YCBCR:
		sp->h_sampling = td->td_ycbcrsubsampling[0];
		sp->v_sampling = td->td_ycbcrsubsampling[1];
		break;
	default:
		
		sp->h_sampling = 1;
		sp->v_sampling = 1;
		break;
	}

	
	TIFFjpeg_data_src(sp, tif);
	tif->tif_postdecode = _TIFFNoPostDecode; 
	return (1);
}

static int
JPEGPreDecode(TIFF* tif, tsample_t s)
{
	JPEGState *sp = JState(tif);
	TIFFDirectory *td = &tif->tif_dir;
	static const char module[] = "JPEGPreDecode";
	uint32 segment_width, segment_height;
	int downsampled_output;
	int ci;

	assert(sp != NULL);
	assert(sp->cinfo.comm.is_decompressor);
	
	if (!TIFFjpeg_abort(sp))
		return (0);
	
	if (TIFFjpeg_read_header(sp, TRUE) != JPEG_HEADER_OK)
		return (0);
	
	segment_width = td->td_imagewidth;
	segment_height = td->td_imagelength - tif->tif_row;
	if (isTiled(tif)) {
                segment_width = td->td_tilewidth;
                segment_height = td->td_tilelength;
		sp->bytesperline = TIFFTileRowSize(tif);
	} else {
		if (segment_height > td->td_rowsperstrip)
			segment_height = td->td_rowsperstrip;
		sp->bytesperline = TIFFOldScanlineSize(tif);
	}
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE && s > 0) {
		
		segment_width = TIFFhowmany(segment_width, sp->h_sampling);
		segment_height = TIFFhowmany(segment_height, sp->v_sampling);
	}
	if (sp->cinfo.d.image_width < segment_width ||
	    sp->cinfo.d.image_height < segment_height) {
		TIFFWarningExt(tif->tif_clientdata, module,
			       "Improper JPEG strip/tile size, "
			       "expected %dx%d, got %dx%d",
			       segment_width, segment_height,
			       sp->cinfo.d.image_width,
			       sp->cinfo.d.image_height);
	} 
	if (sp->cinfo.d.image_width > segment_width ||
	    sp->cinfo.d.image_height > segment_height) {
		
		TIFFErrorExt(tif->tif_clientdata, module,
			     "JPEG strip/tile size exceeds expected dimensions,"
			     " expected %dx%d, got %dx%d",
			     segment_width, segment_height,
			     sp->cinfo.d.image_width, sp->cinfo.d.image_height);
		return (0);
	}
	if (sp->cinfo.d.num_components !=
	    (td->td_planarconfig == PLANARCONFIG_CONTIG ?
	     td->td_samplesperpixel : 1)) {
		TIFFErrorExt(tif->tif_clientdata, module, "Improper JPEG component count");
		return (0);
	}
#ifdef JPEG_LIB_MK1
	if (12 != td->td_bitspersample && 8 != td->td_bitspersample) {
			TIFFErrorExt(tif->tif_clientdata, module, "Improper JPEG data precision");
            return (0);
	}
        sp->cinfo.d.data_precision = td->td_bitspersample;
        sp->cinfo.d.bits_in_jsample = td->td_bitspersample;
#else
	if (sp->cinfo.d.data_precision != td->td_bitspersample) {
			TIFFErrorExt(tif->tif_clientdata, module, "Improper JPEG data precision");
            return (0);
	}
#endif
	if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
		
		if (sp->cinfo.d.comp_info[0].h_samp_factor != sp->h_sampling ||
		    sp->cinfo.d.comp_info[0].v_samp_factor != sp->v_sampling) {
				TIFFWarningExt(tif->tif_clientdata, module,
                                    "Improper JPEG sampling factors %d,%d\n"
                                    "Apparently should be %d,%d.",
                                    sp->cinfo.d.comp_info[0].h_samp_factor,
                                    sp->cinfo.d.comp_info[0].v_samp_factor,
                                    sp->h_sampling, sp->v_sampling);

				
				if (sp->cinfo.d.comp_info[0].h_samp_factor
					> sp->h_sampling
				    || sp->cinfo.d.comp_info[0].v_samp_factor
					> sp->v_sampling) {
					TIFFErrorExt(tif->tif_clientdata,
						     module,
					"Cannot honour JPEG sampling factors"
					" that exceed those specified.");
					return (0);
				}

			    
			    if (!_TIFFFindFieldInfo(tif, 33918, TIFF_ANY)) {
					TIFFWarningExt(tif->tif_clientdata, module,
					"Decompressor will try reading with "
					"sampling %d,%d.",
					sp->cinfo.d.comp_info[0].h_samp_factor,
					sp->cinfo.d.comp_info[0].v_samp_factor);

				    sp->h_sampling = (uint16)
					sp->cinfo.d.comp_info[0].h_samp_factor;
				    sp->v_sampling = (uint16)
					sp->cinfo.d.comp_info[0].v_samp_factor;
			    }
		}
		
		for (ci = 1; ci < sp->cinfo.d.num_components; ci++) {
			if (sp->cinfo.d.comp_info[ci].h_samp_factor != 1 ||
			    sp->cinfo.d.comp_info[ci].v_samp_factor != 1) {
				TIFFErrorExt(tif->tif_clientdata, module, "Improper JPEG sampling factors");
				return (0);
			}
		}
	} else {
		
		if (sp->cinfo.d.comp_info[0].h_samp_factor != 1 ||
		    sp->cinfo.d.comp_info[0].v_samp_factor != 1) {
			TIFFErrorExt(tif->tif_clientdata, module, "Improper JPEG sampling factors");
			return (0);
		}
	}
	downsampled_output = FALSE;
	if (td->td_planarconfig == PLANARCONFIG_CONTIG &&
	    sp->photometric == PHOTOMETRIC_YCBCR &&
	    sp->jpegcolormode == JPEGCOLORMODE_RGB) {
	
		sp->cinfo.d.jpeg_color_space = JCS_YCbCr;
		sp->cinfo.d.out_color_space = JCS_RGB;
	} else {
			
		sp->cinfo.d.jpeg_color_space = JCS_UNKNOWN;
		sp->cinfo.d.out_color_space = JCS_UNKNOWN;
		if (td->td_planarconfig == PLANARCONFIG_CONTIG &&
		    (sp->h_sampling != 1 || sp->v_sampling != 1))
			downsampled_output = TRUE;
		
	}
	if (downsampled_output) {
		
		sp->cinfo.d.raw_data_out = TRUE;
		tif->tif_decoderow = JPEGDecodeRaw;
		tif->tif_decodestrip = JPEGDecodeRaw;
		tif->tif_decodetile = JPEGDecodeRaw;
	} else {
		
		sp->cinfo.d.raw_data_out = FALSE;
		tif->tif_decoderow = JPEGDecode;
		tif->tif_decodestrip = JPEGDecode;
		tif->tif_decodetile = JPEGDecode;
	}
	
	if (!TIFFjpeg_start_decompress(sp))
		return (0);
	
	if (downsampled_output) {
		if (!alloc_downsampled_buffers(tif, sp->cinfo.d.comp_info,
					       sp->cinfo.d.num_components))
			return (0);
		sp->scancount = DCTSIZE;	
	}
	return (1);
}

 static int
JPEGDecode(TIFF* tif, tidata_t buf, tsize_t cc, tsample_t s)
{
    JPEGState *sp = JState(tif);
    tsize_t nrows;
    (void) s;

    nrows = cc / sp->bytesperline;
    if (cc % sp->bytesperline)
		TIFFWarningExt(tif->tif_clientdata, tif->tif_name, "fractional scanline not read");

    if( nrows > (int) sp->cinfo.d.image_height )
        nrows = sp->cinfo.d.image_height;

    
    if (nrows)
    {
        JSAMPROW line_work_buf = NULL;

        
#if !defined(JPEG_LIB_MK1)        
        if( sp->cinfo.d.data_precision == 12 )
#endif
        {
            line_work_buf = (JSAMPROW) 
                _TIFFmalloc(sizeof(short) * sp->cinfo.d.output_width 
                            * sp->cinfo.d.num_components );
        }

        do {
            if( line_work_buf != NULL )
            {
                
                if (TIFFjpeg_read_scanlines(sp, &line_work_buf, 1) != 1)
                    return (0);

                if( sp->cinfo.d.data_precision == 12 )
                {
                    int value_pairs = (sp->cinfo.d.output_width 
                                       * sp->cinfo.d.num_components) / 2;
                    int iPair;

                    for( iPair = 0; iPair < value_pairs; iPair++ )
                    {
                        unsigned char *out_ptr = 
                            ((unsigned char *) buf) + iPair * 3;
                        JSAMPLE *in_ptr = line_work_buf + iPair * 2;

                        out_ptr[0] = (in_ptr[0] & 0xff0) >> 4;
                        out_ptr[1] = ((in_ptr[0] & 0xf) << 4)
                            | ((in_ptr[1] & 0xf00) >> 8);
                        out_ptr[2] = ((in_ptr[1] & 0xff) >> 0);
                    }
                }
                else if( sp->cinfo.d.data_precision == 8 )
                {
                    int value_count = (sp->cinfo.d.output_width 
                                       * sp->cinfo.d.num_components);
                    int iValue;

                    for( iValue = 0; iValue < value_count; iValue++ )
                    {
                        ((unsigned char *) buf)[iValue] = 
                            line_work_buf[iValue] & 0xff;
                    }
                }
            }
            else
            {
                
                JSAMPROW bufptr = (JSAMPROW)buf;
  
                if (TIFFjpeg_read_scanlines(sp, &bufptr, 1) != 1)
                    return (0);
            }

            ++tif->tif_row;
            buf += sp->bytesperline;
            cc -= sp->bytesperline;
        } while (--nrows > 0);

        if( line_work_buf != NULL )
            _TIFFfree( line_work_buf );
    }

    
    return sp->cinfo.d.output_scanline < sp->cinfo.d.output_height
        || TIFFjpeg_finish_decompress(sp);
}

 static int
JPEGDecodeRaw(TIFF* tif, tidata_t buf, tsize_t cc, tsample_t s)
{
	JPEGState *sp = JState(tif);
	tsize_t nrows;
	(void) s;

	
	if ( (nrows = sp->cinfo.d.image_height) ) {
		
		JDIMENSION clumps_per_line = sp->cinfo.d.comp_info[1].downsampled_width;            
		int samples_per_clump = sp->samplesperclump;

#ifdef JPEG_LIB_MK1
		unsigned short* tmpbuf = _TIFFmalloc(sizeof(unsigned short) *
		    sp->cinfo.d.output_width *
		    sp->cinfo.d.num_components);
#endif

		do {
			jpeg_component_info *compptr;
			int ci, clumpoffset;

			
			if (sp->scancount >= DCTSIZE) {
				int n = sp->cinfo.d.max_v_samp_factor * DCTSIZE;
				if (TIFFjpeg_read_raw_data(sp, sp->ds_buffer, n) != n)
					return (0);
				sp->scancount = 0;
			}
			
			clumpoffset = 0;    
			for (ci = 0, compptr = sp->cinfo.d.comp_info;
			    ci < sp->cinfo.d.num_components;
			    ci++, compptr++) {
				int hsamp = compptr->h_samp_factor;
				int vsamp = compptr->v_samp_factor;
				int ypos;

				for (ypos = 0; ypos < vsamp; ypos++) {
					JSAMPLE *inptr = sp->ds_buffer[ci][sp->scancount*vsamp + ypos];
#ifdef JPEG_LIB_MK1
					JSAMPLE *outptr = (JSAMPLE*)tmpbuf + clumpoffset;
#else
					JSAMPLE *outptr = (JSAMPLE*)buf + clumpoffset;
#endif
					JDIMENSION nclump;

					if (hsamp == 1) {
						
						for (nclump = clumps_per_line; nclump-- > 0; ) {
							outptr[0] = *inptr++;
							outptr += samples_per_clump;
						}
					} else {
						int xpos;

			
						for (nclump = clumps_per_line; nclump-- > 0; ) {
							for (xpos = 0; xpos < hsamp; xpos++)
								outptr[xpos] = *inptr++;
							outptr += samples_per_clump;
						}
					}
					clumpoffset += hsamp;
				}
			}

#ifdef JPEG_LIB_MK1
			{
				if (sp->cinfo.d.data_precision == 8)
				{
					int i=0;
					int len = sp->cinfo.d.output_width * sp->cinfo.d.num_components;
					for (i=0; i<len; i++)
					{
						((unsigned char*)buf)[i] = tmpbuf[i] & 0xff;
					}
				}
				else
				{         
					int value_pairs = (sp->cinfo.d.output_width
					    * sp->cinfo.d.num_components) / 2;
					int iPair;
					for( iPair = 0; iPair < value_pairs; iPair++ )
					{
						unsigned char *out_ptr = ((unsigned char *) buf) + iPair * 3;
						JSAMPLE *in_ptr = tmpbuf + iPair * 2;
						out_ptr[0] = (in_ptr[0] & 0xff0) >> 4;
						out_ptr[1] = ((in_ptr[0] & 0xf) << 4)
						    | ((in_ptr[1] & 0xf00) >> 8);
						out_ptr[2] = ((in_ptr[1] & 0xff) >> 0);
					}
				}
			}
#endif

			sp->scancount ++;
			tif->tif_row += sp->v_sampling;
			
			buf += sp->bytesperline;
			cc -= sp->bytesperline;
			nrows -= sp->v_sampling;
		} while (nrows > 0);

#ifdef JPEG_LIB_MK1
		_TIFFfree(tmpbuf);
#endif

	}

	
	return sp->cinfo.d.output_scanline < sp->cinfo.d.output_height
	    || TIFFjpeg_finish_decompress(sp);
}

static void
unsuppress_quant_table (JPEGState* sp, int tblno)
{
	JQUANT_TBL* qtbl;

	if ((qtbl = sp->cinfo.c.quant_tbl_ptrs[tblno]) != NULL)
		qtbl->sent_table = FALSE;
}

static void
unsuppress_huff_table (JPEGState* sp, int tblno)
{
	JHUFF_TBL* htbl;

	if ((htbl = sp->cinfo.c.dc_huff_tbl_ptrs[tblno]) != NULL)
		htbl->sent_table = FALSE;
	if ((htbl = sp->cinfo.c.ac_huff_tbl_ptrs[tblno]) != NULL)
		htbl->sent_table = FALSE;
}

static int
prepare_JPEGTables(TIFF* tif)
{
	JPEGState* sp = JState(tif);

        JPEGInitializeLibJPEG( tif, 0, 0 );

	
	if (!TIFFjpeg_set_quality(sp, sp->jpegquality, FALSE))
		return (0);
	
	
	if (!TIFFjpeg_suppress_tables(sp, TRUE))
		return (0);
	if (sp->jpegtablesmode & JPEGTABLESMODE_QUANT) {
		unsuppress_quant_table(sp, 0);
		if (sp->photometric == PHOTOMETRIC_YCBCR)
			unsuppress_quant_table(sp, 1);
	}
	if (sp->jpegtablesmode & JPEGTABLESMODE_HUFF) {
		unsuppress_huff_table(sp, 0);
		if (sp->photometric == PHOTOMETRIC_YCBCR)
			unsuppress_huff_table(sp, 1);
	}
	
	if (!TIFFjpeg_tables_dest(sp, tif))
		return (0);
	
	if (!TIFFjpeg_write_tables(sp))
		return (0);

	return (1);
}

static int
JPEGSetupEncode(TIFF* tif)
{
	JPEGState* sp = JState(tif);
	TIFFDirectory *td = &tif->tif_dir;
	static const char module[] = "JPEGSetupEncode";

        JPEGInitializeLibJPEG( tif, 1, 0 );

	assert(sp != NULL);
	assert(!sp->cinfo.comm.is_decompressor);

	
	sp->cinfo.c.in_color_space = JCS_UNKNOWN;
	sp->cinfo.c.input_components = 1;
	if (!TIFFjpeg_set_defaults(sp))
		return (0);
	
	sp->photometric = td->td_photometric;
	switch (sp->photometric) {
	case PHOTOMETRIC_YCBCR:
		sp->h_sampling = td->td_ycbcrsubsampling[0];
		sp->v_sampling = td->td_ycbcrsubsampling[1];
		
		{
			float *ref;
			if (!TIFFGetField(tif, TIFFTAG_REFERENCEBLACKWHITE,
					  &ref)) {
				float refbw[6];
				long top = 1L << td->td_bitspersample;
				refbw[0] = 0;
				refbw[1] = (float)(top-1L);
				refbw[2] = (float)(top>>1);
				refbw[3] = refbw[1];
				refbw[4] = refbw[2];
				refbw[5] = refbw[1];
				TIFFSetField(tif, TIFFTAG_REFERENCEBLACKWHITE,
					     refbw);
			}
		}
		break;
	case PHOTOMETRIC_PALETTE:		
	case PHOTOMETRIC_MASK:
		TIFFErrorExt(tif->tif_clientdata, module,
			  "PhotometricInterpretation %d not allowed for JPEG",
			  (int) sp->photometric);
		return (0);
	default:
		
		sp->h_sampling = 1;
		sp->v_sampling = 1;
		break;
	}

	

	
#ifdef JPEG_LIB_MK1
        
	if (td->td_bitspersample != 8 && td->td_bitspersample != 12) 
#else
	if (td->td_bitspersample != BITS_IN_JSAMPLE )
#endif
	{
		TIFFErrorExt(tif->tif_clientdata, module, "BitsPerSample %d not allowed for JPEG",
			  (int) td->td_bitspersample);
		return (0);
	}
	sp->cinfo.c.data_precision = td->td_bitspersample;
#ifdef JPEG_LIB_MK1
        sp->cinfo.c.bits_in_jsample = td->td_bitspersample;
#endif
	if (isTiled(tif)) {
		if ((td->td_tilelength % (sp->v_sampling * DCTSIZE)) != 0) {
			TIFFErrorExt(tif->tif_clientdata, module,
				  "JPEG tile height must be multiple of %d",
				  sp->v_sampling * DCTSIZE);
			return (0);
		}
		if ((td->td_tilewidth % (sp->h_sampling * DCTSIZE)) != 0) {
			TIFFErrorExt(tif->tif_clientdata, module,
				  "JPEG tile width must be multiple of %d",
				  sp->h_sampling * DCTSIZE);
			return (0);
		}
	} else {
		if (td->td_rowsperstrip < td->td_imagelength &&
		    (td->td_rowsperstrip % (sp->v_sampling * DCTSIZE)) != 0) {
			TIFFErrorExt(tif->tif_clientdata, module,
				  "RowsPerStrip must be multiple of %d for JPEG",
				  sp->v_sampling * DCTSIZE);
			return (0);
		}
	}

	
	if (sp->jpegtablesmode & (JPEGTABLESMODE_QUANT|JPEGTABLESMODE_HUFF)) {
                if( sp->jpegtables == NULL
                    || memcmp(sp->jpegtables,"\0\0\0\0\0\0\0\0\0",8) == 0 )
                {
                        if (!prepare_JPEGTables(tif))
                                return (0);
                        
                        
                        tif->tif_flags |= TIFF_DIRTYDIRECT;
                        TIFFSetFieldBit(tif, FIELD_JPEGTABLES);
                }
	} else {
		
		
		TIFFClrFieldBit(tif, FIELD_JPEGTABLES);
	}

	
	TIFFjpeg_data_dest(sp, tif);

	return (1);
}

static int
JPEGPreEncode(TIFF* tif, tsample_t s)
{
	JPEGState *sp = JState(tif);
	TIFFDirectory *td = &tif->tif_dir;
	static const char module[] = "JPEGPreEncode";
	uint32 segment_width, segment_height;
	int downsampled_input;

	assert(sp != NULL);
	assert(!sp->cinfo.comm.is_decompressor);
	
	if (isTiled(tif)) {
		segment_width = td->td_tilewidth;
		segment_height = td->td_tilelength;
		sp->bytesperline = TIFFTileRowSize(tif);
	} else {
		segment_width = td->td_imagewidth;
		segment_height = td->td_imagelength - tif->tif_row;
		if (segment_height > td->td_rowsperstrip)
			segment_height = td->td_rowsperstrip;
		sp->bytesperline = TIFFOldScanlineSize(tif);
	}
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE && s > 0) {
		
		segment_width = TIFFhowmany(segment_width, sp->h_sampling);
		segment_height = TIFFhowmany(segment_height, sp->v_sampling);
	}
	if (segment_width > 65535 || segment_height > 65535) {
		TIFFErrorExt(tif->tif_clientdata, module, "Strip/tile too large for JPEG");
		return (0);
	}
	sp->cinfo.c.image_width = segment_width;
	sp->cinfo.c.image_height = segment_height;
	downsampled_input = FALSE;
	if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
		sp->cinfo.c.input_components = td->td_samplesperpixel;
		if (sp->photometric == PHOTOMETRIC_YCBCR) {
			if (sp->jpegcolormode == JPEGCOLORMODE_RGB) {
				sp->cinfo.c.in_color_space = JCS_RGB;
			} else {
				sp->cinfo.c.in_color_space = JCS_YCbCr;
				if (sp->h_sampling != 1 || sp->v_sampling != 1)
					downsampled_input = TRUE;
			}
			if (!TIFFjpeg_set_colorspace(sp, JCS_YCbCr))
				return (0);
			
			sp->cinfo.c.comp_info[0].h_samp_factor = sp->h_sampling;
			sp->cinfo.c.comp_info[0].v_samp_factor = sp->v_sampling;
		} else {
			sp->cinfo.c.in_color_space = JCS_UNKNOWN;
			if (!TIFFjpeg_set_colorspace(sp, JCS_UNKNOWN))
				return (0);
			
		}
	} else {
		sp->cinfo.c.input_components = 1;
		sp->cinfo.c.in_color_space = JCS_UNKNOWN;
		if (!TIFFjpeg_set_colorspace(sp, JCS_UNKNOWN))
			return (0);
		sp->cinfo.c.comp_info[0].component_id = s;
		
		if (sp->photometric == PHOTOMETRIC_YCBCR && s > 0) {
			sp->cinfo.c.comp_info[0].quant_tbl_no = 1;
			sp->cinfo.c.comp_info[0].dc_tbl_no = 1;
			sp->cinfo.c.comp_info[0].ac_tbl_no = 1;
		}
	}
	
	sp->cinfo.c.write_JFIF_header = FALSE;
	sp->cinfo.c.write_Adobe_marker = FALSE;
	
        if (!TIFFjpeg_set_quality(sp, sp->jpegquality, FALSE))
                return (0);
	if (! (sp->jpegtablesmode & JPEGTABLESMODE_QUANT)) {
		unsuppress_quant_table(sp, 0);
		unsuppress_quant_table(sp, 1);
	}
	if (sp->jpegtablesmode & JPEGTABLESMODE_HUFF)
		sp->cinfo.c.optimize_coding = FALSE;
	else
		sp->cinfo.c.optimize_coding = TRUE;
	if (downsampled_input) {
		
		sp->cinfo.c.raw_data_in = TRUE;
		tif->tif_encoderow = JPEGEncodeRaw;
		tif->tif_encodestrip = JPEGEncodeRaw;
		tif->tif_encodetile = JPEGEncodeRaw;
	} else {
		
		sp->cinfo.c.raw_data_in = FALSE;
		tif->tif_encoderow = JPEGEncode;
		tif->tif_encodestrip = JPEGEncode;
		tif->tif_encodetile = JPEGEncode;
	}
	
	if (!TIFFjpeg_start_compress(sp, FALSE))
		return (0);
	
	if (downsampled_input) {
		if (!alloc_downsampled_buffers(tif, sp->cinfo.c.comp_info,
					       sp->cinfo.c.num_components))
			return (0);
	}
	sp->scancount = 0;

	return (1);
}

static int
JPEGEncode(TIFF* tif, tidata_t buf, tsize_t cc, tsample_t s)
{
	JPEGState *sp = JState(tif);
	tsize_t nrows;
	JSAMPROW bufptr[1];

	(void) s;
	assert(sp != NULL);
	
	nrows = cc / sp->bytesperline;
	if (cc % sp->bytesperline)
		TIFFWarningExt(tif->tif_clientdata, tif->tif_name, "fractional scanline discarded");

        
        if( !isTiled(tif) && tif->tif_row+nrows > tif->tif_dir.td_imagelength )
            nrows = tif->tif_dir.td_imagelength - tif->tif_row;

	while (nrows-- > 0) {
		bufptr[0] = (JSAMPROW) buf;
		if (TIFFjpeg_write_scanlines(sp, bufptr, 1) != 1)
			return (0);
		if (nrows > 0)
			tif->tif_row++;
		buf += sp->bytesperline;
	}
	return (1);
}

static int
JPEGEncodeRaw(TIFF* tif, tidata_t buf, tsize_t cc, tsample_t s)
{
	JPEGState *sp = JState(tif);
	JSAMPLE* inptr;
	JSAMPLE* outptr;
	tsize_t nrows;
	JDIMENSION clumps_per_line, nclump;
	int clumpoffset, ci, xpos, ypos;
	jpeg_component_info* compptr;
	int samples_per_clump = sp->samplesperclump;
	tsize_t bytesperclumpline;

	(void) s;
	assert(sp != NULL);
	
	
	
	bytesperclumpline = (((sp->cinfo.c.image_width+sp->h_sampling-1)/sp->h_sampling)
			     *(sp->h_sampling*sp->v_sampling+2)*sp->cinfo.c.data_precision+7)
			    /8;

	nrows = ( cc / bytesperclumpline ) * sp->v_sampling;
	if (cc % bytesperclumpline)
		TIFFWarningExt(tif->tif_clientdata, tif->tif_name, "fractional scanline discarded");

	
	clumps_per_line = sp->cinfo.c.comp_info[1].downsampled_width;

	while (nrows > 0) {
		
		clumpoffset = 0;		
		for (ci = 0, compptr = sp->cinfo.c.comp_info;
		     ci < sp->cinfo.c.num_components;
		     ci++, compptr++) {
		    int hsamp = compptr->h_samp_factor;
		    int vsamp = compptr->v_samp_factor;
		    int padding = (int) (compptr->width_in_blocks * DCTSIZE -
					 clumps_per_line * hsamp);
		    for (ypos = 0; ypos < vsamp; ypos++) {
			inptr = ((JSAMPLE*) buf) + clumpoffset;
			outptr = sp->ds_buffer[ci][sp->scancount*vsamp + ypos];
			if (hsamp == 1) {
			    
			    for (nclump = clumps_per_line; nclump-- > 0; ) {
				*outptr++ = inptr[0];
				inptr += samples_per_clump;
			    }
			} else {
			    
			    for (nclump = clumps_per_line; nclump-- > 0; ) {
				for (xpos = 0; xpos < hsamp; xpos++)
				    *outptr++ = inptr[xpos];
				inptr += samples_per_clump;
			    }
			}
			
			for (xpos = 0; xpos < padding; xpos++) {
			    *outptr = outptr[-1];
			    outptr++;
			}
			clumpoffset += hsamp;
		    }
		}
		sp->scancount++;
		if (sp->scancount >= DCTSIZE) {
			int n = sp->cinfo.c.max_v_samp_factor * DCTSIZE;
			if (TIFFjpeg_write_raw_data(sp, sp->ds_buffer, n) != n)
				return (0);
			sp->scancount = 0;
		}
		tif->tif_row += sp->v_sampling;
		buf += sp->bytesperline;
		nrows -= sp->v_sampling;
	}
	return (1);
}

static int
JPEGPostEncode(TIFF* tif)
{
	JPEGState *sp = JState(tif);

	if (sp->scancount > 0) {
		
		int ci, ypos, n;
		jpeg_component_info* compptr;

		for (ci = 0, compptr = sp->cinfo.c.comp_info;
		     ci < sp->cinfo.c.num_components;
		     ci++, compptr++) {
			int vsamp = compptr->v_samp_factor;
			tsize_t row_width = compptr->width_in_blocks * DCTSIZE
				* sizeof(JSAMPLE);
			for (ypos = sp->scancount * vsamp;
			     ypos < DCTSIZE * vsamp; ypos++) {
				_TIFFmemcpy((tdata_t)sp->ds_buffer[ci][ypos],
					    (tdata_t)sp->ds_buffer[ci][ypos-1],
					    row_width);

			}
		}
		n = sp->cinfo.c.max_v_samp_factor * DCTSIZE;
		if (TIFFjpeg_write_raw_data(sp, sp->ds_buffer, n) != n)
			return (0);
	}

	return (TIFFjpeg_finish_compress(JState(tif)));
}

static void
JPEGCleanup(TIFF* tif)
{
	JPEGState *sp = JState(tif);
	
	assert(sp != 0);

	tif->tif_tagmethods.vgetfield = sp->vgetparent;
	tif->tif_tagmethods.vsetfield = sp->vsetparent;
	tif->tif_tagmethods.printdir = sp->printdir;

	if( sp->cinfo_initialized )
	    TIFFjpeg_destroy(sp);	
	if (sp->jpegtables)		
		_TIFFfree(sp->jpegtables);
	_TIFFfree(tif->tif_data);	
	tif->tif_data = NULL;

	_TIFFSetDefaultCompressionState(tif);
}

static void 
JPEGResetUpsampled( TIFF* tif )
{
	JPEGState* sp = JState(tif);
	TIFFDirectory* td = &tif->tif_dir;

	
	tif->tif_flags &= ~TIFF_UPSAMPLED;
	if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
		if (td->td_photometric == PHOTOMETRIC_YCBCR &&
		    sp->jpegcolormode == JPEGCOLORMODE_RGB) {
			tif->tif_flags |= TIFF_UPSAMPLED;
		} else {
#ifdef notdef
			if (td->td_ycbcrsubsampling[0] != 1 ||
			    td->td_ycbcrsubsampling[1] != 1)
				; 
#endif
		}
	}

	
        if( tif->tif_tilesize > 0 )
            tif->tif_tilesize = isTiled(tif) ? TIFFTileSize(tif) : (tsize_t) -1;

        if(tif->tif_scanlinesize > 0 )
            tif->tif_scanlinesize = TIFFScanlineSize(tif); 
}

static int
JPEGVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
	JPEGState* sp = JState(tif);
	const TIFFFieldInfo* fip;
	uint32 v32;

	assert(sp != NULL);

	switch (tag) {
	case TIFFTAG_JPEGTABLES:
		v32 = va_arg(ap, uint32);
		if (v32 == 0) {
			
			return (0);
		}
		_TIFFsetByteArray(&sp->jpegtables, va_arg(ap, void*),
		    (long) v32);
		sp->jpegtables_length = v32;
		TIFFSetFieldBit(tif, FIELD_JPEGTABLES);
		break;
	case TIFFTAG_JPEGQUALITY:
		sp->jpegquality = va_arg(ap, int);
		return (1);			
	case TIFFTAG_JPEGCOLORMODE:
		sp->jpegcolormode = va_arg(ap, int);
                JPEGResetUpsampled( tif );
		return (1);			
	case TIFFTAG_PHOTOMETRIC:
        {
                int ret_value = (*sp->vsetparent)(tif, tag, ap);
                JPEGResetUpsampled( tif );
                return ret_value;
        }
	case TIFFTAG_JPEGTABLESMODE:
		sp->jpegtablesmode = va_arg(ap, int);
		return (1);			
	case TIFFTAG_YCBCRSUBSAMPLING:
                
		sp->ycbcrsampling_fetched = 1;
                
		return (*sp->vsetparent)(tif, tag, ap);
	case TIFFTAG_FAXRECVPARAMS:
		sp->recvparams = va_arg(ap, uint32);
		break;
	case TIFFTAG_FAXSUBADDRESS:
		_TIFFsetString(&sp->subaddress, va_arg(ap, char*));
		break;
	case TIFFTAG_FAXRECVTIME:
		sp->recvtime = va_arg(ap, uint32);
		break;
	case TIFFTAG_FAXDCS:
		_TIFFsetString(&sp->faxdcs, va_arg(ap, char*));
		break;
	default:
		return (*sp->vsetparent)(tif, tag, ap);
	}

	if ((fip = _TIFFFieldWithTag(tif, tag))) {
		TIFFSetFieldBit(tif, fip->field_bit);
	} else {
		return (0);
	}

	tif->tif_flags |= TIFF_DIRTYDIRECT;
	return (1);
}

static void 
JPEGFixupTestSubsampling( TIFF * tif )
{
#ifdef CHECK_JPEG_YCBCR_SUBSAMPLING
    JPEGState *sp = JState(tif);
    TIFFDirectory *td = &tif->tif_dir;

    JPEGInitializeLibJPEG( tif, 0, 0 );

    
    if( !sp->cinfo.comm.is_decompressor 
        || sp->ycbcrsampling_fetched  
        || td->td_photometric != PHOTOMETRIC_YCBCR )
        return;

    sp->ycbcrsampling_fetched = 1;
    if( TIFFIsTiled( tif ) )
    {
        if( !TIFFFillTile( tif, 0 ) )
			return;
    }
    else
    {
        if( !TIFFFillStrip( tif, 0 ) )
            return;
    }

    TIFFSetField( tif, TIFFTAG_YCBCRSUBSAMPLING, 
                  (uint16) sp->h_sampling, (uint16) sp->v_sampling );

    
    tif->tif_curstrip = -1;

#endif 
}

static int
JPEGVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
	JPEGState* sp = JState(tif);

	assert(sp != NULL);

	switch (tag) {
		case TIFFTAG_JPEGTABLES:
			*va_arg(ap, uint32*) = sp->jpegtables_length;
			*va_arg(ap, void**) = sp->jpegtables;
			break;
		case TIFFTAG_JPEGQUALITY:
			*va_arg(ap, int*) = sp->jpegquality;
			break;
		case TIFFTAG_JPEGCOLORMODE:
			*va_arg(ap, int*) = sp->jpegcolormode;
			break;
		case TIFFTAG_JPEGTABLESMODE:
			*va_arg(ap, int*) = sp->jpegtablesmode;
			break;
		case TIFFTAG_YCBCRSUBSAMPLING:
			JPEGFixupTestSubsampling( tif );
			return (*sp->vgetparent)(tif, tag, ap);
		case TIFFTAG_FAXRECVPARAMS:
			*va_arg(ap, uint32*) = sp->recvparams;
			break;
		case TIFFTAG_FAXSUBADDRESS:
			*va_arg(ap, char**) = sp->subaddress;
			break;
		case TIFFTAG_FAXRECVTIME:
			*va_arg(ap, uint32*) = sp->recvtime;
			break;
		case TIFFTAG_FAXDCS:
			*va_arg(ap, char**) = sp->faxdcs;
			break;
		default:
			return (*sp->vgetparent)(tif, tag, ap);
	}
	return (1);
}

static void
JPEGPrintDir(TIFF* tif, FILE* fd, long flags)
{
	JPEGState* sp = JState(tif);

	assert(sp != NULL);

	(void) flags;
	if (TIFFFieldSet(tif,FIELD_JPEGTABLES))
		fprintf(fd, "  JPEG Tables: (%lu bytes)\n",
			(unsigned long) sp->jpegtables_length);
        if (TIFFFieldSet(tif,FIELD_RECVPARAMS))
                fprintf(fd, "  Fax Receive Parameters: %08lx\n",
                   (unsigned long) sp->recvparams);
        if (TIFFFieldSet(tif,FIELD_SUBADDRESS))
                fprintf(fd, "  Fax SubAddress: %s\n", sp->subaddress);
        if (TIFFFieldSet(tif,FIELD_RECVTIME))
                fprintf(fd, "  Fax Receive Time: %lu secs\n",
                    (unsigned long) sp->recvtime);
        if (TIFFFieldSet(tif,FIELD_FAXDCS))
                fprintf(fd, "  Fax DCS: %s\n", sp->faxdcs);
}

static uint32
JPEGDefaultStripSize(TIFF* tif, uint32 s)
{
	JPEGState* sp = JState(tif);
	TIFFDirectory *td = &tif->tif_dir;

	s = (*sp->defsparent)(tif, s);
	if (s < td->td_imagelength)
		s = TIFFroundup(s, td->td_ycbcrsubsampling[1] * DCTSIZE);
	return (s);
}

static void
JPEGDefaultTileSize(TIFF* tif, uint32* tw, uint32* th)
{
	JPEGState* sp = JState(tif);
	TIFFDirectory *td = &tif->tif_dir;

	(*sp->deftparent)(tif, tw, th);
	*tw = TIFFroundup(*tw, td->td_ycbcrsubsampling[0] * DCTSIZE);
	*th = TIFFroundup(*th, td->td_ycbcrsubsampling[1] * DCTSIZE);
}

static int JPEGInitializeLibJPEG( TIFF * tif, int force_encode, int force_decode )
{
    JPEGState* sp = JState(tif);
    uint32 *byte_counts = NULL;
    int     data_is_empty = TRUE;
    int     decompress;

    if(sp->cinfo_initialized)
    {
        if( force_encode && sp->cinfo.comm.is_decompressor )
            TIFFjpeg_destroy( sp );
        else if( force_decode && !sp->cinfo.comm.is_decompressor )
            TIFFjpeg_destroy( sp );
        else
            return 1;

        sp->cinfo_initialized = 0;
    }

    
    if( TIFFIsTiled( tif ) 
        && TIFFGetField( tif, TIFFTAG_TILEBYTECOUNTS, &byte_counts ) 
        && byte_counts != NULL )
    {
        data_is_empty = byte_counts[0] == 0;
    }
    if( !TIFFIsTiled( tif ) 
        && TIFFGetField( tif, TIFFTAG_STRIPBYTECOUNTS, &byte_counts) 
        && byte_counts != NULL )
    {
        data_is_empty = byte_counts[0] == 0;
    }

    if( force_decode )
        decompress = 1;
    else if( force_encode )
        decompress = 0;
    else if( tif->tif_mode == O_RDONLY )
        decompress = 1;
    else if( data_is_empty )
        decompress = 0;
    else
        decompress = 1;

    
    if ( decompress ) {
        if (!TIFFjpeg_create_decompress(sp))
            return (0);

    } else {
        if (!TIFFjpeg_create_compress(sp))
            return (0);
    }

    sp->cinfo_initialized = TRUE;

    return 1;
}

int
TIFFInitJPEG(TIFF* tif, int scheme)
{
	JPEGState* sp;

	assert(scheme == COMPRESSION_JPEG);

	
	if (!_TIFFMergeFieldInfo(tif, jpegFieldInfo, N(jpegFieldInfo))) {
		TIFFErrorExt(tif->tif_clientdata,
			     "TIFFInitJPEG",
			     "Merging JPEG codec-specific tags failed");
		return 0;
	}

	
	tif->tif_data = (tidata_t) _TIFFmalloc(sizeof (JPEGState));

	if (tif->tif_data == NULL) {
		TIFFErrorExt(tif->tif_clientdata,
			     "TIFFInitJPEG", "No space for JPEG state block");
		return 0;
	}
        _TIFFmemset(tif->tif_data, 0, sizeof(JPEGState));

	sp = JState(tif);
	sp->tif = tif;				

	
	sp->vgetparent = tif->tif_tagmethods.vgetfield;
	tif->tif_tagmethods.vgetfield = JPEGVGetField; 
	sp->vsetparent = tif->tif_tagmethods.vsetfield;
	tif->tif_tagmethods.vsetfield = JPEGVSetField; 
	sp->printdir = tif->tif_tagmethods.printdir;
	tif->tif_tagmethods.printdir = JPEGPrintDir;   

	
	sp->jpegtables = NULL;
	sp->jpegtables_length = 0;
	sp->jpegquality = 75;			
	sp->jpegcolormode = JPEGCOLORMODE_RAW;
	sp->jpegtablesmode = JPEGTABLESMODE_QUANT | JPEGTABLESMODE_HUFF;

        sp->recvparams = 0;
        sp->subaddress = NULL;
        sp->faxdcs = NULL;

        sp->ycbcrsampling_fetched = 0;

	
	tif->tif_setupdecode = JPEGSetupDecode;
	tif->tif_predecode = JPEGPreDecode;
	tif->tif_decoderow = JPEGDecode;
	tif->tif_decodestrip = JPEGDecode;
	tif->tif_decodetile = JPEGDecode;
	tif->tif_setupencode = JPEGSetupEncode;
	tif->tif_preencode = JPEGPreEncode;
	tif->tif_postencode = JPEGPostEncode;
	tif->tif_encoderow = JPEGEncode;
	tif->tif_encodestrip = JPEGEncode;
	tif->tif_encodetile = JPEGEncode;
	tif->tif_cleanup = JPEGCleanup;
	sp->defsparent = tif->tif_defstripsize;
	tif->tif_defstripsize = JPEGDefaultStripSize;
	sp->deftparent = tif->tif_deftilesize;
	tif->tif_deftilesize = JPEGDefaultTileSize;
	tif->tif_flags |= TIFF_NOBITREV;	

        sp->cinfo_initialized = FALSE;

	
        if( tif->tif_diroff == 0 )
        {
#define SIZE_OF_JPEGTABLES 2000

            sp->jpegtables_length = SIZE_OF_JPEGTABLES;
            sp->jpegtables = (void *) _TIFFmalloc(sp->jpegtables_length);
	    _TIFFmemset(sp->jpegtables, 0, SIZE_OF_JPEGTABLES);
#undef SIZE_OF_JPEGTABLES
        }

        
        TIFFSetFieldBit( tif, FIELD_YCBCRSUBSAMPLING );

	return 1;
}
#endif 

