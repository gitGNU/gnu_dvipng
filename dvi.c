#include "dvipng.h"
#include <libgen.h>

struct dvi_data* DVIOpen(char* dviname)
{
  int     k;
  unsigned char* pre;
  char* tmpstring;
  struct dvi_data* dvi;

  if ((dvi = malloc(sizeof(struct dvi_data)))==NULL)
    Fatal("cannot allocate memory for dvi struct");

  dvi->type = DVI_TYPE;
  dvi->fontnump=NULL;

  strcpy(dvi->name,dviname);

  /* split into file name + extension */
  strcpy(dvi->outname,basename(dviname));
  tmpstring = strrchr(dvi->outname, '.');
  if (tmpstring == NULL) {
    strcat(dvi->name, ".dvi");
  } else {
    *tmpstring='\0';
  }

  if ((dvi->filep = fopen(dvi->name,"rb")) == NULL) {
    /* do not insist on .dvi */
    tmpstring = strrchr(dvi->name, '.');
    *tmpstring='\0';
    if ((dvi->filep = fopen(dvi->name,"rb")) == NULL) {
      perror(dvi->name);
      exit (EXIT_FAILURE);
    }
  }
  DEBUG_PRINTF(DEBUG_DVI,"OPEN FILE\t%s", dvi->name);
  pre=DVIGetCommand(dvi);
  if (*pre != PRE) {
    Fatal("PRE doesn't occur first--are you sure this is a DVI file?\n\n");
  }
  k = UNumRead(pre+1,1);
  DEBUG_PRINTF(DEBUG_DVI,"DVI START:\tPRE %d",k);
  if (k != DVIFORMAT) {
    Fatal("DVI format = %d, can only process DVI format %d files\n\n",
	  k, DVIFORMAT);
  }
  dvi->num = UNumRead(pre+2, 4);
  dvi->den = UNumRead(pre+6, 4);
  DEBUG_PRINTF2(DEBUG_DVI," %d/%d",dvi->num,dvi->den);  
  dvi->mag = UNumRead(pre+10, 4); /*FIXME, see font.c*/
  DEBUG_PRINTF(DEBUG_DVI," %d",dvi->mag);  
  if ( usermag > 0 && usermag != dvi->mag ) {
    Warning("DVI magnification of %d over-ridden by user (%ld)",
	    (long)dvi->mag, usermag );
    dvi->mag = usermag;
  }
  dvi->conv = (1.0/(((double)dvi->num / (double)dvi->den) *
		    ((double)dvi->mag / 1000.0) *
		    ((double)resolution/254000.0)))+0.5;
  DEBUG_PRINTF(DEBUG_DVI," (%d)",dvi->conv);
  k = UNumRead(pre+14,1);
  DEBUG_PRINTF2(DEBUG_DVI," '%.*s'",k,pre+15);
  if (G_verbose)
    printf("'%.*s' -> %s#.png\n",k,pre+15,dvi->outname);
  return(dvi);
}

unsigned char* DVIGetCommand(struct dvi_data* dvi)
     /* This function reads in and stores the next dvi command. */
     /* Perhaps mmap would be appropriate here */
{ 
  static unsigned char command[STRSIZE];
  static unsigned char* lcommand = command;
  int length;
  uint32_t strlength=0;

  DEBUG_PRINTF(DEBUG_DVI,"\n@%ld ", ftell(dvi->filep));
  *command = fgetc(dvi->filep);
  length = dvi_commandlength[*command];
  if (length < 0)
    Fatal("undefined dvi op-code %d",*command);
  if (fread(command+1, 1, length-1, dvi->filep) < length-1) 
    Fatal("%s command shorter than expected",dvi_commands[*command]);
  switch (*command)  {
  case XXX4:
    strlength = *(command + 1);
  case XXX3:
    strlength = strlength * 256 + *(command + length - 3);
  case XXX2: 
    strlength = strlength * 256 + *(command + length - 2);
  case XXX1:
    strlength = strlength * 256 + *(command + length - 1);
    /*    strlength = UNumRead((char*)command+1, length-1);*/
    break;
  case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
    strlength = *(command + length - 1) + *(command + length - 2);
    break;
  case PRE: 
    strlength = *(command + length - 1);
    break;
  }
  if (lcommand!=command) {
    free(lcommand);
    lcommand=command;
  }
  if (strlength > 0) { /* Read string */
    if (strlength + (uint32_t)length >  (uint32_t)STRSIZE) {
      /* string + command length exceeds that of buffer */
      if ((lcommand=malloc(length+strlength))==NULL) 
	Fatal("cannot allocate memory for dvi command");
      memcpy(lcommand,command,length);
    }
    if (fread(lcommand+length,1,strlength,dvi->filep) < strlength) 
      Fatal("%s command shorter than expected",dvi_commands[*command]);
  }
  return(lcommand);
}


uint32_t CommandLength(unsigned char* command)
{ 
  /* generally 2^32+5 bytes max, but in practice 32 bit numbers suffice */
  unsigned char*   current;           
  uint32_t length=0;

  current = command+1;
  length = dvi_commandlength[*command];
  switch (*command)  {
  case XXX1: case XXX2: case XXX3: case XXX4: 
    length += UNumRead((char*)command + 1,length - 1);
    break;
  case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4: 
    length += *(command + length - 1) + *(command + length - 2);
    break;
  case PRE: 
    length += *(command + length - 1);
    break;
  }
  return(length);
}

