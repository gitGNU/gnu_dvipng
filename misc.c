#include "dvipng.h"

static char *programname;
static int   flags=BE_NONQUIET;

/*-->DecodeArgs*/
/*********************************************************************/
/***************************** DecodeArgs ****************************/
/*********************************************************************/
bool DecodeArgs(int argc, char ** argv)
{
  int     i;                 /* argument index for flags       */
  bool ppused=_FALSE;        /* Flag when -pp is used          */
  char *dviname=NULL;        /* Name of dvi file               */
  char *outname=NULL;        /* Name of output file (concatenated
				with the page number and .png) */

  programname=argv[0];

  if (argc == 2 && (strcmp (argv[1], "--version") == 0)) {
    puts (PACKAGE_STRING);
#ifdef HAVE_LIBKPATHSEA
    puts (KPSEVERSION);
#endif
    puts ("Copyright (C) 2002-2003 Jan-Åke Larsson.\n\
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
	{ 
	  int debug = 0;
	  
	  if (*p == 0 && argv[i+1])
	    p = argv[i+1];
	  debug = atoi(p);
	  flags |= (debug>0) ? debug * LASTFLAG * 2 : DEBUG_DVI;
	  if (debug > 0 && p == argv[i+1])
	    i++;
#ifdef HAVE_LIBKPATHSEA
	  kpathsea_debug = ( debug * LASTFLAG * 2) / LASTDEBUG;
#endif
	  Message(PARSE_STDIN,"Debug output enabled\n");
	}
        break;
#endif
      case 'o':       /* Output file is specified */
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
        outname=p;
        /* remove .png extension */
	Message(PARSE_STDIN,"Output file: %s\n",
		outname);
        break;
#ifdef MAKETEXPK
      case 'M':
        /* -M, -M1 => don't make font; -M0 => do.  */
        makeTexPK = (*p == '0');
#ifdef HAVE_LIBKPATHSEA
        kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_cmdline);
#endif 
	if (makeTexPK)
	  Message(PARSE_STDIN,"MakeTeXPK enabled\n");
	else
	  Message(PARSE_STDIN,"MakeTeXPK disabled\n");
        break;
      case 'm':
	if (strcmp(p,"ode") == 0 ) {
	  if (argv[i+1])
	    MFMODE = argv[++i] ;
	  Message(PARSE_STDIN,"MetaFont mode: %s\n",MFMODE);
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
	Message(PARSE_STDIN,"Offset: (%d,%d)\n",x_offset_def,y_offset_def);
        break ;
      case 'T' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	if (strcmp(p,"bbox")==0) {
	  PassDefault=PASS_BBOX; 
	  Message(PARSE_STDIN,"Pagesize: (bbox)\n");
	} else if (strcmp(p,"tight")==0) {
	  PassDefault=PASS_TIGHT_BBOX;
	  Message(PARSE_STDIN,"Pagesize: (tight bbox)\n");
	} else { 
	  handlepapersize(p, &x_width, &y_width) ;
	  /* Avoid PASS_BBOX */
	  PassDefault = PASS_DRAW;
	  Message(PARSE_STDIN,"Pagesize: (%d,%d)\n",x_width,y_width);
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
	} else 
	  Fatal("The papersize %s is not implemented, sorry.\n",p);
	Message(PARSE_STDIN,"Papersize: %s\n",p);
        break;
      case 'b':
	if ( *p == 'g' ) { /* -bg background color */
	  p++;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if (strncmp(p,"Transparent",11) == 0 ) 
	    borderwidth=-1;
	  else
	    background(p);
	  if (borderwidth>=0) 
	    Message(PARSE_STDIN,"Background: rgb %d,%d,%d\n",
		    bRed,bGreen,bBlue);
	  else 
	    Message(PARSE_STDIN,"Transp. background (fallback rgb %d,%d,%d)\n",
		    bRed,bGreen,bBlue);
	} else if ( *p == 'd' ) { /* -bd border width */
	  p++;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  borderwidth = atoi(p);
	  Message(PARSE_STDIN,"Transp. border: %d dots\n",borderwidth);
	}
	break;
      case 'f':
	if ( *p == 'g' ) { /* -fg foreground color */
	  p++;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  resetcolorstack(p);
	  Message(PARSE_STDIN,"Foreground: rgb %d,%d,%d\n",Red,Green,Blue);
	} else if (strcmp(p,"ollow") == 0 ) {
	  if (DVIFollowToggle())
	    Message(PARSE_STDIN,"Follow mode on\n");
	  else
	    Message(PARSE_STDIN,"Follow mode off\n");
	}
	break;
      case 'x' : case 'y' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	usermag = atoi(p);
	if (usermag < 1 || usermag > 1000000)
	  Fatal("Bad magnification parameter (-x or -y).") ;
	Message(PARSE_STDIN,"Magstep: %d\n",usermag);
	/*overridemag = (c == 'x' ? 1 : -1) ;*/
	break ;
      case 'p' :
	if (*p == 'p') {  /* a -pp specifier for a page list */
	  ppused=_TRUE;
	  p++ ;
	  if (*p == 0 && argv[i+1])
	    p = argv[++i];
	  Message(PARSE_STDIN,"Page list: %s\n",p);
	  if (ParsePages(p))
	    Fatal("bad page list specifier (-pp).");
	  break ;
	} else {   /* a -p specifier for first page */
	  int32_t firstpage;
	  bool abspage=_FALSE;

	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if (*p == '=') {
	    abspage=_TRUE;
	    p++ ;
	  }
	  firstpage = atoi(p);
	  FirstPage(firstpage,abspage);
	  Message(PARSE_STDIN,"First page: %d\n",firstpage);
	}
	break ;
      case 'l':
	{
	  int32_t lastpage;
	  bool abspage=_FALSE;

	  if (*p == 0 && argv[i+1])
	    p = argv[++i] ;
	  if (*p == '=') {
	    abspage=_TRUE;
	    p++ ;
	  } 
	  lastpage = atoi(p);
	  LastPage(lastpage,abspage);
	  Message(PARSE_STDIN,"Last page: %d\n",lastpage);
	}
	break ;
      case 'q':       /* quiet operation */
        flags &= !BE_NONQUIET & !BE_VERBOSE;
        break;
      case 'r':       /* switch order to process pages */
	if (Reverse()) 
	  Message(PARSE_STDIN,"Reverse order\n");
	else
	  Message(PARSE_STDIN,"Normal order\n");
        break;
      case 'v':    /* verbatim mode */
	flags |= BE_NONQUIET | BE_VERBOSE;
        break;
      case 'D' :
	if (*p == 0 && argv[i+1])
	  p = argv[++i] ;
	resolution = atoi(p);
	if (resolution < 10 || resolution > 10000)
	  Fatal("bad dpi parameter (-D).") ;
	Message(PARSE_STDIN,"Dpi: %d\n",resolution);
	break;
      case 'Q':       /* quality (= shrinkfactor) */
        shrinkfactor = atoi(p);
	Message(PARSE_STDIN,"Shrinkfactor: %d\n",shrinkfactor);
	break;
      case '\0':
	flags |= PARSE_STDIN;
	break;
      default:
	Warning("%s is not a valid option", argv[i]);
      }
    } else {
      dviname=argv[i];
    }
  }

  Message(BE_NONQUIET,"This is %s Copyright 2002-2003 Jan-Åke Larsson\n",
	  PACKAGE_STRING);

  if (dviname != NULL) {
    if (dvi != NULL && dvi->filep != NULL) 
      DVIClose(dvi);
    dvi=DVIOpen(dviname,outname);  
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
    fprintf(ERR_STREAM,"  -o f      Output file, '%%d' is pagenumber\n");
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
    fprintf(ERR_STREAM,"  -follow   Follow mode\n");
    fprintf(ERR_STREAM,"  -Q #      Quality (~xdvi's shrinkfactor)\n");
    
    fprintf(ERR_STREAM,"\n   # = number   f = file   s = string  * = suffix, '0' to turn off\n");
    fprintf(ERR_STREAM,"       c = comma-separated dimension pair (e.g., 3.2in,-32.1cm)\n\n");
    /*#ifdef HAVE_LIBKPATHSEA
      {
      extern DllImport char *kpse_bug_address;
      putc ('\n', ERR_STREAM);
      fputs (kpse_bug_address, ERR_STREAM);
      }
      #endif*/
    if ((flags & PARSE_STDIN) == 0) {
      exit(1);
    }
  }
  if ((flags & PARSE_STDIN) == 0 && (! ppused)) 
    ParsePages("--");
  return((flags & PARSE_STDIN) != 0);
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

