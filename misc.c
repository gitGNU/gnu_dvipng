#include "dvipng.h"

/*-->ActualFactor*/
/**********************************************************************/
/**************************  ActualFactor  ****************************/
/**********************************************************************/
double  /* compute the actual size factor given the approximation */
#if NeedFunctionPrototypes
ActualFactor(long4 unmodsize)
#else
ActualFactor(unmodsize)
long4    unmodsize;                 /* actually factor * 1000 */
#endif
{
  double  realsize;     /* the actual magnification factor */
  realsize = (double)unmodsize / 1000.0;
  if (abs((int)(unmodsize - 1095l))<2)
    realsize = 1.095445115; /*stephalf*/
  else if (abs((int)(unmodsize - 1315l))<2)
    realsize = 1.31453414; /*stepihalf*/
  else if (abs((int)(unmodsize - 1577l))<2)
    realsize = 1.57744097; /*stepiihalf*/
  else if (abs((int)(unmodsize - 1893l))<2)
    realsize = 1.89292916; /*stepiiihalf*/
  else if (abs((int)(unmodsize - 2074l))<2)
    realsize = 2.0736;   /*stepiv*/
  else if (abs((int)(unmodsize - 2488l))<2)
    realsize = 2.48832;  /*stepv*/
  else if (abs((int)(unmodsize - 2986l))<2)
    realsize = 2.985984; /*stepvi*/
  /* the remaining magnification steps are represented with sufficient
     accuracy already */
  return(realsize);
}


