/* dvipng.c
 */
/**********************************************************************
 ****************************  Intro  *********************************
 **********************************************************************
 * This program translates TeX's DVI-Code into Portable Network Graphics.
 *
 * Copyright (C) Jan-Åke Larsson 2002
 *
 * Recipe:
 * Take a simple dvi2?? converter, dvilj is suitable here.
 * Read and relish the code of xdvi and dvips
 * Choose a png-drawing library with a simple interface and support
 *  on many platforms: gd
 * Stir, sprinkle with some insights of your own, and enjoy
 *
 **********************************************************************
 * Preprocessor switches:
 *      #define DEBUG    for massive printing of trace information
 *               when -d cmdline option specified
 *      #define DRAWGLYPH draws PK-Glyphs on stdout
 **********************************************************************
 */

#define MAIN
#include "dvipng.h"

/**********************************************************************/
/*******************************  main  *******************************/
/**********************************************************************/
int main P2C(int, argc, char **, argv)
{

#ifdef TIMING
#ifdef BSD_TIME_CALLS
  ftime(&timebuffer);
  timer = timebuffer.time + (float)(timebuffer.millitm) / 1000.0;
#else
  gettimeofday(&Tp, NULL);
  timer = Tp.tv_sec + ((float)(Tp.tv_usec))/ 1000000.0;
#endif
#endif

  setbuf(ERR_STREAM, NULL);
  (void) strcpy(G_progname, argv[0]);

  initcolor();
  DecodeArgs(argc, argv);
  vfstack[0] = dvi;

#ifdef KPATHSEA
  kpse_set_progname(argv[0]);
  kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_compile);
  kpse_init_prog("DVIPNG", resolution, MFMODE, "cmr10");
#endif


  if (!ParseStdin) {
    /* If pages not given on commandline, insert all pages */
    if (QueueEmpty()) {
      if (!Reverse) {
	QueuePage(1,MAXPAGE,_TRUE);
      } else {
	struct page_list *tpagelistp;
	
	tpagelistp=hpagelistp;
	while(tpagelistp!=NULL) {
	  SkipPage();
	  tpagelistp->next=InitPage();
	  tpagelistp=tpagelistp->next;
	}
	QueuePage(1,abspagenumber,_TRUE);
      }
    }
    DoPages();
  } else {
    char    line[STRSIZE];
    char    *linev[3];
    
    if (!QueueEmpty()) 
      DoPages();
    printf("%s> ",dvi->name);
    fgets(line,STRSIZE,stdin);
    while(!FEOF(stdin)) {
      linev[0]=line;  /* OBSERVE linev[0] is never used in DecodeArgs */
      while( *linev[0]!='\n' ) linev[0]++;
      *linev[0]='\0';
      linev[1]=line;
      linev[2]=line+2;
      while( *linev[2]!=' ' && *linev[2]!='\0' ) linev[2]++;
      while( *linev[2]==' ') *(linev[2]++)='\0';
      if (*linev[2]=='\0') {
	DecodeArgs(2,linev);
      } else {
	DecodeArgs(3,linev);
      }
      if (!QueueEmpty()) 
	DoPages();
      printf("%s> ",dvi->name);
      fgets(line,STRSIZE,stdin);
    }
  }

#ifdef TIMING
#ifdef BSD_TIME_CALLS
  ftime(&timebuffer);
  timer = (timebuffer.time + (float)(timebuffer.millitm)/1000.0) - timer;
#else
  gettimeofday(&Tp, NULL);
  timer = (Tp.tv_sec + (float)(Tp.tv_usec)/1000000.0) - timer;
#endif
  
  if (ndone > 0) {
    fprintf(ERR_STREAM,
	    "Time of complete run: %.2f s, %d page(s), %.2f s/page.\n",
	    timer, ndone, timer / ndone);
  }
  if (my_toc >= 0.01) {
    fprintf(ERR_STREAM,
	    "Thereof in TIC/TOC region %.2f s.\n",my_toc);
  }
#endif

  CloseFiles();
  exit(G_errenc);
}



