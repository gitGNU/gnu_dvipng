#include "dvipng.h"
/*-->FindPostAmblePtr*/
/**********************************************************************/
/************************  FindPostAmblePtr  **************************/
/**********************************************************************/
/* this routine will move to the end of the file and find the start
    of the postamble */
#define	TRAILER	223	
#if NeedFunctionPrototypes
void FindPostAmblePtr(long *postambleptr)
#else
void FindPostAmblePtr(postambleptr)
long *postambleptr;
#endif
{
  long4    i;
  int k;

  FSEEK(dvifp,  (long)-4, SEEK_END);   /* goto end of file */
  *postambleptr = FTELL(dvifp);/* - 4;
				 FSEEK(dvifp, *postambleptr, SEEK_SET);*/
  while (_TRUE) {
    FSEEK(dvifp, --(*postambleptr), SEEK_SET);
    if (((i = NoSignExtend(dvifp, 1)) != TRAILER) &&
        (i != DVIFORMAT))
      Fatal("Bad end of DVI file");
    if (i == DVIFORMAT)
      break;
  }
  FSEEK(dvifp, (*postambleptr) - 4, SEEK_SET);
  (*postambleptr) = NoSignExtend(dvifp, 4);
  FSEEK(dvifp, *postambleptr, SEEK_SET);
}

/*-->ReadPostAmble*/
/**********************************************************************/
/**************************  ReadPostAmble  ***************************/
/**********************************************************************/
/***********************************************************************
    This routine is used to read in the postamble values.  It
    initializes the magnification and checks the stack height prior to
    starting printing the document.
***********************************************************************/
#if NeedFunctionPrototypes
void ReadPostAmble(bool load)
#else
void ReadPostAmble(load)
bool load;
#endif
{
  FindPostAmblePtr(&postambleptr);
  if (NoSignExtend(dvifp, 1) != POST)
    Fatal("POST missing at head of postamble");
#ifdef DEBUG
  if (Debug)
    fprintf(ERR_STREAM,"got POST command\n");
#endif
  ppagep = (long)NoSignExtend(dvifp, 4);
  num = NoSignExtend(dvifp, 4);
  den = NoSignExtend(dvifp, 4);
  mag = NoSignExtend(dvifp, 4);
  if ( usermag > 0 && usermag != mag )
    Warning("DVI magnification of %ld over-ridden by user (%ld)",
            (long)mag, usermag );
  if ( usermag > 0 )
    mag = usermag;
  hconv = DoConv(num, den, hconvRESOLUTION);
  vconv = DoConv(num, den, vconvRESOLUTION);
  (void) NoSignExtend(dvifp, 4);   /* height-plus-depth of tallest page */
  (void) NoSignExtend(dvifp, 4);   /* width of widest page */
  if (NoSignExtend(dvifp, 2) >= STACK_SIZE)
    Fatal("Stack size is too small");
  (void) NoSignExtend(dvifp, 2);   /* this reads the number of pages in */
  /* the DVI file */
#ifdef DEBUG
  if (Debug)
    fprintf(ERR_STREAM,"now reading font defs");
#endif
  if (load)
    GetFontDef();
}


