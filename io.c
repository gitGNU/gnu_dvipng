#include "dvipng.h"
/* This function closes all open files */
void
#if NeedFunctionPrototypes
CloseFiles(void)
#else
CloseFiles()
#endif
{
  struct font_entry *fe;
  FILEPTR f;

  /* First input/output files */
  /*  if (outfp != FPNULL) {
    BCLOSE(outfp);
  }
  if (dvifp != FPNULL) {
    BCLOSE(dvifp);
    }*/
#ifdef __riscos
  if (metafile != FPNULL) {
    BCLOSE(metafile);
  }
#endif
  /* Now all open font files */
  fe = hfontptr;
  while (fe != NULL) {
    f = fe->font_file_id;
    if ((f != FPNULL) && (f != NO_FILE)) {
      BCLOSE(f);
    }
    fe = fe->next;
  }
}

/*-->NoSignExtend*/
/**********************************************************************/
/***************************  NoSignExtend  ***************************/
/**********************************************************************/
long4
#if NeedFunctionPrototypes
NoSignExtend(FILEPTR fp, register int n)
#else
NoSignExtend(fp, n)     /* return n byte quantity from file fd */
register FILEPTR fp;    /* file pointer    */
register int    n;      /* number of bytes */
#endif
{
  long4 x = 0;      /* number being constructed */
  unsigned char h;
  while (n--) {
    x <<= 8; 
    read_byte(fp,h);
    x |= h;
  }
  /* fprintf(stderr,"[%ld] ",(long)x);*/
  return(x);
}


#ifndef ARITHMETIC_RIGHT_SHIFT
long4   signTab[5] = {0,0x00000080,0x00008000,0x00800000,0x00000000};
long4 extendTab[5] = {0,~0^0xff,~0^0xffff,~0^0xffffff,~0^0xffffffff};
#endif

/*-->SignExtend*/
/**********************************************************************/
/****************************  SignExtend  ****************************/
/**********************************************************************/
long4
#if NeedFunctionPrototypes
SignExtend(FILEPTR fp, register int n)
#else
SignExtend(fp, n)     /* return n byte quantity from file fd */
register FILEPTR fp;  /* file pointer    */
register int     n;   /* number of bytes */
#endif
{
  int     n1;       /* number of bytes      */
  long4   x;        /* number being constructed */
  unsigned char    h;
#ifdef SIGN_DEBUG
  long4    x0;      /* copy of x  */
#endif
  read_byte(fp,h); 
  x = h; /* get first (high-order) byte */
  n1 = n--;
  while (n--) {
    x <<= 8;
    read_byte(fp,h);
    x |= h;
  }
  /*
   *   NOTE: This code assumes that the right-shift is an arithmetic, rather
   *   than logical, shift which will propagate the sign bit right.   According
   *   to Kernighan and Ritchie, this is compiler dependent!
   */

#ifdef SIGN_DEBUG
  x0 = x;
#endif

#ifdef ARITHMETIC_RIGHT_SHIFT
  x <<= 32 - 8 * n1;
  x >>= 32 - 8 * n1; /* sign extend */
#else
  if (x & signTab[n1]) x |= extendTab[n1];
#endif

#ifdef SIGN_DEBUG
  fprintf(ERR_STREAM,"\tSignExtend(fp,%d)=%lX, was=%lX,%d\n",
          n1,x,x0,x0&signTab[n1]);
#endif

#ifdef DEBUG
  if (Debug > 1)
    fprintf(ERR_STREAM,"\tSignExtend(fp,%d)=%lX\n", n1, x);
#endif
  return(x);
}