void SkipPage(void)
{ 
  /* Skip present page */
  unsigned char* command;           

  command=DVIGetCommand(dvi);
  while (*command != EOP)  {
    switch (*command)  {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      DEBUG_PRINTF(DEBUG_DVI,"NOSKIP CMD:\t%s", dvi_commands[*command]);
      FontDef(command,(struct dvi_vf_entry*)dvi);
      break;
    case BOP: case PRE: case POST: case POST_POST:
      Fatal("%s occurs within page", dvi_commands[*command]);
      break;
#ifdef DEBUG
    default:
      DEBUG_PRINTF(DEBUG_DVI,"SKIP CMD:\t%s", dvi_commands[*command]);
#endif
    }
    command=DVIGetCommand(dvi);
  } /* while */
  DEBUG_PRINTF(DEBUG_DVI,"SKIP CMD:\t%s", dvi_commands[*command]);
}

struct page_list* InitPage(void)
{
  /* Find page start, return pointer to page_list entry if found */
  struct page_list* tpagelistp=NULL;
  unsigned char* command;           

  command=DVIGetCommand(dvi);
  /* Skip until page start or postamble */
  while((*command != BOP) && (*command != POST)) {
    switch(*command) {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      DEBUG_PRINTF(DEBUG_DVI,"NOPAGE CMD:\t%s", dvi_commands[*command]);
      FontDef(command,(struct dvi_vf_entry*)dvi);
      break;
    case NOP:
      DEBUG_PUTS(DEBUG_DVI,"NOPAGE CMD:\tNOP");
      break;
    default:
      Fatal("%s occurs outside page", dvi_commands[*command]);
    }
    command=DVIGetCommand(dvi);
  }
  if ((tpagelistp = malloc(sizeof(struct page_list)))==NULL)
    Fatal("cannot allocate memory for new page entry");
  tpagelistp->next = NULL;
  if ( *command == BOP ) {  /*  Init page */
    int i;
    DEBUG_PUTS(DEBUG_DVI,"PAGE START:\tBOP");
    tpagelistp->offset = ftell(dvi->filep)-45;
    for (i = 0; i <= 9; i++) {
      tpagelistp->count[i] = UNumRead(command + 1 + i*4, 4);
      DEBUG_PRINTF(DEBUG_DVI," %d",tpagelistp->count[i]);
    }
    tpagelistp->count[10] = ++abspagenumber;
    DEBUG_PRINTF(DEBUG_DVI," (%d)",abspagenumber);
  } else {
    DEBUG_PUTS(DEBUG_DVI,"DVI END:\tPOST");
    tpagelistp->offset = ftell(dvi->filep)-1;
    tpagelistp->count[10] = -1; /* POST */
  }
  return(tpagelistp);
}

struct page_list* FindPage(int32_t pagenum, bool abspage)
     /* Find page of certain number, absolute number if Abspage is set */
{
  struct page_list* tpagelistp;
  int index;
  
  index = abspage ? 10 : 0 ;
#ifdef DEBUG
  if (abspage) {
    DEBUG_PRINTF(DEBUG_DVI,"\n  FIND PAGE:\t(%d)",pagenum);
  } else {
    DEBUG_PRINTF(DEBUG_DVI,"\n  FIND PAGE:\t%d",pagenum);
  }
#endif
  /* If we have read past the last page in our current list or the
   *  list is empty, look at the next page
   */
  if (hpagelistp==NULL || hpagelistp->offset < ftell(dvi->filep)) {
    tpagelistp=hpagelistp;
    hpagelistp=InitPage();
    hpagelistp->next=tpagelistp;
  }
  if (hpagelistp==NULL)    
      Fatal("no pages in %s",dvi->name);
  /* Check if page is in list */
  tpagelistp = hpagelistp;
  while(tpagelistp!=NULL && tpagelistp->count[index]!=pagenum) {
    tpagelistp = tpagelistp->next;
  }
  /* If not, skip current page and look at the next page */
  if (tpagelistp==NULL) {
    while(hpagelistp->count[index]!=pagenum && hpagelistp->count[10]!=-1) {
      SkipPage();
      tpagelistp=hpagelistp;
      hpagelistp=InitPage();
      hpagelistp->next=tpagelistp;
    }    
    tpagelistp=hpagelistp;
  }
  /* Have we found it? */
  if (tpagelistp->count[index]==pagenum) {
    if (pagenum==PAGE_POST) {
      fseek(dvi->filep, tpagelistp->offset+1L, SEEK_SET);
     } else {
       fseek(dvi->filep, tpagelistp->offset+45L, SEEK_SET);
     }
  } else /* we're at POST, are we trying to find the last page? */
    if (pagenum==PAGE_LASTPAGE) {
      tpagelistp=hpagelistp->next;
      fseek(dvi->filep, tpagelistp->offset+45L, SEEK_SET);
    } else {
      tpagelistp=NULL;
      /*	Warning("page %d not found",pagenum);*/
    }
  return(tpagelistp);
}

void DVIClose(struct dvi_data* dvi)
{
  struct page_list* temp;
  
  /* Delete the page list */

  temp=hpagelistp;
  while(hpagelistp!=NULL) {
    hpagelistp=hpagelistp->next;
    free(temp);
    temp=hpagelistp;
  }
  abspagenumber=0;

  fclose(dvi->filep);
  free(dvi);
}

