#include "dvipng.h"
/* This function closes all open files */
void CloseFiles P1H(void)
{
  struct font_entry *fe;

  /*
    if (dvi->filep != NULL) {
    fclose(dvi->filep);
    }*/
#ifdef __riscos
  if (metafile != NULL) {
    fclose(metafile);
  }
#endif
  /* Now all open font files */
  fe = hfontptr;
  while (fe != NULL) {
    /*    f = fe->filep;
    if ((f != NULL) && (f != NO_FILE)) {
      fclose(f);
      }*/
    close(fe->filedes);
    fe = fe->next;
  }
}

uint32_t UNumRead P2C(unsigned char*, current, register int, n)
{
  uint32_t x = (unsigned char) *(current)++;  /* number being constructed */

  while(--n) {
    x = (x << 8) | *(current)++;
  }
  return(x);
}

int32_t SNumRead P2C(unsigned char*, current, register int, n)
{
  int32_t	x;
  
#if	__STDC__
  x = (signed char) *(current)++;      /* number being constructed */
#else
  x = *(current)++;      /* number being constructed */
  if (x & 0x80) x -= 0x100;
#endif
  while(--n) { 
    x = (x << 8) | *(current)++;
  }
  return(x);
}


