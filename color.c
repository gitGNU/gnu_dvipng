#include "dvipng.h"

/*
 * Color. We delete and recreate the gdImage for each new page. This
 * means that the stack must contain rgb value not color
 * index. Besides, the current antialiasing implementation needs rgb
 * anyway.
*/

static int cstack_red[STACK_SIZE];
static int cstack_green[STACK_SIZE]; 
static int cstack_blue[STACK_SIZE];
static int csp=0;

void initcolor() 
{
   csp = 0;
   cstack_red[0]=0; 
   cstack_green[0]=0; 
   cstack_blue[0]=0; 
   Red=0;
   Green=0;
   Blue=0;
}

void
stringrgb(p,r,g,b)
     char *p;
     int *r,*g,*b;
{
  if (strncmp(p,"Black",5)==0) {
    p+=5;
    *r = *g = *b = 0;
  } else if (strncmp(p,"White",5)==0) {
    p+=5;
    *r = *g = *b = 255;
  } else if (strncmp(p,"gray",4)==0) {
    p+=4;
    *r = *g = *b = (int) (255 * strtod(p,&p));
  } else if (strncmp(p,"rgb",3)==0) {
    p+=3;
    *r = (int) (255 * strtod(p,&p));
    *g = (int) (255 * strtod(p,&p));
    *b = (int) (255 * strtod(p,&p));
  } else if (strncmp(p,"cmyk",4)==0) {
    double c,m,y,k;

    p+=4;
    c = strtod(p,&p);
    m = strtod(p,&p);
    y = strtod(p,&p);
    k = strtod(p,&p);
    *r = (int) (255 * ((1-c)*(1-k)));
    *g = (int) (255 * ((1-m)*(1-k)));
    *b = (int) (255 * ((1-y)*(1-k)));
  } else
    Warning("Unimplemented color specification '%s'\n",p);
}

void
background(p)
     char* p;
{
  stringrgb(p, &bRed, &bGreen, &bBlue);
  if (page_imagep) {
    DoBop(); 
    Warning("Setting background inside page will clear the image, sorry.");
    /*FSEEK(dvi->filep,cpagep,SEEK_SET);*/ /* Restart page output */
    /* GdImageFilledRectangle(...); will take forever to finish and
      will clear the image */
    /* The below probably works only on color-indexed gdImages, and
       doesn't work with antialiasing */
    /*page_imagep->red[0]=bRed;
    page_imagep->green[0]=bGreen;
    page_imagep->blue[0]=bBlue;*/
  }
} 

void
pushcolor(p)
     char * p;
{
  if ( ++csp == STACK_SIZE )
    Fatal("Out of color stack space") ;
  stringrgb(p, &Red, &Green, &Blue);
  cstack_red[csp] = Red; 
  cstack_green[csp] = Green; 
  cstack_blue[csp] = Blue; 
}

void
popcolor()
{
  if (csp > 0) csp--; /* Last color is global */
  Red = cstack_red[csp];
  Green = cstack_green[csp];
  Blue = cstack_blue[csp];
}

void
resetcolorstack(p)
     char * p;
{
  if ( csp > 0 )
    Warning("Global color change within nested colors\n");
  csp=-1;
  pushcolor(p) ;
}
