#include "dvipng.h"
#include <libgen.h>
#include <sys/stat.h>

void DVIInit(struct dvi_data* dvi)
{
  int     k;
  unsigned char* pre;
  struct stat stat;

  fseek(dvi->filep,0,SEEK_SET);
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
  Message(BE_VERBOSE,"'%.*s' -> %s#.png\n",k,pre+15,dvi->outname);
  fstat(fileno(dvi->filep), &stat);
  dvi->mtime = stat.st_mtime;
}

struct dvi_data* DVIOpen(char* dviname,char* outname)
{
  char* tmpstring;
  struct dvi_data* dvi;

  if ((dvi = malloc(sizeof(struct dvi_data)))==NULL)
    Fatal("cannot allocate memory for dvi struct");

  dvi->type = DVI_TYPE;
  dvi->fontnump=NULL;

  strcpy(dvi->name,dviname);
  tmpstring = strrchr(dvi->name, '.');
  if (tmpstring == NULL) 
    strcat(dvi->name, ".dvi");

  /* split into file name + extension */
  if (outname==NULL) 
    strcpy(dvi->outname,basename(dviname));
  else
    strcpy(dvi->outname,basename(outname));
  tmpstring = strrchr(dvi->outname, '.');
  if (tmpstring != NULL) 
    *tmpstring='\0';

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
  DVIInit(dvi);
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
    if (dvi->pagelistp==NULL)
      tpagelistp->count[10] = 1;
    else
      tpagelistp->count[10] = dvi->pagelistp->count[10]+1;
    DEBUG_PRINTF(DEBUG_DVI," (%d)", tpagelistp->count[10]);
  } else {
    DEBUG_PUTS(DEBUG_DVI,"DVI END:\tPOST");
    tpagelistp->offset = ftell(dvi->filep)-1;
    tpagelistp->count[0] = PAGE_POST; /* POST */
  }
  return(tpagelistp);
}

void SeekPage(struct page_list* page)
{
  if (page->count[0]==PAGE_POST) {
    fseek(dvi->filep, page->offset+1L, SEEK_SET);
  } else {
    fseek(dvi->filep, page->offset+45L, SEEK_SET);
  }
}

struct page_list* NextPage(struct page_list* page)
{
  struct page_list* tpagelistp;

  /* if page points to POST there is no next page */
  if (page!=NULL && page->count[0]==PAGE_POST) 
    return(NULL);

  /* If we have read past the last page in our current list or the
   *  list is empty, sneak a look at the next page
   */
  if (dvi->pagelistp==NULL 
      || dvi->pagelistp->offset+45L < ftell(dvi->filep)) {
    tpagelistp=dvi->pagelistp;
    if ((dvi->pagelistp=InitPage())==NULL)    
      Fatal("no pages in %s",dvi->name);
    dvi->pagelistp->next=tpagelistp;
  }

  if (page!=dvi->pagelistp) {
    /* also works if page==NULL, we'll get the first page then */
    tpagelistp=dvi->pagelistp;
    while(tpagelistp!=NULL && tpagelistp->next!=page)
      tpagelistp=tpagelistp->next;
  } else {
    /* dvi->pagelistp points to the last page we've read so far,
     * the last page that we know where it is, so to speak
     * So look at the next
     */
    SeekPage(dvi->pagelistp);
    SkipPage();
    tpagelistp=dvi->pagelistp;
    dvi->pagelistp=InitPage();
    dvi->pagelistp->next=tpagelistp;
    tpagelistp=dvi->pagelistp;
  }
  return(tpagelistp);
}

struct page_list* PrevPage(struct page_list* page)
{
  return(page->next);
}


struct page_list* FindPage(int32_t pagenum, bool abspage)
     /* Find first page of certain number, 
	absolute number if abspage is set */
{
  struct page_list* page=NextPage(NULL);
  
  if (pagenum==PAGE_LASTPAGE || pagenum==PAGE_POST) {
    while(page!=NULL && page->count[0]!=PAGE_POST)
      page=NextPage(page);
    if (pagenum==PAGE_LASTPAGE)
      page=PrevPage(page);
  } else
    if (pagenum!=PAGE_FIRSTPAGE) 
      while(page != NULL && pagenum != page->count[abspage ? 0 : 10])
	page=NextPage(page);
  return(page);
}


void DelPageList(struct dvi_data* dvi)
{
  struct page_list* temp;
  
  /* Delete the page list */

  temp=dvi->pagelistp;
  while(dvi->pagelistp!=NULL) {
    dvi->pagelistp=dvi->pagelistp->next;
    free(temp);
    temp=dvi->pagelistp;
  }
}

void DVIClose(struct dvi_data* dvi)
{
  DelPageList(dvi);
  fclose(dvi->filep);
  free(dvi);
}

void DVIReOpen(struct dvi_data* dvi)
{
  struct stat stat;
  fstat(fileno(dvi->filep), &stat);
  if (dvi->mtime != stat.st_mtime) {
    fclose(dvi->filep);
    if ((dvi->filep = fopen(dvi->name,"rb")) == NULL) {
      perror(dvi->name);
      exit (EXIT_FAILURE);
    }
    Message(PARSE_STDIN,"Reopened file\n");
    DEBUG_PRINTF(DEBUG_DVI,"\nREOPEN FILE\t%s", dvi->name);
    DelPageList(dvi);
    DVIInit(dvi);
  }
}
