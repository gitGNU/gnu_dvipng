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
 *      #define DEBUG      for massive printing of trace information
 *                         when -d cmdline option specified
 **********************************************************************
 */

#define MAIN
#include "dvipng.h"

/**********************************************************************/
/*******************************  main  *******************************/
/**********************************************************************/
int main(int argc, char ** argv)
{
  bool parsestdin;
  
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

  initcolor();
  parsestdin=DecodeArgs(argc, argv);

#ifdef KPATHSEA
  kpse_set_progname(argv[0]);
  kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_compile);
  kpse_init_prog("DVIPNG", resolution, MFMODE, "cmr10");
#endif

  DoPages();

  if (parsestdin) {
    char    line[STRSIZE];

    printf("%s> ",dvi!=NULL?dvi->name:"");
    fgets(line,STRSIZE,stdin);
    while(!feof(stdin)) {
      DecodeString(line);
      if (dvi!=NULL) {
	DVIReOpen(dvi);
	DoPages();
      }
      printf("%s> ",dvi!=NULL?dvi->name:"");
      fgets(line,STRSIZE,stdin);
    }
    printf("\n");
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

  exit(EXIT_SUCCESS);
}