/*-->DecodeArgs*/
/*********************************************************************/
/***************************** DecodeArgs ****************************/
/*********************************************************************/
void
#if NeedFunctionPrototypes
DecodeArgs(int argc, char *argv[])
#else
DecodeArgs(argc, argv)
int     argc;
char    *argv[];
#endif
{
  int     argind;            /* argument index for flags      */
  char    curarea[STRSIZE];  /* current file area             */
  char    curname[STRSIZE];  /* current file name             */
  char    *tcp, *tcp1;       /* temporary character pointers  */
  char    *this_arg;
  double  x_offset = 0.0, y_offset = 0.0;
#ifdef __riscos
  int     ddi;
#endif

#ifndef KPATHSEA
  if ((tcp = getenv("TEXPXL")) != NULL) PXLpath = tcp;
#endif

  if (argc == 2 && (strcmp (argv[1], "--version") == 0)) {
    puts ("dvipng(k) 0.0");
#ifdef KPATHSEA
    puts (KPSEVERSION);
#endif
    puts ("Copyright (C) 2002 Jan-Åke Larsson.\n\
There is NO warranty.  You may redistribute this software\n\
under the terms of the GNU General Public License.\n\
For more information about these matters, see the files\n\
named COPYING and dvipng.c.");
    exit (0); 
  }

  argind = 1;
  while (argind < argc) {
    tcp = argv[argind];
    if (tcp[0] == '-' && tcp[1] != '\0') {
      ++tcp;
      switch (*tcp) {
#ifdef DEBUG
      case 'd':       /* selects Debug output */
	Debug = _TRUE;
#ifdef KPATHSEA
	sscanf(tcp + 1, "%u", &kpathsea_debug);
#endif
	qfprintf(ERR_STREAM,"Debug output enabled\n");
        break;
#endif
      case 'o':       /* Output file is specified */
        EmitFileName = ++tcp;
#ifdef MSDOS
        /* delete trailing ':' (causing hangup) */
        if (EmitFileName[strlen(EmitFileName)-1] == ':')
          EmitFileName[strlen(EmitFileName)-1] = '\0';
#endif
#ifdef OS2  /* repeated to avoid problems with stupid c preprocessors  */
        /* delete trailing ':' (causing hangup) */
        if (EmitFileName[strlen(EmitFileName)-1] == ':')
          EmitFileName[strlen(EmitFileName)-1] = '\0';
#endif
        break;
#ifdef MAKETEXPK
      case 'M':
        /* -M, -M1 => don't make font; -M0 => do.  */
        makeTexPK = *(tcp + 1) == '0';
#ifdef KPATHSEA
        kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_cmdline);
#endif /* KPATHSEA */
        break;
#endif /* MAKETEXPK */
      case 'O':       /* specify origin */
	/*        this_arg = 0;
        if (!(*++tcp)) {
          this_arg = (++argind >= argc ? 0 : argv[argind]);
        } else {
          this_arg = tcp;
        }
        if (!this_arg || sscanf(this_arg,"%hd", &x_origin) != 1)
	Fatal("Argument of -X is not a valid integer\n");*/
	Fatal("Offset specification -O # is not yet implemented");
        break;
      case 'x':       /* specify magnification to use */
	this_arg = 0;
        if (!(*++tcp)) {
          this_arg = (++argind >= argc ? 0 : argv[argind]);
        } else {
          this_arg = tcp;
        }
        if (!this_arg || sscanf(this_arg,"%hd", &usermag) != 1)
            Fatal("Argument of -x is not a valid integer\n");
        break;
      case 'p':       /* first page  */
        if ( sscanf(tcp + 1, "%ld", &FirstPage) != 1 )
          Fatal("Argument is not a valid integer\n");
        FirstPageSpecified = _TRUE;
        break;
      case 'q':       /* quiet operation */
        G_quiet = _TRUE;
        break;
      case 'r':       /* switch order to process pages */
        Reverse = (bool)(!Reverse);
        break;
      case 't':       /* specify paper format */
        this_arg = ++tcp;
	if (strcmp(this_arg,"a4")) {
	  v_pgsiz_dots = 297*resolution/shrinkfactor/25.4;
	  h_pgsiz_dots = 210*resolution/shrinkfactor/25.4;
	} else if (strcmp(this_arg,"letter")) {
	  v_pgsiz_dots = 8.5*resolution/shrinkfactor;
	  h_pgsiz_dots = 11*resolution/shrinkfactor;
	} else if (strcmp(this_arg,"legal")) {
	  v_pgsiz_dots = 8.5*resolution/shrinkfactor;
	  h_pgsiz_dots = 14*resolution/shrinkfactor;
	} else if (strcmp(this_arg,"executive")) {
	  v_pgsiz_dots = 7.25*resolution/shrinkfactor;
	  h_pgsiz_dots = 10.5*resolution/shrinkfactor;
	} else if (strcmp(this_arg,"landscape")) {
	  Landscape = _TRUE;
	} else 
	  Fatal("The pagesize %s is not implemented, sorry.\n",this_arg);
        break;
      case 'l':       /* ending pagenumber */
        if ( sscanf(tcp + 1, "%ld", &LastPage) != 1 )
          Fatal("Argument of -l is not a valid integer\n");
        LastPageSpecified = _TRUE;
        break;
      case 'v':    /* verbatim mode (print pxl-file names) */
        G_verbose = _TRUE;
        break;
      case 'D':       /* resolution */
        if ( sscanf(tcp + 1, "%d", &resolution) != 1 )
          Fatal("Argument of -D is not a valid integer\n");
	/*	MFMODE = MFMODE600; */
	x_origin = resolution;
	y_origin = resolution;
	break;
      case 'Q':       /* quality (= shrinkfactor) */
        if ( sscanf(tcp + 1, "%d", &shrinkfactor) != 1 )
          Fatal("Argument of -Q is not a valid integer\n");
	break;
      default:
        fprintf(ERR_STREAM, "%c is not a valid flag\n", *tcp);
      }
    } else {

#ifdef KPATHSEA
        /* split into directory + file name */
	tcp = (char *)basename(argv[argind]);/* this knows about any kind of slashes */
	if (tcp == argv[argind])
	  curarea[0] = '\0';
	else {
	  (void) strcpy(curarea, argv[argind]);
	  curarea[tcp-argv[argind]] = '\0';
	}
#else
        tcp = strrchr(argv[argind], '/');
        /* split into directory + file name */
        if (tcp == NULL) {
          curarea[0] = '\0';
          tcp = argv[argind];
        } else {
          (void) strcpy(curarea, argv[argind]);
          curarea[tcp-argv[argind]+1] = '\0';
          tcp += 1;
        }
#endif

        (void) strcpy(curname, tcp);
        /* split into file name + extension */
        tcp1 = strrchr(curname, '.');
        if (tcp1 == NULL) {
          (void) strcpy(rootname, curname);
          strcat(curname, ".dvi");
        } else {
          *tcp1 = '\0';
          (void) strcpy(rootname, curname);
          *tcp1 = '.';
        }

        (void) strcpy(filename, curarea);
        (void) strcat(filename, curname);

        if ((dvifp = BINOPEN(filename)) == FPNULL) {
          /* do not insist on .dvi */
          if (tcp1 == NULL) {
            int l = strlen(curname);
            if (l > 4)
              curname[l - 4] = '\0';
            l = strlen(filename);
            if (l > 4)
              filename[l - 4] = '\0';
          }
          if (tcp1 != NULL || (dvifp = BINOPEN(filename)) == FPNULL) {
#ifdef MSC5
            Fatal("%s: can't find DVI file \"%s\"\n\n",
                  G_progname, filename);
#else
            perror(filename);
            exit (EXIT_FAILURE);
#endif
          }
        }
    }
    argind++;
  }

  x_goffset = (short) MM_TO_PXL(x_offset)/shrinkfactor + x_origin/shrinkfactor;
  y_goffset = (short) MM_TO_PXL(y_offset)/shrinkfactor + y_origin/shrinkfactor;

  if (dvifp == FPNULL) {
    fprintf(ERR_STREAM,"\nThis is the DVI to PNG converter version %s",
             VERSION);
    fprintf(ERR_STREAM," (%s)\n", OS);
    fprintf(ERR_STREAM,"usage: %s [OPTION]... DVIFILE\n", G_progname);

    fprintf(ERR_STREAM,"OPTIONS are:\n");
#ifdef DEBUG
    fprintf(ERR_STREAM,"\t--D ..... turns debug output on\n");
#endif
    fprintf(ERR_STREAM,
            "\t-aX ..... X= searchpath leading to pixel-files (.pk or .pxl)\n");
    fprintf(ERR_STREAM,"\t-cX ..... X= number of copies\n");
    fprintf(ERR_STREAM,"\t-eX ..... X= output file\n");
    fprintf(ERR_STREAM,"\t-fX ..... print from begin of page number X\n");
#ifdef __riscos
    fprintf(ERR_STREAM,"\t-iX ..... X= name of dir to cache diagrams in\n");
    fprintf(ERR_STREAM,"\t-j  ..... don't print diagrams\n");
    fprintf(ERR_STREAM,"\t-k  ..... cache diagram bitmaps\n");
#endif
    fprintf(ERR_STREAM,"\t-l  ..... landscape mode\n");
#ifdef MAKETEXPK
    fprintf(ERR_STREAM,"\t-MX ..... Don't generate missing PK files\n");
#endif
    fprintf(ERR_STREAM,"\t-mX ..... magnification X=0;h;1;2;3;4;5;#xxxx\n");
    fprintf(ERR_STREAM,"\t-pX ..... print X pages\n");
#ifdef __riscos
    fprintf(ERR_STREAM,"\t-P  ..... Process printouts in 2 passes\n");
#endif
    fprintf(ERR_STREAM,"\t-q  ..... quiet operation\n");
    fprintf(ERR_STREAM,"\t-r  ..... process pages in reverse order\n");
    fprintf(ERR_STREAM,"\t-RX ..... set resolution to X dpi\n");
    fprintf(ERR_STREAM,"\t-sX ..... set paper size to X (see documentation)\n");
    fprintf(ERR_STREAM,"\t-tX ..... print to end of page number X\n");
    fprintf(ERR_STREAM,"\t-w  ..... don't print out warnings\n");
    fprintf(ERR_STREAM,"\t-v  ..... tell user which pixel-files are used\n");
    fprintf(ERR_STREAM,"\t-xX ..... X= x-offset on printout in mm\n");
    fprintf(ERR_STREAM,"\t-yX ..... X= y-offset on printout in mm\n");
    fprintf(ERR_STREAM,"\t-XO ..... O= x page origin in dots (default=%d)\n",
            XDEFAULTOFF );
    fprintf(ERR_STREAM,"\t-YO ..... O= y page origin in dots (default=%d)\n",
            YDEFAULTOFF );
    fprintf(ERR_STREAM,"\t-   ..... dvifile is stdin (must be seekable); implies -e-\n");
    /*#ifdef KPATHSEA
    {
      extern DllImport char *kpse_bug_address;
      putc ('\n', ERR_STREAM);
      fputs (kpse_bug_address, ERR_STREAM);
    }
    #endif*/
    exit(1);
  }
  if (EQ(EmitFileName, "")) {
    if ((EmitFileName = (char *)malloc( STRSIZE )) != NULL)
      allocated_storage += STRSIZE;
    else
      Fatal("Can't allocate storage of %d bytes\n",STRSIZE);
    (void) strcpy(EmitFileName, curname);
    if ((tcp1 = strrchr(EmitFileName, '.')))
      *tcp1 = '\0';
    strcat(EmitFileName, "png");
  }
  if (G_quiet)
    G_verbose = _FALSE;
}

