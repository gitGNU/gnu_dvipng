#include "dvipng.h"

void SkipPage P1H(void)
{ 
  /* Skip present page */
  short   command;           

  command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
  if (Debug)
    printf("SKIP CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  while (command != EOP)  {
    if ((command > SETC_127 && command < FONT_00) 
	|| command > FONT_63) {
      switch (command)  {
      case PUT1: case SET1: case RIGHT1: case W1: case X1:
      case DOWN1: case Y1: case Z1: case FNT1:
	FSEEK(dvifp,1,SEEK_CUR);
	break;
      case PUT2: case SET2: case RIGHT2: case W2: case X2:
      case DOWN2: case Y2: case Z2: case FNT2:
	FSEEK(dvifp,2,SEEK_CUR);
	break;
      case PUT3: case SET3: case RIGHT3: case W3: case X3:
      case DOWN3: case Y3: case Z3: case FNT3:
	FSEEK(dvifp,3,SEEK_CUR);
	break;
      case PUT4: case SET4: case RIGHT4: case W4: case X4:
      case DOWN4: case Y4: case Z4: case FNT4:
	FSEEK(dvifp,4,SEEK_CUR);
	break;
      case SET_RULE: case PUT_RULE:
	FSEEK(dvifp,8,SEEK_CUR);
	break;
      case EOP: case PUSH: case POP: case W0: case X0:
      case Y0: case Z0: case NOP:
	break;
      case XXX1: case XXX2: case XXX3: case XXX4:
	FSEEK(dvifp, 
          (int)NoSignExtend(dvifp, (int)command - XXX1 + 1),SEEK_CUR);
	break;
      case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
	ReadFontDef((int)NoSignExtend(dvifp, (int)command - FNT_DEF1 + 1));
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
	Fatal("%d is an undefined command", command);
	break;
      }
    } /* if */
    command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
    if (Debug)
      printf("SKIP CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  } /* while */
}

struct page_list* InitPage P1H(void)
{
  /* Find page start, return pointer to page_list entry if found */
  short   command;       
  struct page_list* tpagelistp=NULL;

  command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
  if (Debug)
    printf("INIT CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif

  /* Skip until page start or postamble */
  while((command != BOP) && (command != POST)) {
    switch(command) {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      ReadFontDef((int)NoSignExtend(dvifp, (int)command - FNT_DEF1 + 1));
      break;
    case NOP: case BOP: case POST:
      break;
    default:
      Fatal("DVI file has non-allowed command %d between pages.",command);
    }
    command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
    if (Debug)
      printf("INIT CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  }
  if ( command == BOP ) {  /*  Init page */
    int i;
    if ((tpagelistp = malloc(sizeof(struct page_list)))==NULL)
      Fatal("cannot allocate memory for new page entry");
    tpagelistp->offset = (long) FTELL(dvifp) - 1;
    for (i = 0; i <= 9; i++) {
      tpagelistp->count[i] = NoSignExtend(dvifp, 4);
    }
    tpagelistp->count[10] = ++abspagenumber;
#ifdef DEBUG
    if (Debug)
      printf("INIT PAGE@%ld:\tcount0=%ld\t(absno=%ld)\n", tpagelistp->offset,
	     tpagelistp->count[0],abspagenumber);
#endif
    (void)NoSignExtend(dvifp, 4);
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
  FSEEK(dvifp, tpagelistp->offset+45L, SEEK_SET);
  /* If page not yet found, skip in file until found */
  while(tpagelistp!=NULL && tpagelistp->count[index]!=pagenum) {
    SkipPage();
    tpagelistp->next=InitPage();
    tpagelistp = tpagelistp->next;
  }

#ifdef DEBUG
  if (Debug)
    if (tpagelistp!=NULL)
      printf("FIND PAGE@%ld:\t%ld\t(%ld)\n", tpagelistp->offset,
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

