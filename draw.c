#include "dvipng.h"

struct stack_entry {  /* stack entry */
  long4    h, v, w, x, y, z;  /* what's on stack */
};

void DrawPage P1C(int, PassNo) 
{
  short   command;           /* current command                         */
  long4   val, val2;         /* temporarys to hold command information  */
  long4   w = 0;             /* current horizontal spacing              */
  long4   x = 0;             /* current horizontal spacing              */
  long4   y = 0;             /* current vertical spacing                */
  long4   z = 0;             /* current vertical spacing                */
  struct  stack_entry stack[STACK_SIZE];  /* stack                      */
  int     sp = 0;            /* stack pointer                           */
  int     k;                 /* temporary parameter                     */
  char    SpecialStr[STRSIZE]; /* "\special" strings                    */

  h = v = 0;
  fontptr = NULL;

  command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
  if (Debug)
    printf("DRAW CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  while (command != EOP)  {
    if (/*command >= SETC_000 &&*/ command <= SETC_127) {
      val=SetChar(command,PassNo);
      MoveOver(val);
    } else if (command >= FONT_00 && command <= FONT_63) {
      SetFntNum((long4)command - FONT_00);
    } else switch (command)  {
    case PUT1: case PUT2: case PUT3: case PUT4:
      val = NoSignExtend(dvifp, (int)command - PUT1 + 1);
      (void) SetChar(val,PassNo);
      break;
    case SET1: case SET2: case SET3: case SET4:
      val = NoSignExtend(dvifp, (int)command - SET1 + 1);
      val2 = SetChar(val,PassNo);
      MoveOver(val2);
      break;
    case SET_RULE:
      val = NoSignExtend(dvifp, 4);
      val2 = NoSignExtend(dvifp, 4);
      val = SetRule(val, val2, PassNo);
      MoveOver(val);
      break;
    case PUT_RULE:
      val = NoSignExtend(dvifp, 4);
      val2 = NoSignExtend(dvifp, 4);
      (void) SetRule(val, val2, PassNo);
      break;
    case BOP:
      Fatal("BOP occurs within page");
      break;
    case EOP:
      break;
    case PUSH:
      if (sp >= STACK_SIZE)
	Fatal("stack overflow");
      stack[sp].h = h;
      stack[sp].v = v;
      stack[sp].w = w;
      stack[sp].x = x;
      stack[sp].y = y;
      stack[sp].z = z;
      sp++;
      break;
    case POP:
      --sp;
      if (sp < 0)
	Fatal("stack underflow");
      h = stack[sp].h;
      v = stack[sp].v;
      w = stack[sp].w;
      x = stack[sp].x;
      y = stack[sp].y;
      z = stack[sp].z;
      break;
    case RIGHT1: case RIGHT2: case RIGHT3: case RIGHT4:
      val = SignExtend(dvifp, (int)command - RIGHT1 + 1);
      MoveOver(val);
      break;
    case W1: case W2: case W3: case W4:
      w = SignExtend(dvifp, (int)command - W1 + 1);
    case W0:
      MoveOver(w);
      break;
    case X1: case X2: case X3: case X4:
      x = SignExtend(dvifp, (int)command - X1 + 1);
    case X0:
      MoveOver(x);
      break;
    case DOWN1: case DOWN2: case DOWN3: case DOWN4:
      val = SignExtend(dvifp, (int)command - DOWN1 + 1);
      MoveDown(val);
      break;
    case Y1: case Y2: case Y3: case Y4:
      y = SignExtend(dvifp, (int)command - Y1 + 1);
    case Y0:
      MoveDown(y);
      break;
    case Z1: case Z2: case Z3: case Z4:
      z = SignExtend(dvifp, (int)command - Z1 + 1);
    case Z0:
      MoveDown(z);
      break;
    case FNT1: case FNT2: case FNT3: case FNT4:
      k = NoSignExtend(dvifp, (int) command - FNT1 + 1);
      SetFntNum(k);
      break;
    case XXX1: case XXX2: case XXX3: case XXX4:
      k = (int)NoSignExtend(dvifp, (int)command - XXX1 + 1);
      GetBytes(dvifp, SpecialStr, k);
      if ( PassNo == PASS_DRAW)
	DoSpecial(SpecialStr, k);
      break;
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      k = (int)NoSignExtend(dvifp, (int)command - FNT_DEF1 + 1);
      ReadFontDef(k);
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
    case NOP:
      break;
    default:
      Fatal("%d is an undefined command", command);
      break;
    }
    command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
    if (Debug)
      printf("DRAW CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  } /* while  */
  /* end of page */
}

void DoPages P1H(void)
{
  struct page_list *tpagelistp;

  while((tpagelistp=FindPage(TodoPage()))!=NULL) {
    if (PassDefault == PASS_BBOX) {
      DrawPage(PASS_BBOX);
      /* Reset to after BOP of current page */
      FSEEK(dvifp, tpagelistp->offset+45, SEEK_SET); 
      x_width = x_max-x_min;
      y_width = y_max-y_min;
      x_offset = -x_min; /* offset by moving topleft corner */
      y_offset = -y_min; 
      x_max = x_min = -x_offset_def; /* reset BBOX */
      y_max = y_min = -y_offset_def;
    }
    qfprintf(ERR_STREAM,"[%ld",  tpagelistp->count[0]);
    DoBop();
    DrawPage(PASS_DRAW);
    FormFeed(tpagelistp->count[0]);
    ++ndone;
    qfprintf(ERR_STREAM,"] ");
    if (tpagelistp->next==NULL) 
      tpagelistp->next=InitPage();
  }
  qfprintf(ERR_STREAM,"\n");
}
