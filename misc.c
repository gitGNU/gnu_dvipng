#include "dvipng.h"

static char *programname;

/*-->DecodeArgs*/
/*********************************************************************/
/***************************** DecodeArgs ****************************/
/*********************************************************************/
bool DecodeArgs(int argc, char ** argv)
{
  int     i;                 /* argument index for flags      */
  int32_t firstpage=PAGE_MINPAGE;
  int32_t lastpage=PAGE_LASTPAGE;
  bool abspage=_TRUE;
  static bool reverse=_FALSE;
  static bool parsestdin=_FALSE;

#ifndef KPATHSEA
  if ((tcp = getenv("TEXPXL")) != NULL) PXLpath = tcp;
#endif

  if (argc == 2 && (strcmp (argv[1], "--version") == 0)) {
    puts (VERSION);
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
	sscanf(p, "%u", &Debug);
	if(Debug==0) 
	  Debug = DEBUG_DVI;
#ifdef KPATHSEA
	kpathsea_debug = Debug/DEBUG_GLYPH;
#endif
	printf("Debug output enabled\n");
        break;
#endif
#if 0 /* -o disabled for now. rootname is dvi->outname nowadays */
      case 'o':       /* Output file is specified */
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
        (void) strcpy(rootname, p);
        /* remove .png extension */
        p1 = strrchr(rootname, '.');
        if (p1 != NULL && strcmp(p1,".png") == 0 ) {
	  *p1='\0';
        }
	if (parsestdin)
	  printf("Output file: %s#.png (#=page number)\n",rootname);
        break;
#endif
#ifdef MAKETEXPK
      case 'M':
        /* -M, -M1 => don't make font; -M0 => do.  */
        makeTexPK = (*p == '0');
#ifdef KPATHSEA
        kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_cmdline);
#endif /* KPATHSEA */
	if (parsestdin) {
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
	  if (parsestdin)
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
	if (parsestdin)
	  printf("Offset: (%d,%d)\n",x_offset_def,y_offset_def);
        break ;
      case 'T' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (strcmp(p,"bbox")==0) {
	  PassDefault=PASS_BBOX;
	  if (parsestdin)
	    printf("Pagesize: (bbox)\n");
	} else if (strcmp(p,"tight")==0) {
	  PassDefault=PASS_TIGHT_BBOX;
	  if (parsestdin)
	    printf("Pagesize: (tight bbox)\n");
	} else { 
	  handlepapersize(p, &x_width, &y_width) ;
	  if (Landscape) {
	    Warning("Both landscape and pagesize specified; using pagesize") ;
	    Landscape = _FALSE ;
	  }
	  /* Avoid PASS_BBOX */
	  PassDefault = PASS_DRAW;
	  if (parsestdin)
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
	if (parsestdin)
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
	  if (parsestdin) {
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
	  if (parsestdin)
	    printf("Transp. border: %d dots\n",borderwidth);
	}
	break;
      case 'f':
	if ( *p == 'g' ) { /* -fg foreground color */
	  p++;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  resetcolorstack(p);
	  if (parsestdin)
	    printf("Foreground: rgb %d,%d,%d\n",Red,Green,Blue);
	}
	break;
      case 'x' : case 'y' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (sscanf(p, "%d", &usermag)==0 || usermag < 1 ||
	    usermag > 1000000)
	  Fatal("Bad magnification parameter (-x or -y).") ;
	if (parsestdin)
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
	if (*p == 'p') {  /* a -pp specifier for a page list */
	  int pp_abspage = _FALSE;

	  p++ ;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if (*p == '=') {
	    pp_abspage = _TRUE ;
	    p++ ;
	  }
	  if (QueueParse(p,pp_abspage,reverse))
	    Fatal("bad page list specifier (-pp).") ;
	  break ;
	} else {   /* a -p specifier for first page */
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if (*p == '=') {
	    p++ ;
	  } else {
	    abspage = _FALSE;
	  }
	  if (sscanf(p, "%d", &firstpage)!=1) {
	    Fatal("bad first page option (-p %s).",p) ;
	  }
	if (parsestdin) 
	  printf("First page: %d\n",firstpage);
	}
	break ;
      case 'l':
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (*p == '=') {
	  p++ ;
	} else {
	  abspage = _FALSE ;
	}
	if (sscanf(p, "%d", &lastpage)!=1) {
	  Fatal("bad last page option (-l %s).",p) ;
	}
	if (parsestdin) 
	  printf("Last page: %d\n",lastpage);
	break ;
      case 'q':       /* quiet operation */
        G_quiet = _TRUE;
	G_verbose = _FALSE;
        break;
      case 'r':       /* switch order to process pages */
        reverse = (bool)(!reverse);
	if (parsestdin) {
	  if (reverse) 
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
	if (parsestdin) 
	  printf("Dpi: %d\n",resolution);
	break;
      case 'Q':       /* quality (= shrinkfactor) */
        if ( sscanf(p, "%d", &shrinkfactor) != 1 )
          Fatal("argument of -Q is not a valid integer\n");
	if (parsestdin) 
	  printf("Shrinkfactor: %d\n",shrinkfactor);
	break;
      case '\0':
	parsestdin=_TRUE;
	break;
      default:
	Warning("%c is not a valid flag\n", c);
      }
    } else {
      if (dvi != NULL && dvi->filep != NULL) {
	DVIClose(dvi);
      }
      dvi=DVIOpen(argv[i]);
    }
  }

  if (argv[0]!=NULL) {
    programname=argv[0];
    qfprintf(ERR_STREAM,"This is %s Copyright 2002 Jan-Åke Larsson\n", 
	     VERSION);
  }

  if (dvi==NULL) {
    fprintf(ERR_STREAM,"\nUsage: dvipng [OPTION]... FILENAME[.dvi]\n");
    fprintf(ERR_STREAM,"Options are chosen to be similar to dvips' options where possible:\n");
#ifdef DEBUG
    fprintf(ERR_STREAM,"  -d #      Debug (# is the debug bitmap, 1 if not given)\n");
#endif
    fprintf(ERR_STREAM,"  -D #      Resolution\n");
    fprintf(ERR_STREAM,"  -M*       Don't make fonts\n");
    fprintf(ERR_STREAM,"  -l #      Last page to be output\n");
    fprintf(ERR_STREAM,"  -mode s   MetaFont mode (default cx)\n");
    fprintf(ERR_STREAM,"  -o f      Output file (currently disabled)\n");
    fprintf(ERR_STREAM,"  -O c      Image offset\n");
    fprintf(ERR_STREAM,"  -p #      First page to be output\n");
    fprintf(ERR_STREAM,"  -pp #,#.. Page list to be output\n");
    fprintf(ERR_STREAM,"  -q        Quiet operation\n");
    fprintf(ERR_STREAM,"  -r        Reverse order of pages\n");
    fprintf(ERR_STREAM,"  -t c      Paper format (also accepts e.g., '-t a4')\n");
    fprintf(ERR_STREAM,"  -T c      Image size (also accepts '-T bbox' and '-T tight')\n");
    fprintf(ERR_STREAM,"  -v        Verbose operation\n");
    fprintf(ERR_STREAM,"  -x #      Override dvi magnification\n");
    fprintf(ERR_STREAM,"  -         Interactive query of options\n");
    fprintf(ERR_STREAM,"\nThese do not correspond to dvips options:\n");
    fprintf(ERR_STREAM,"  -bd #     Transparent border width in dots\n");
    fprintf(ERR_STREAM,"  -bg s     Background color (TeX-style color)\n");
    fprintf(ERR_STREAM,"  -fg s     Foreground color (TeX-style color)\n");
    fprintf(ERR_STREAM,"  -Q #      Quality (~xdvi's shrinkfactor)\n");
    
    fprintf(ERR_STREAM,"\n     # = number   f = file   s = string  * = suffix, '0' to turn off\n");
    fprintf(ERR_STREAM,"         c = comma-separated dimension pair (e.g., 3.2in,-32.1cm)\n\n");
    /*#ifdef KPATHSEA
      {
      extern DllImport char *kpse_bug_address;
      putc ('\n', ERR_STREAM);
      fputs (kpse_bug_address, ERR_STREAM);
      }
      #endif*/
    if (!parsestdin) {
      exit(1);
    }
  }

  if (!parsestdin || firstpage!=PAGE_MINPAGE || lastpage!=PAGE_LASTPAGE) {
    QueuePage(firstpage!=PAGE_MINPAGE ? firstpage : 1,
	      lastpage,abspage,reverse);
  }
  return(parsestdin);
}
/*
char * xmalloc(unsigned size)
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
void Fatal (char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  fprintf(ERR_STREAM, "\n");
  fprintf(ERR_STREAM, "%s: Fatal error, ", programname);
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
void Warning(char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

#ifndef vms
  G_errenc = 1;
#else
  G_errenc = (SS$_ABORT | STS$M_INHIB_MSG);  /* no message on screen */
#endif
  if ( G_nowarn || G_quiet )
    return;
  
  fprintf(ERR_STREAM, "%s warning: ", programname);
  vfprintf(ERR_STREAM, fmt, args);
  fprintf(ERR_STREAM, "\n");
  va_end(args);
}
