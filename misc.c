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
  int     i;                 /* argument index for flags      */
  char    curarea[STRSIZE];  /* current file area             */
  char    curname[STRSIZE];  /* current file name             */
  char    *p, *p1;           /* temporary character pointers  */
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

  for (i=1; i<argc; i++) {
    if (*argv[i]=='-') {
      char *p=argv[i]+2 ;
      char c=argv[i][1] ;
      switch (c) {
#ifdef DEBUG
      case 'd':       /* selects Debug output */
	Debug = _TRUE;
#ifdef KPATHSEA
	sscanf(p, "%u", &kpathsea_debug);
#endif
	qfprintf(ERR_STREAM,"Debug output enabled\n");
        break;
#endif
      case 'o':       /* Output file is specified */
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
        (void) strcpy(rootname, p);
        /* remove .png extension */
        p1 = strrchr(rootname, '.');
        if (p1 != NULL && strcmp(p1,".png") == 0 ) {
	  *p1='\0';
        }
        break;
#ifdef MAKETEXPK
      case 'M':
        /* -M, -M1 => don't make font; -M0 => do.  */
        makeTexPK = (*p == '0');
#ifdef KPATHSEA
        kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_cmdline);
#endif /* KPATHSEA */
        break;
#endif /* MAKETEXPK */
      case 'O' : /* Offset */
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	handlepapersize(p, &x_offset_def, &y_offset_def) ;
	x_offset = x_offset_def; 
	y_offset = y_offset_def; 
	x_max = x_min = -x_offset_def; /* set BBOX */
	y_max = y_min = -y_offset_def;
	break ;
      case 'T' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	handlepapersize(p, &x_width, &y_width) ;
	if (Landscape) {
	  Warning(
		"Both landscape and papersize specified; ignoring landscape") ;
	  Landscape = _FALSE ;
	}
	/* Avoid PASS_BBOX */
	PassDefault = PASS_DRAW;
	break ;
      case 't':       /* specify paper format, only for cropmarks */
	/* cropmarks not implemented yet */
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (strcmp(p,"a4")) {
	  handlepapersize("210mm,297mm",x_pwidth,y_pwidth);
	} else if (strcmp(p,"letter")) {
	  handlepapersize("8.5in,11in",x_pwidth,y_pwidth);
	} else if (strcmp(p,"legal")) {
	  handlepapersize("8.5in,14in",x_pwidth,y_pwidth);
	} else if (strcmp(p,"executive")) {
	  handlepapersize("7.25in,10.5in",x_pwidth,y_pwidth);
	} else if (strcmp(p,"landscape")) {
	  /* Bug out on both papersize and Landscape? */
	  Landscape = _TRUE;
	} else 
	  Fatal("The papersize %s is not implemented, sorry.\n",p);
        break;
      case 'x' : case 'y' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (sscanf(p, "%ld", &usermag)==0 || usermag < 1 ||
	    usermag > 1000000)
	  Fatal("Bad magnification parameter (-x or -y).") ;
	/*overridemag = (c == 'x' ? 1 : -1) ;*/
	break ;
      case 'p' :
#if defined(MSDOS) || defined(OS2) || defined(ATARIST)
	/* check for emTeX job file (-pj=filename) */
	/*if (*p == 'j') {
	  p++;
	  if (*p == '=' || *p == ':')
	    p++;
	  mfjobname = newstring(p);
	  break;
	  }*/
	/* must be page number instead */
#endif
	if (*p == 'p') {  /* a -pp specifier for a page list? */
	  p++ ;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if (ParsePages(p))
	    Fatal("bad page list specifier (-pp).") ;
	  Pagelist = _TRUE ;
	  break ;
	}
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (*p == '=') {
	  Abspage = _TRUE ;
	  p++ ;
	}
	switch(sscanf(p,
#ifdef SHORTINT
		      "%ld.%ld",
#else        /* ~SHORTINT */
		      "%d.%d",
#endif        /* ~SHORTINT */
		      &FirstPage, &Firstseq)) {
	case 1:           Firstseq = 0 ;
	case 2:           break ;
	default:     	  Fatal("bad first page option (-p %s).",p) ;
	}
	break ;
      case 'l':
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (*p == '=') {
	  Abspage = _TRUE ;
	  p++ ;
	}
	switch(sscanf(p, 
#ifdef SHORTINT
		      "%ld.%ld",
#else        /* ~SHORTINT */
		      "%d.%d",
#endif        /* ~SHORTINT */
		      &LastPage, &Lastseq)) {
	case 1:           Lastseq = 0 ;
	case 2:           break ;
	default:	  Fatal("bad last page option (-l %s).",p);
	}
	break ;
      case 'q':       /* quiet operation */
        G_quiet = _TRUE;
	G_verbose = _FALSE;
        break;
      case 'r':       /* switch order to process pages */
        Reverse = (bool)(!Reverse);
        break;
      case 'v':    /* verbatim mode */
        G_verbose = _TRUE;
        break;
      case 'D' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (sscanf(p, "%d", &resolution)==0 || resolution < 10 ||
	    resolution > 10000)
	  Fatal("bad dpi parameter (-D).") ;
	break;
      case 'Q':       /* quality (= shrinkfactor) */
        if ( sscanf(p, "%d", &shrinkfactor) != 1 )
          Fatal("argument of -Q is not a valid integer\n");
	break;
      default:
        Warning("%c is not a valid flag\n", *p);
      }
    } else {

#ifdef KPATHSEA
      /* split into directory + file name */
      p = (char *)basename(argv[i]);/* this knows about any kind of slashes */
      if (p == argv[i])
	curarea[0] = '\0';
      else {
	(void) strcpy(curarea, argv[i]);
	curarea[p-argv[i]] = '\0';
      }
#else
      p = strrchr(argv[i], '/');
      /* split into directory + file name */
      if (p == NULL) {
	curarea[0] = '\0';
	p = argv[i];
      } else {
	(void) strcpy(curarea, argv[i]);
	curarea[p-argv[i]+1] = '\0';
	p += 1;
      }
#endif
      
      (void) strcpy(curname, p);
      /* split into file name + extension */
      p1 = strrchr(curname, '.');
      if (p1 == NULL) {
	if (*rootname == '\0') {
	  (void) strcpy(rootname, curname);
	}
	strcat(curname, ".dvi");
      } else if (*rootname == '\0') {
	*p1 = '\0';
	(void) strcpy(rootname, curname);
	*p1 = '.';
      }
      
      (void) strcpy(filename, curarea);
      (void) strcat(filename, curname);
      
      if ((dvifp = BINOPEN(filename)) == FPNULL) {
	/* do not insist on .dvi */
	if (p1 == NULL) {
	  int l = strlen(curname);
	  if (l > 4)
	    curname[l - 4] = '\0';
	  l = strlen(filename);
	  if (l > 4)
	    filename[l - 4] = '\0';
	}
	if (p1 != NULL || (dvifp = BINOPEN(filename)) == FPNULL) {
#ifdef MSC5
	  Fatal("can't find DVI file \"%s\"\n\n", filename);
#else
	  perror(filename);
	  exit (EXIT_FAILURE);
#endif
	}
      }
    }
  }

  if (dvifp == FPNULL) {
    fprintf(ERR_STREAM,"\nThis is the DVI to PNG converter version %s",
             VERSION);
    fprintf(ERR_STREAM," (%s)\n", OS);
    fprintf(ERR_STREAM,"usage: %s [OPTION]... DVIFILE\n", G_progname);

    fprintf(ERR_STREAM,"OPTIONS are:\n");
#ifdef DEBUG
    fprintf(ERR_STREAM,"\t-d ..... turns debug output on\n");
#endif
    /*    fprintf(ERR_STREAM,
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
            0 );
    fprintf(ERR_STREAM,"\t-YO ..... O= y page origin in dots (default=%d)\n",
            0 );
	    fprintf(ERR_STREAM,"\t-   ..... dvifile is stdin (must be seekable); implies -e-\n");*/
    /*#ifdef KPATHSEA
    {
      extern DllImport char *kpse_bug_address;
      putc ('\n', ERR_STREAM);
      fputs (kpse_bug_address, ERR_STREAM);
    }
    #endif*/
    exit(1);
  }
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