/*-->DoConv*/
/*********************************************************************/
/********************************  DoConv  ***************************/
/*********************************************************************/
long4
#if NeedFunctionPrototypes
DoConv(long4 num, long4 den, int convResolution)
#else
DoConv(num, den, convResolution)
long4    num, den;
int     convResolution;
#endif
{
  /*register*/ double conv;
  conv = ((double)num / (double)den) *
    ((double)mag / 1000.0) *
    ((double)convResolution/254000.0);

  return((long4)((1.0/conv)+0.5));
}

/*-->AllDone*/
/**********************************************************************/
/****************************** AllDone  ******************************/
/**********************************************************************/
#if NeedFunctionPrototypes
void AllDone(bool PFlag)
#else
void AllDone(PFlag)
bool PFlag;
#endif
{
#ifdef TIMING
  double  time;
#endif

#ifdef DEBUG
  if (Debug) {
    fprintf(ERR_STREAM,"\nDynamically allocated storage: %ld Bytes \n",
            (long)allocated_storage);
    fprintf(ERR_STREAM,"%d characters downloaded as soft fonts\n", G_ncdl);
  }
#endif

#ifdef TIMING
#ifdef BSD_TIME_CALLS
  ftime(&timebuffer);
  time = (timebuffer.time + (float)(timebuffer.millitm)/1000.0) - start_time;
#else
  gettimeofday(&Tp, NULL);
  time = (Tp.tv_sec + (float)(Tp.tv_usec)/1000000.0) - start_time;
#endif
  
  if (ndone > 0) {
    fprintf(ERR_STREAM,
	    "Time of complete run: %.2f s, %d page(s), %.2f s/page.\n",
	    time, ndone, time / ndone);
  }
  if (my_toc > 0) {
    fprintf(ERR_STREAM,
	    "Thereof in TIC/TOC region %.2f s.\n",my_toc);
  }
#endif

  CloseFiles();
  exit(G_errenc);
}