void DecodeString(char *string)
{
#define PARSEARGS 10
  char    *strv[PARSEARGS];
  int     strc=1;
  strv[0]=NULL;                       /* No program name */

  while (*string==' ' || *string=='\t' || *string=='\n') 
    string++;
  while (*string!='\0') {
    strv[strc++]=string;
    if (*string!='\'') {
      /* Normal split at whitespace */
      while (*string!=' ' && *string!='\t' && *string!='\n' && *string!='\0') 
	string++;
    } else {
      /* String delimiter found , so search for next */
      strv[strc-1]=++string;
      while (*string!='\'' && *string!='\0') 
	string++;
    }
    if (*string!='\0')
      *string++='\0';
    while (*string==' ' || *string=='\t' || *string=='\n') 
      string++;
  }
  if (strc>1) /* Nonempty */
    (void) DecodeArgs(strc,strv);
}



uint32_t UNumRead(unsigned char* current, register int n)
{
  uint32_t x = (unsigned char) *(current)++; /* number being constructed */
  while(--n) 
    x = (x << 8) | *(current)++;
  return(x);
}

int32_t SNumRead(unsigned char* current, register int n)
{
  int32_t x = (signed char) *(current)++; /* number being constructed */
  while(--n)
    x = (x << 8) | *(current)++;
  return(x);
}


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
  exit(EXIT_FAILURE);
}



/*-->Warning*/
/**********************************************************************/
/*****************************  Warning  ******************************/
/**********************************************************************/
void Warning(char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

  if ( flags & BE_NONQUIET ) {
    fprintf(ERR_STREAM, "%s warning: ", programname);
    vfprintf(ERR_STREAM, fmt, args);
    fprintf(ERR_STREAM, "\n");
    va_end(args);
  }
}

/*-->Message*/
/**********************************************************************/
/*****************************  Message  ******************************/
/**********************************************************************/
void Message(int activeflags, char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  if ( flags & activeflags ) {
    vfprintf(ERR_STREAM, fmt, args);
#ifdef DEBUG
    fflush(ERR_STREAM);
#endif
  }
  va_end(args);
}

