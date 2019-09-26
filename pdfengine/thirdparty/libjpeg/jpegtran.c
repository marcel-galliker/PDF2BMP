

#include "cdjpeg.h"		
#include "transupp.h"		
#include "jversion.h"		

#ifdef USE_CCOMMAND		
#ifdef __MWERKS__
#include <SIOUX.h>              
#include <console.h>		
#endif
#ifdef THINK_C
#include <console.h>		
#endif
#endif

static const char * progname;	
static char * outfilename;	
static char * scaleoption;	
static JCOPY_OPTION copyoption;	
static jpeg_transform_info transformoption; 

LOCAL(void)
usage (void)

{
  fprintf(stderr, "usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
  fprintf(stderr, "inputfile outputfile\n");
#else
  fprintf(stderr, "[inputfile]\n");
#endif

  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -copy none     Copy no extra markers from source file\n");
  fprintf(stderr, "  -copy comments Copy only comment markers (default)\n");
  fprintf(stderr, "  -copy all      Copy all extra markers\n");
#ifdef ENTROPY_OPT_SUPPORTED
  fprintf(stderr, "  -optimize      Optimize Huffman table (smaller file, but slow compression)\n");
#endif
#ifdef C_PROGRESSIVE_SUPPORTED
  fprintf(stderr, "  -progressive   Create progressive JPEG file\n");
#endif
  fprintf(stderr, "Switches for modifying the image:\n");
#if TRANSFORMS_SUPPORTED
  fprintf(stderr, "  -crop WxH+X+Y  Crop to a rectangular subarea\n");
  fprintf(stderr, "  -grayscale     Reduce to grayscale (omit color data)\n");
  fprintf(stderr, "  -flip [horizontal|vertical]  Mirror image (left-right or top-bottom)\n");
  fprintf(stderr, "  -perfect       Fail if there is non-transformable edge blocks\n");
  fprintf(stderr, "  -rotate [90|180|270]         Rotate image (degrees clockwise)\n");
#endif
  fprintf(stderr, "  -scale M/N     Scale output image by fraction M/N, eg, 1/8\n");
#if TRANSFORMS_SUPPORTED
  fprintf(stderr, "  -transpose     Transpose image\n");
  fprintf(stderr, "  -transverse    Transverse transpose image\n");
  fprintf(stderr, "  -trim          Drop non-transformable edge blocks\n");
#endif
  fprintf(stderr, "Switches for advanced users:\n");
#ifdef C_ARITH_CODING_SUPPORTED
  fprintf(stderr, "  -arithmetic    Use arithmetic coding\n");
#endif
  fprintf(stderr, "  -restart N     Set restart interval in rows, or in blocks with B\n");
  fprintf(stderr, "  -maxmemory N   Maximum memory to use (in kbytes)\n");
  fprintf(stderr, "  -outfile name  Specify name for output file\n");
  fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
  fprintf(stderr, "Switches for wizards:\n");
#ifdef C_MULTISCAN_FILES_SUPPORTED
  fprintf(stderr, "  -scans file    Create multi-scan JPEG per script file\n");
#endif
  exit(EXIT_FAILURE);
}

LOCAL(void)
select_transform (JXFORM_CODE transform)

{
#if TRANSFORMS_SUPPORTED
  if (transformoption.transform == JXFORM_NONE ||
      transformoption.transform == transform) {
    transformoption.transform = transform;
  } else {
    fprintf(stderr, "%s: can only do one image transformation at a time\n",
	    progname);
    usage();
  }
#else
  fprintf(stderr, "%s: sorry, image transformation was not compiled\n",
	  progname);
  exit(EXIT_FAILURE);
#endif
}

LOCAL(int)
parse_switches (j_compress_ptr cinfo, int argc, char **argv,
		int last_file_arg_seen, boolean for_real)

{
  int argn;
  char * arg;
  boolean simple_progressive;
  char * scansarg = NULL;	

  
  simple_progressive = FALSE;
  outfilename = NULL;
  scaleoption = NULL;
  copyoption = JCOPYOPT_DEFAULT;
  transformoption.transform = JXFORM_NONE;
  transformoption.perfect = FALSE;
  transformoption.trim = FALSE;
  transformoption.force_grayscale = FALSE;
  transformoption.crop = FALSE;
  cinfo->err->trace_level = 0;

  

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (*arg != '-') {
      
      if (argn <= last_file_arg_seen) {
	outfilename = NULL;	
	continue;		
      }
      break;			
    }
    arg++;			

    if (keymatch(arg, "arithmetic", 1)) {
      
#ifdef C_ARITH_CODING_SUPPORTED
      cinfo->arith_code = TRUE;
#else
      fprintf(stderr, "%s: sorry, arithmetic coding not supported\n",
	      progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "copy", 2)) {
      
      if (++argn >= argc)	
	usage();
      if (keymatch(argv[argn], "none", 1)) {
	copyoption = JCOPYOPT_NONE;
      } else if (keymatch(argv[argn], "comments", 1)) {
	copyoption = JCOPYOPT_COMMENTS;
      } else if (keymatch(argv[argn], "all", 1)) {
	copyoption = JCOPYOPT_ALL;
      } else
	usage();

    } else if (keymatch(arg, "crop", 2)) {
      
#if TRANSFORMS_SUPPORTED
      if (++argn >= argc)	
	usage();
      if (! jtransform_parse_crop_spec(&transformoption, argv[argn])) {
	fprintf(stderr, "%s: bogus -crop argument '%s'\n",
		progname, argv[argn]);
	exit(EXIT_FAILURE);
      }
#else
      select_transform(JXFORM_NONE);	
#endif

    } else if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
      
      
      static boolean printed_version = FALSE;

      if (! printed_version) {
	fprintf(stderr, "Independent JPEG Group's JPEGTRAN, version %s\n%s\n",
		JVERSION, JCOPYRIGHT);
	printed_version = TRUE;
      }
      cinfo->err->trace_level++;

    } else if (keymatch(arg, "flip", 1)) {
      
      if (++argn >= argc)	
	usage();
      if (keymatch(argv[argn], "horizontal", 1))
	select_transform(JXFORM_FLIP_H);
      else if (keymatch(argv[argn], "vertical", 1))
	select_transform(JXFORM_FLIP_V);
      else
	usage();

    } else if (keymatch(arg, "grayscale", 1) || keymatch(arg, "greyscale",1)) {
      
#if TRANSFORMS_SUPPORTED
      transformoption.force_grayscale = TRUE;
#else
      select_transform(JXFORM_NONE);	
#endif

    } else if (keymatch(arg, "maxmemory", 3)) {
      
      long lval;
      char ch = 'x';

      if (++argn >= argc)	
	usage();
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
	usage();
      if (ch == 'm' || ch == 'M')
	lval *= 1000L;
      cinfo->mem->max_memory_to_use = lval * 1000L;

    } else if (keymatch(arg, "optimize", 1) || keymatch(arg, "optimise", 1)) {
      
#ifdef ENTROPY_OPT_SUPPORTED
      cinfo->optimize_coding = TRUE;
#else
      fprintf(stderr, "%s: sorry, entropy optimization was not compiled\n",
	      progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "outfile", 4)) {
      
      if (++argn >= argc)	
	usage();
      outfilename = argv[argn];	

    } else if (keymatch(arg, "perfect", 2)) {
      
      transformoption.perfect = TRUE;

    } else if (keymatch(arg, "progressive", 2)) {
      
#ifdef C_PROGRESSIVE_SUPPORTED
      simple_progressive = TRUE;
      
#else
      fprintf(stderr, "%s: sorry, progressive output was not compiled\n",
	      progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "restart", 1)) {
      
      long lval;
      char ch = 'x';

      if (++argn >= argc)	
	usage();
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
	usage();
      if (lval < 0 || lval > 65535L)
	usage();
      if (ch == 'b' || ch == 'B') {
	cinfo->restart_interval = (unsigned int) lval;
	cinfo->restart_in_rows = 0; 
      } else {
	cinfo->restart_in_rows = (int) lval;
	
      }

    } else if (keymatch(arg, "rotate", 2)) {
      
      if (++argn >= argc)	
	usage();
      if (keymatch(argv[argn], "90", 2))
	select_transform(JXFORM_ROT_90);
      else if (keymatch(argv[argn], "180", 3))
	select_transform(JXFORM_ROT_180);
      else if (keymatch(argv[argn], "270", 3))
	select_transform(JXFORM_ROT_270);
      else
	usage();

    } else if (keymatch(arg, "scale", 4)) {
      
      if (++argn >= argc)	
	usage();
      scaleoption = argv[argn];
      

    } else if (keymatch(arg, "scans", 1)) {
      
#ifdef C_MULTISCAN_FILES_SUPPORTED
      if (++argn >= argc)	
	usage();
      scansarg = argv[argn];
      
#else
      fprintf(stderr, "%s: sorry, multi-scan output was not compiled\n",
	      progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "transpose", 1)) {
      
      select_transform(JXFORM_TRANSPOSE);

    } else if (keymatch(arg, "transverse", 6)) {
      
      select_transform(JXFORM_TRANSVERSE);

    } else if (keymatch(arg, "trim", 3)) {
      
      transformoption.trim = TRUE;

    } else {
      usage();			
    }
  }

  

  if (for_real) {

#ifdef C_PROGRESSIVE_SUPPORTED
    if (simple_progressive)	
      jpeg_simple_progression(cinfo);
#endif

#ifdef C_MULTISCAN_FILES_SUPPORTED
    if (scansarg != NULL)	
      if (! read_scan_script(cinfo, scansarg))
	usage();
#endif
  }

  return argn;			
}