/*-->Fatal*/
/**********************************************************************/
/******************************  Fatal  *******************************/
/**********************************************************************/
void
#if NeedVarargsPrototypes
Fatal (char *fmt, ...)
#else
Fatal(va_alist)      /* issue a fatal error message */
     va_dcl
#endif
{
#if !NeedVarargsPrototypes
  const char *fmt;
#endif
  va_list args;

#if NeedVarargsPrototypes
  va_start(args, fmt);
#else
  va_start(args);
  fmt = va_arg(args, const char *);
#endif
  fprintf(ERR_STREAM, "\n");
  fprintf(ERR_STREAM, "%s: Fatal error, ", G_progname);
  vfprintf(ERR_STREAM, fmt, args);

  fprintf(ERR_STREAM, "\n\n");
  va_end(args);
  CloseFiles();
#ifndef vms
  exit(2);
#else
  exit(SS$_ABORT);
#endif
}



/*-->Warning*/
/**********************************************************************/
/*****************************  Warning  ******************************/
/**********************************************************************/
void                           /* issue a warning */
#if NeedVarargsPrototypes
Warning(char *fmt, ...)
#else
Warning(va_alist)
     va_dcl
#endif
{
#if !NeedVarargsPrototypes
  const char *fmt;
#endif
  va_list args;

#if NeedVarargsPrototypes
  va_start(args, fmt);
#else
  va_start(args);
  fmt = va_arg(args, const char *);
#endif

#ifndef vms
  G_errenc = 1;
#else
  G_errenc = (SS$_ABORT | STS$M_INHIB_MSG);  /* no message on screen */
#endif
  if ( G_nowarn || G_quiet )
    return;
  
  fprintf(ERR_STREAM, "%s warning: ", G_progname);
  vfprintf(ERR_STREAM, fmt, args);
  fprintf(ERR_STREAM, "\n");
  va_end(args);
}
