#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>

/*
 * Color. We delete and recreate the gdImage for each new page. This
 * means that the stack must contain rgb value not color index.
 * Besides, the current antialiasing implementation needs rgb anyway.
*/

static int cstack_red[STACK_SIZE];
static int cstack_green[STACK_SIZE]; 
static int cstack_blue[STACK_SIZE];
static int csp=0;

struct colorname {
  struct colorname* next;
  char*             name;
  char*             color;
} *colornames=NULL;

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

void LoadDvipsNam (void)
{
  char *pos,*max,*buf,*dvipsnam_file =
    kpse_find_file("dvipsnam.def",kpse_tex_format,false);
  int fd;
  struct colorname *tmp=NULL;
  struct stat stat;
  unsigned char* dvipsnam_mmap;
  
  if (dvipsnam_file == NULL) {
    Warning("color name file dvipsnam.def could not be found");
    return;
  }
  DEBUG_PRINT(DEBUG_COLOR,("\n  OPEN COLOR NAMES:\t'%s'", dvipsnam_file));
  if ((fd = open(dvipsnam_file,O_RDONLY)) == -1) {
    Warning("color name file %s could not be opened", dvipsnam_file);
    return;
  }
  fstat(fd,&stat);
  dvipsnam_mmap = mmap(NULL,stat.st_size, PROT_READ, MAP_SHARED,fd,0);
  if (dvipsnam_mmap == (unsigned char *)-1) {
    Warning("cannot mmap color name <%s> !\n",dvipsnam_file);
    return;
  }
  if ((buf = malloc(stat.st_size*2))==NULL) 
    Fatal("cannot alloc space for color names");
  pos=dvipsnam_mmap;
  max=dvipsnam_mmap+stat.st_size;
  tmp=(struct colorname*)buf;
  buf+=stat.st_size;
  while (pos<max && *pos!='\\') pos++;
  while(pos+9<max && strncmp(pos,"\\endinput",9)!=0) {
    while (pos+17<max && strncmp(pos,"\\DefineNamedColor",17)!=0) {
      pos++;
      while (pos<max && *pos!='\\') pos++;
    }
    while(pos<max && *pos!='}') pos++; /* skip first argument */
    while(pos<max && *pos!='{') pos++; /* find second argument */
    pos++;
    tmp->name=buf;
    while(pos<max && *pos!='}')        /* copy color name */
      *buf++=*pos++;
    *buf++='\0';
    while(pos<max && *pos!='{') pos++; /* find third argument */
    pos++;
    tmp->color=buf;
    while(pos<max && *pos!='}')        /* copy color model */
      *buf++=*pos++;
    *buf++=' ';
    while(pos<max && *pos!='{') pos++; /* find fourth argument */
    pos++;
    while(pos<max && *pos!='}') {      /* copy color values */
      *buf++=*pos++;
      if (*(buf-1)==',') *(buf-1)=' '; /* commas should become blanks */
    }
    *buf++='\0';
    while (pos<max && *pos!='\\') pos++;
    DEBUG_PRINT(DEBUG_COLOR,("\n  COLOR NAME '%s' '%s'",
		 tmp->name,tmp->color)); 
    tmp->next = colornames;
    colornames = tmp;
    tmp++;
  }
  if (munmap(dvipsnam_mmap,stat.st_size))
    Warning("cannot munmap color name file %s!?\n",dvipsnam_file);
  if (close(fd))
    Warning("cannot close color name file %s!?\n",dvipsnam_file);
}


void stringrgb(char* p,int *r,int *g,int *b)
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
  } else {
    struct colorname *tmp;

    if (colornames==NULL) 
      LoadDvipsNam();
    tmp=colornames;
    while(tmp!=NULL && strcmp(tmp->name,p)) 
      tmp=tmp->next;
    if (tmp!=NULL)
      /* One-level recursion */
      stringrgb(tmp->color,r,g,b);
    else {
      flags |= PAGE_GAVE_WARN;
      Warning("Unimplemented color specification '%s'\n",p);
    }
  }
}

void background(char* p)
{
  stringrgb(p, &bRed, &bGreen, &bBlue);
} 

void pushcolor(char * p)
{
  if ( ++csp == STACK_SIZE )
    Fatal("Out of color stack space") ;
  stringrgb(p, &Red, &Green, &Blue);
  cstack_red[csp] = Red; 
  cstack_green[csp] = Green; 
  cstack_blue[csp] = Blue; 
}

void popcolor()
{
  if (csp > 0) csp--; /* Last color is global */
  Red = cstack_red[csp];
  Green = cstack_green[csp];
  Blue = cstack_blue[csp];
}

void resetcolorstack(char * p)
{
  if ( csp > 0 )
    Warning("Global color change within nested colors\n");
  csp=-1;
  pushcolor(p) ;
}
