/* color.c */

/************************************************************************

  Part of the dvipng distribution

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.

  Copyright (C) 2002-2005 Jan-Åke Larsson

************************************************************************/

#include "dvipng.h"
#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

/*
 * Color. We delete and recreate the gdImage for each new page. This
 * means that the stack must contain rgb value not color index.
 * Besides, the current antialiasing implementation needs rgb anyway.
*/

struct colorname {
  char*             name;
  char*             color;
} *colornames=NULL;

void initcolor() 
{
   csp = 1;
   cstack[0].red=255; 
   cstack[0].green=255; 
   cstack[0].blue=255; 
   cstack[1].red=0; 
   cstack[1].green=0; 
   cstack[1].blue=0; 
}

void LoadDvipsNam (void)
{
  char *pos,*max,*buf,*dvipsnam_file;
  struct colorname *tmp=NULL;
  struct filemmap fmmap;

  TEMPSTR(dvipsnam_file,kpse_find_file("dvipsnam.def",kpse_tex_format,false));
  if (dvipsnam_file == NULL) {
    Warning("color name file dvipsnam.def could not be found");
    return;
  }
  DEBUG_PRINT(DEBUG_COLOR,("\n  OPEN COLOR NAMES:\t'%s'", dvipsnam_file));
  if (MmapFile(dvipsnam_file,&fmmap)) return;
  if ((colornames = malloc(fmmap.size*2))==NULL) 
    Fatal("cannot alloc space for color names");
  pos=fmmap.mmap;
  max=fmmap.mmap+fmmap.size;
  tmp=colornames;
  buf=(char*)colornames+fmmap.size;
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
    DEBUG_PRINT(DEBUG_COLOR,("\n  COLOR NAME:\t'%s' '%s'",
			     tmp->name,tmp->color)); 
    tmp++;
  }
  tmp->name=NULL;
  UnMmapFile(&fmmap);
}

void ClearDvipsNam(void)
{
  if (colornames!=NULL)
    free(colornames);
  colornames=NULL;
}

float toktof(char* token)
{
  if (token!=NULL)
    return(atof(token));
  flags |= PAGE_GAVE_WARN;
  Warning("Missing color-specification value, treated as zero\n");
  return(0.0);
}

void stringrgb(char* p,int *r,int *g,int *b)
{
  char* token;

  DEBUG_PRINT(DEBUG_COLOR,("\n  COLOR SPEC:\t'%s' ",p));
  token=strtok(p," ");
  if (strcmp(token,"Black")==0) {
    p+=5;
    *r = *g = *b = 0;
  } else if (strcmp(token,"White")==0) {
    p+=5;
    *r = *g = *b = 255;
  } else if (strcmp(token,"gray")==0) {
    p+=4;
    *r = *g = *b = (int) (255 * toktof(strtok(NULL," ")));
  } else if (strcmp(token,"rgb")==0) {
    p+=3;
    *r = (int) (255 * toktof(strtok(NULL," ")));
    *g = (int) (255 * toktof(strtok(NULL," ")));
    *b = (int) (255 * toktof(strtok(NULL," ")));
  } else if (strncmp(p,"cmyk",4)==0) {
    double c,m,y,k;

    p+=4;
    c = toktof(strtok(NULL," "));
    m = toktof(strtok(NULL," "));
    y = toktof(strtok(NULL," "));
    k = toktof(strtok(NULL," "));
    *r = (int) (255 * ((1-c)*(1-k)));
    *g = (int) (255 * ((1-m)*(1-k)));
    *b = (int) (255 * ((1-y)*(1-k)));
  } else {
    struct colorname *tmp;

    if (colornames==NULL) 
      LoadDvipsNam();
    tmp=colornames;
    while(tmp!=NULL && tmp->name!=NULL && strcmp(tmp->name,token)) 
      tmp++;
    if (tmp->name!=NULL) {
      /* One-level recursion */
      char* colorspec=alloca(sizeof(char)*strlen(tmp->color+1));
      strcpy(colorspec,tmp->color);
      stringrgb(colorspec,r,g,b);
    } else {
      char* t2=strtok(NULL,"");  
      if (t2!=NULL) 
	Warning("Unimplemented color specification '%s %s'\n",p,t2);
      else 
	Warning("Unimplemented color specification '%s'\n",p);
      flags |= PAGE_GAVE_WARN;
    }
  }
  DEBUG_PRINT(DEBUG_COLOR,("(%d %d %d) ",*r,*g,*b))
}

void background(char* p)
{
  stringrgb(p, &cstack[0].red, &cstack[0].green, &cstack[0].blue);
  DEBUG_PRINT(DEBUG_COLOR,("\n  BACKGROUND:\t(%d %d %d) ",
			   cstack[0].red, cstack[0].green, cstack[0].blue));
}

void pushcolor(char * p)
{
  if ( ++csp == STACK_SIZE )
    Fatal("Out of color stack space") ;
  stringrgb(p, &cstack[csp].red, &cstack[csp].green, &cstack[csp].blue);
  DEBUG_PRINT(DEBUG_COLOR,("\n  COLOR PUSH:\t(%d %d %d) ",
			   cstack[csp].red, cstack[csp].green, cstack[csp].blue))
}

void popcolor()
{
  if (csp > 1) csp--; /* Last color is global */
  DEBUG_PRINT(DEBUG_COLOR,("\n  COLOR POP\t"))
}

void resetcolorstack(char * p)
{
  if ( csp > 1 )
    Warning("Global color change within nested colors\n");
  csp=0;
  pushcolor(p) ;
  DEBUG_PRINT(DEBUG_COLOR,("\n  RESET COLOR:\tbottom of stack:"))
}

void StoreColorStack(struct page_list *tpagep)
{
  int i=0;

  DEBUG_PRINT(DEBUG_COLOR,("\n  STORE COLOR STACK:\t %d ", csp));
  tpagep->csp=csp;
  while ( i <= csp ) {
    DEBUG_PRINT(DEBUG_COLOR,("\n  COLOR STACK:\t %d (%d %d %d) ",i,
			     cstack[i].red, cstack[i].green, cstack[i].blue));
    tpagep->cstack[i].red = cstack[i].red;
    tpagep->cstack[i].green = cstack[i].green;
    tpagep->cstack[i].blue = cstack[i].blue;
    i++;
  }
}

void ReadColorStack(struct page_list *tpagep)
{
  int i=0;

  DEBUG_PRINT(DEBUG_COLOR,("\n  READ COLOR STACK:\t %d ", tpagep->csp));
  csp=tpagep->csp;
  while ( i <= tpagep->csp ) {
    DEBUG_PRINT(DEBUG_COLOR,("\n  COLOR STACK:\t %d (%d %d %d) ",i,
			     cstack[i].red, cstack[i].green, cstack[i].blue));
    cstack[i].red = tpagep->cstack[i].red;
    cstack[i].green = tpagep->cstack[i].green;
    cstack[i].blue = tpagep->cstack[i].blue;
    i++;
  }
}

void StoreBackgroundColor(struct page_list *tpagep)
{
  /* Background color changes affect the _whole_ page */
  tpagep->cstack[0].red = cstack[0].red;
  tpagep->cstack[0].green = cstack[0].green;
  tpagep->cstack[0].blue = cstack[0].blue;
}
