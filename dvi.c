#include "dvipng.h"
#include <libgen.h>

#define dvi_data font_entry

struct dvi_data* DVIOpen P1C(char*, dviname)
{
  int     k;
  unsigned char* pre;
  char* tmpstring;
  struct dvi_data* dvi;

  if ((dvi = malloc(sizeof(struct dvi_data)))==NULL)
    Fatal("cannot allocate memory for dvi struct");

  dvi->d = 65536; /* Natural scaling on dvi lengths */
  dvi->hfontnump=NULL;

  strcpy(dvi->name,dviname);

  /* split into file name + extension */
  strcpy(dvi->n,basename(dviname));
  tmpstring = strrchr(dvi->n, '.');
  if (tmpstring == NULL) {
    strcat(dvi->name, ".dvi");
  } else {
    *tmpstring='\0';
  }

  if ((dvi->filep = BINOPEN(dvi->name)) == FPNULL) {
    /* do not insist on .dvi */
    tmpstring = strrchr(dvi->name, '.');
    *tmpstring='\0';
    if ((dvi->filep = BINOPEN(dvi->name)) == FPNULL) {
      perror(dvi->name);
      exit (EXIT_FAILURE);
    }
  }
#ifdef DEBUG
  if (Debug)
    printf("OPEN FILE\t%s\n", dvi->name);
#endif
  
  pre=DVIGetCommand(dvi);
  if (*pre != PRE) {
    Fatal("PRE doesn't occur first--are you sure this is a DVI file?\n\n");
  }
#ifdef DEBUG
  if (Debug)
    printf("DVI START:\tPRE ");
#endif
  
  k = UNumRead(pre+1,1);
  if (k != DVIFORMAT) {
    Fatal("DVI format = %d, can only process DVI format %d files\n\n",
	  k, DVIFORMAT);
  }
  num = UNumRead(pre+2, 4);
  den = UNumRead(pre+6, 4);
  mag = UNumRead(pre+10, 4);
  if ( usermag > 0 && usermag != mag )
    Warning("DVI magnification of %d over-ridden by user (%ld)",
	    (long)mag, usermag );
  if ( usermag > 0 )
    mag = usermag;
  conv = DoConv(num, den, resolution);
  
  k = UNumRead(pre+14,1);
#ifdef DEBUG
  if (Debug)
    printf("'%.*s'\n",k,pre+15);
#endif
  if (G_verbose)
    printf("'%.*s' -> %s#.png\n",k,pre+15,dvi->n);

  return(dvi);
}

unsigned char* DVIGetCommand P1C(struct dvi_data*, dvi)
     /* This function reads in and stores the next dvi command. */
{ 
  static unsigned char command[STRSIZE];
  static unsigned char* lcommand = command;
  int length;
  uint32_t strlength=0;

#ifdef DEBUG
  if (Debug)
    printf("@%ld ", ftell(dvi->filep));
#endif

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
    strlength = *(command + length-1) + *(command+length-2);
    break;
  case PRE: 
    strlength = *(command+length-1);
    break;
  default:
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


uint32_t CommandLength P1C(unsigned char*, command)
{ 
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
  default:
  }
  return(length);
}

void SkipPage P1H(void)
{ 
  /* Skip present page */
  unsigned char* command;           

  command=DVIGetCommand(dvi);
  while (*command != EOP)  {
    switch (*command)  {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
#ifdef DEBUG
      if (Debug)
	printf("NOSKIP CMD:\t%s", dvi_commands[*command]);
#endif
      FontDef(command,dvi);
#ifdef DEBUG
      if (Debug)
	printf("\n");
#endif
      break;
    case BOP: case PRE:    case POST:    case POST_POST:
      Fatal("%s occurs within page", dvi_commands[*command]);
      break;
    default:
#ifdef DEBUG
      if (Debug)
	printf("SKIP CMD:\t%s\n", dvi_commands[*command]);
#endif
    }
    command=DVIGetCommand(dvi);
  } /* while */
#ifdef DEBUG
  if (Debug)
    printf("SKIP CMD:\t%s\n", dvi_commands[*command]);
#endif
}

struct page_list* InitPage P1H(void)
{
  /* Find page start, return pointer to page_list entry if found */
  struct page_list* tpagelistp=NULL;
  unsigned char* command;           

  command=DVIGetCommand(dvi);
  /* Skip until page start or postamble */
  while((*command != BOP) && (*command != POST)) {
    switch(*command) {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
#ifdef DEBUG
      if (Debug)
	printf("NOPAGE CMD:\t%s ", dvi_commands[*command]);
#endif
      FontDef(command,dvi);
#ifdef DEBUG
      if (Debug)
	printf("\n");
#endif
      break;
    case NOP:
#ifdef DEBUG
      if (Debug)
	printf("NOPAGE CMD:\tNOP\n");
#endif
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
#ifdef DEBUG
    if (Debug)
      printf("PAGE START:\tBOP ");
#endif
    tpagelistp->offset = FTELL(dvi->filep)-45;
    for (i = 0; i <= 9; i++) {
      tpagelistp->count[i] = UNumRead(command + 1 + i*4, 4);
    }
    tpagelistp->count[10] = ++abspagenumber;
#ifdef DEBUG
    if (Debug)
      printf("(absno=%d)\n",abspagenumber);
#endif
  } else {
#ifdef DEBUG
    if (Debug)
      printf("DVI END:\tPOST\n");
#endif
    tpagelistp->offset = FTELL(dvi->filep)-1;
    tpagelistp->count[10] = -1; /* POST */
  }
  return(tpagelistp);
}

struct page_list* FindPage P1C(int32_t,pagenum)
     /* Find page of certain number, absolute number if Abspage is set */
{
  struct page_list* tpagelistp;
  int index;
  
  /* If no page is to be found, return no page */
  if (pagenum==MAXPAGE)
    return(NULL);

  index = Abspage ? 10 : 0 ;

  /* Check if page is in list (stop on last page in list anyway) */
  tpagelistp = hpagelistp;
  while(tpagelistp->next!=NULL && tpagelistp->count[index]!=pagenum) {
    tpagelistp = tpagelistp->next;
  }
  /* Last page may be pointer to POST */
  if (tpagelistp->count[10]==-1) {
    fseek(dvi->filep, tpagelistp->offset+1L, SEEK_SET);
  } else {
    fseek(dvi->filep, tpagelistp->offset+45L, SEEK_SET);
    /* If page not yet found, skip in file until found */
    while(tpagelistp!=NULL && tpagelistp->count[index]!=pagenum) {
      SkipPage();
      tpagelistp->next=InitPage();
      tpagelistp = tpagelistp->next;
    }
  }

#ifdef DEBUG
  if (Debug)
    if (tpagelistp!=NULL)
      printf("FIND PAGE@%d:\tcount0=%d\t(absno=%d)\n", tpagelistp->offset,
	     tpagelistp->count[0],tpagelistp->count[10]);
#endif

  return(tpagelistp);
}

void DVIClose P1C(struct dvi_data*, dvi)
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

