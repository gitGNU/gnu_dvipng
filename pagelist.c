#include "dvipng.h"

void SkipPage P1H(void)
{ 
  /* Skip present page */
  unsigned char   command[STRSIZE];           

  ReadCommand(command);
#ifdef DEBUG
  if (Debug)
    printf("SKIP CMD:\t%d\n", *command);
#endif
  while (*command != EOP)  {
    switch (*command)  {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      FontDef(command,&dvi);
      break;
    case BOP:
      Fatal("BOP occurs within page");
      break;
    case PRE:
      Fatal("PRE occurs within page");
      break;
    case POST:
      Fatal("POST occurs within page");
	break;
    case POST_POST:
      Fatal("POST_POST occurs within page");
      break;
    default:
    }
    ReadCommand(command);
#ifdef DEBUG
    if (Debug)
      printf("SKIP CMD:\t%d\n", *command);
#endif
  } /* while */
}

struct page_list* InitPage P1H(void)
{
  /* Find page start, return pointer to page_list entry if found */
  struct page_list* tpagelistp=NULL;
  unsigned char   command[STRSIZE];           

  ReadCommand(command);
#ifdef DEBUG
  if (Debug)
    printf("INIT CMD:\t%d\n", *command);
#endif

  /* Skip until page start or postamble */
  while((*command != BOP) && (*command != POST)) {
    switch(*command) {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      FontDef(command,&dvi);
      break;
    case NOP: case BOP: case POST:
      break;
    default:
      Fatal("DVI file has non-allowed command %d between pages.",command);
    }
    ReadCommand(command);
#ifdef DEBUG
    if (Debug)
      printf("INIT CMD:\t%d\n", *command);
#endif
  }
  if ( *command == BOP ) {  /*  Init page */
    int i;
    if ((tpagelistp = malloc(sizeof(struct page_list)))==NULL)
      Fatal("cannot allocate memory for new page entry");
    tpagelistp->offset = (long) FTELL(dvi.filep)-45;
    for (i = 0; i <= 9; i++) {
      tpagelistp->count[i] = UNumRead(command + 1 + i*4, 4);
    }
    tpagelistp->count[10] = ++abspagenumber;
#ifdef DEBUG
    if (Debug)
      printf("INIT PAGE@%ld:\tcount0=%ld\t(absno=%ld)\n", tpagelistp->offset,
	     tpagelistp->count[0],abspagenumber);
#endif
    tpagelistp->next = NULL;
  }
  return(tpagelistp);
}

struct page_list* FindPage P1C(long4,pagenum)
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
  /* Whether or not the right page has been found, we want it */
  FSEEK(dvi.filep, tpagelistp->offset+45L, SEEK_SET);
  /* If page not yet found, skip in file until found */
  while(tpagelistp!=NULL && tpagelistp->count[index]!=pagenum) {
    SkipPage();
    tpagelistp->next=InitPage();
    tpagelistp = tpagelistp->next;
  }

#ifdef DEBUG
  if (Debug)
    if (tpagelistp!=NULL)
      printf("FIND PAGE@%ld:\tcount0=%ld\t(absno=%ld)\n", tpagelistp->offset,
	     tpagelistp->count[0],tpagelistp->count[10]);
#endif

  return(tpagelistp);
}


void DelPageList P1H(void)
     /* Delete the page list (only done on open/reopen) */
{
  struct page_list* temp;

  temp=hpagelistp;
  while(hpagelistp!=NULL) {
    hpagelistp=hpagelistp->next;
    free(temp);
    temp=hpagelistp;
  }
  abspagenumber=0;
}

