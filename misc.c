#include "dvipng.h"


/*-->DecodeArgs*/
/*********************************************************************/
/***************************** DecodeArgs ****************************/
/*********************************************************************/
void DecodeArgs P2C(int, argc, char **, argv)
{
  int     i;                 /* argument index for flags      */

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
	printf("Debug output enabled\n");
        break;
#endif
	/*      case 'o':       *//* Output file is specified */
	/*	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
        (void) strcpy(rootname, p);
        *//* remove .png extension */
        /*p1 = strrchr(rootname, '.');
        if (p1 != NULL && strcmp(p1,".png") == 0 ) {
	  *p1='\0';
        }
	if (ParseStdin)
	  printf("Output file: %s.png\n",rootname);
        break;*/
#ifdef MAKETEXPK
      case 'M':
        /* -M, -M1 => don't make font; -M0 => do.  */
        makeTexPK = (*p == '0');
#ifdef KPATHSEA
        kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_cmdline);
#endif /* KPATHSEA */
	if (ParseStdin) {
	  if (makeTexPK)
	    printf("MakeTeXPK enabled\n");
	  else
	    printf("MakeTeXPK disabled\n");
	}
        break;
      case 'm':
	if (strcmp(p,"ode") == 0 ) {
	  if (argv[i+1])
	    MFMODE = argv[++i] ;
	  (void) strcpy(rootname, p);
	  if (ParseStdin)
	    printf("MetaFont mode: %s\n",MFMODE);
	  break ;
	}
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
	if (ParseStdin)
	  printf("Offset: (%d,%d)\n",x_offset_def,y_offset_def);
        break ;
      case 'T' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (strcmp(p,"bbox")==0) {
	  PassDefault=PASS_BBOX;
	  if (ParseStdin)
	    printf("Pagesize: (bbox)\n");
	} else { 
	  handlepapersize(p, &x_width, &y_width) ;
	  if (Landscape) {
	    Warning("Both landscape and pagesize specified; using pagesize") ;
	    Landscape = _FALSE ;
	  }
	  /* Avoid PASS_BBOX */
	  PassDefault = PASS_DRAW;
	  if (ParseStdin)
	    printf("Pagesize: (%d,%d)\n",x_width,y_width);
	}
	break ;
      case 't':       /* specify paper format, only for cropmarks */
	/* cropmarks not implemented yet */
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (strcmp(p,"a4")) {
	  handlepapersize("210mm,297mm",&x_pwidth,&y_pwidth);
	} else if (strcmp(p,"letter")) {
	  handlepapersize("8.5in,11in",&x_pwidth,&y_pwidth);
	} else if (strcmp(p,"legal")) {
	  handlepapersize("8.5in,14in",&x_pwidth,&y_pwidth);
	} else if (strcmp(p,"executive")) {
	  handlepapersize("7.25in,10.5in",&x_pwidth,&y_pwidth);
	} else if (strcmp(p,"landscape")) {
	  /* Bug out on both papersize and Landscape? */
	  Landscape = _TRUE;
	} else 
	  Fatal("The papersize %s is not implemented, sorry.\n",p);
	if (ParseStdin)
	  printf("Papersize: %s\n",p);
        break;
      case 'b':
	if ( *p == 'g' ) { /* -bg background color */
	  p++;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if (strncmp(p,"Transparent",11) == 0 ) {
	    borderwidth=-1;
	  } else {
	    background(p);
	  }
	  if (ParseStdin) {
	    if (borderwidth>=0) {
	      printf("Background: rgb %d,%d,%d\n",bRed,bGreen,bBlue);
	    } else {
	      printf("Transp. background (fallback rgb %d,%d,%d)\n",
		     bRed,bGreen,bBlue);
	    }
	  } 
	} else if ( *p == 'd' ) { /* -bd border width */
	  p++;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if ( sscanf(p, "%d", &borderwidth) != 1 )
	    Fatal("argument of -bd is not a valid integer\n");
	  if (ParseStdin)
	    printf("Transp. border: %d dots\n",borderwidth);
	}
	break;
      case 'f':
	if ( *p == 'g' ) { /* -fg foreground color */
	  p++;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  resetcolorstack(p);
	  if (ParseStdin)
	    printf("Foreground: rgb %d,%d,%d\n",Red,Green,Blue);
	}
	break;
      case 'x' : case 'y' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (sscanf(p, "%d", &usermag)==0 || usermag < 1 ||
	    usermag > 1000000)
	  Fatal("Bad magnification parameter (-x or -y).") ;
	if (ParseStdin)
	  printf("Magstep: %d\n",usermag);
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
	  if (*p == '=') {
	    Abspage = _TRUE ;
	    p++ ;
	  }
	  if (QueueParse(p,Abspage))
	    Fatal("bad page list specifier (-pp).") ;
	  break ;
	}
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (*p == '=') {
	  Abspage = _TRUE ;
	  p++ ;
	}
	switch(sscanf(p, "%d.%d", &FirstPage, &Firstseq)) {
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
	switch(sscanf(p, "%d.%d", &LastPage, &Lastseq)) {
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
	if (ParseStdin) {
	  if (Reverse) 
	    printf("Reverse order\n");
	  else
	    printf("Normal order\n");
	}
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
	if (ParseStdin) 
	  printf("Dpi: %d\n",resolution);
	break;
      case 'Q':       /* quality (= shrinkfactor) */
        if ( sscanf(p, "%d", &shrinkfactor) != 1 )
          Fatal("argument of -Q is not a valid integer\n");
	if (ParseStdin) 
	  printf("Shrinkfactor: %d\n",shrinkfactor);
	break;
      case '\0':
	ParseStdin=_TRUE;
	break;
      default:
        Warning("%c is not a valid flag\n", c);
      }
    } else {
      if (dvi != NULL && dvi->filep != FPNULL) {
	DVIClose(dvi);
      }
      dvi=DVIOpen(argv[i]);
      if ((hpagelistp=InitPage())==NULL)
	Fatal("no pages in DVI file");
    }
  }

  if (dvi->filep == FPNULL) {
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
uint32_t DoConv P3C(uint32_t, num, uint32_t, den, int, convResolution)
{
  /*register*/ double conv;
  conv = ((double)num / (double)den) *
    ((double)mag / 1000.0) *
    ((double)convResolution/254000.0);

  return((uint32_t)((1.0/conv)+0.5));
}


/*
char * xmalloc P1C(unsigned, size)
{
  char *mem;
  
  if ((mem = malloc(size)) == NULL)
    Fatal("cannot allocate %d bytes", size);
  return mem;
}
*/

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