int
main (int argc, char **argv)
{
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jsrcerr, jdsterr;
#ifdef PROGRESS_REPORT
  struct cdjpeg_progress_mgr progress;
#endif
  jvirt_barray_ptr * src_coef_arrays;
  jvirt_barray_ptr * dst_coef_arrays;
  int file_index;
  
  FILE * fp;

  
#ifdef USE_CCOMMAND
  argc = ccommand(&argv);
#endif

  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "jpegtran";	

  
  srcinfo.err = jpeg_std_error(&jsrcerr);
  jpeg_create_decompress(&srcinfo);
  
  dstinfo.err = jpeg_std_error(&jdsterr);
  jpeg_create_compress(&dstinfo);

  
#ifdef NEED_SIGNAL_CATCHER
  enable_signal_catcher((j_common_ptr) &srcinfo);
#endif

  

  file_index = parse_switches(&dstinfo, argc, argv, 0, FALSE);
  jsrcerr.trace_level = jdsterr.trace_level;
  srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

#ifdef TWO_FILE_COMMANDLINE
  
  if (outfilename == NULL) {
    if (file_index != argc-2) {
      fprintf(stderr, "%s: must name one input and one output file\n",
	      progname);
      usage();
    }
    outfilename = argv[file_index+1];
  } else {
    if (file_index != argc-1) {
      fprintf(stderr, "%s: must name one input and one output file\n",
	      progname);
      usage();
    }
  }
#else
  
  if (file_index < argc-1) {
    fprintf(stderr, "%s: only one input file\n", progname);
    usage();
  }
#endif 

  
  if (file_index < argc) {
    if ((fp = fopen(argv[file_index], READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s for reading\n", progname, argv[file_index]);
      exit(EXIT_FAILURE);
    }
  } else {
    
    fp = read_stdin();
  }

#ifdef PROGRESS_REPORT
  start_progress_monitor((j_common_ptr) &dstinfo, &progress);
#endif

  
  jpeg_stdio_src(&srcinfo, fp);

  
  jcopy_markers_setup(&srcinfo, copyoption);

  
  (void) jpeg_read_header(&srcinfo, TRUE);

  
  if (scaleoption != NULL)
    if (sscanf(scaleoption, "%d/%d",
	&srcinfo.scale_num, &srcinfo.scale_denom) < 1)
      usage();

  
#if TRANSFORMS_SUPPORTED
  
  if (!jtransform_request_workspace(&srcinfo, &transformoption)) {
    fprintf(stderr, "%s: transformation is not perfect\n", progname);
    exit(EXIT_FAILURE);
  }
#endif

  
  src_coef_arrays = jpeg_read_coefficients(&srcinfo);

  
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  
#if TRANSFORMS_SUPPORTED
  dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
						 src_coef_arrays,
						 &transformoption);
#else
  dst_coef_arrays = src_coef_arrays;
#endif

  
  if (fp != stdin)
    fclose(fp);

  
  if (outfilename != NULL) {
    if ((fp = fopen(outfilename, WRITE_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s for writing\n", progname, outfilename);
      exit(EXIT_FAILURE);
    }
  } else {
    
    fp = write_stdout();
  }

  
  file_index = parse_switches(&dstinfo, argc, argv, 0, TRUE);

  
  jpeg_stdio_dest(&dstinfo, fp);

  
  jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

  
  jcopy_markers_execute(&srcinfo, &dstinfo, copyoption);

  
#if TRANSFORMS_SUPPORTED
  jtransform_execute_transformation(&srcinfo, &dstinfo,
				    src_coef_arrays,
				    &transformoption);
#endif

  
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  (void) jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);

  
  if (fp != stdout)
    fclose(fp);

#ifdef PROGRESS_REPORT
  end_progress_monitor((j_common_ptr) &dstinfo);
#endif

  
  exit(jsrcerr.num_warnings + jdsterr.num_warnings ?EXIT_WARNING:EXIT_SUCCESS);
  return 0;			
}
