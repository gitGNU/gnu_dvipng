#include "dvipng.h"

/* Some code at the end of this file is adapted from dvips */

static int32_t first=PAGE_FIRSTPAGE, last=PAGE_LASTPAGE;
static bool    abspage=_FALSE, reverse=_FALSE;
bool           no_ppage=_TRUE;

/* dvips' behaviour:
 * -pp outputs _all_ pages with the correct numbers,
 * -p, -l outputs from the first occurrence of firstpage to the first
 * occurrence of lastpage. Using '=' means absolute pagenumbers
 */

void FirstPage(int32_t page, bool data)
{
  first=page;
  abspage |= data;
}
void LastPage(int32_t page,bool data)
{
  last=page;
  abspage |= data;
}
bool Reverse(void)
{
  return(reverse = !reverse);
}
/*-->NextPPage*/
/**********************************************************************/
/****************************  NextPPage  *****************************/
/**********************************************************************/
/* Return the page in turn on our queue */
/* (Implicitly, PAGE_POST is never in the pagelist) */
bool InPageList(int32_t i);

struct page_list* NextPPage(void* dvi, struct page_list* page)
{
  if (! reverse) { /*********** normal order */
    if (page == NULL) { /* first call: find first page */ 
      if (no_ppage)
	return(NULL);
      page=FindPage(dvi,first,abspage);
    } else              /* later calls: count up, except "last" page */ 
      page=(last==page->count[abspage ? 0 : 10]) ? NULL : NextPage(dvi,page);
    /* seek for pages in pagelist */ 
    while (page!=NULL && ! InPageList(page->count[0]))
      page=(last==page->count[abspage ? 0 : 10]) ? NULL : NextPage(dvi,page);
  } else { /******************** reverse order */
    if (page == NULL) { /* first call: find "last" page */ 
      if (no_ppage)
	return(NULL);
      page=FindPage(dvi,last,abspage);
    } else              /* later calls: count down, except "first" page */
      page=(first==page->count[abspage ? 0 : 10]) ? NULL : PrevPage(dvi,page);
    /* seek for pages in pagelist */ 
    while (page!=NULL && ! InPageList(page->count[0])) 
      page=(first==page->count[abspage ? 0 : 10]) ? NULL : PrevPage(dvi,page);
  }
  return(page);
}

/*
 * Handle a list of pages for dvipng.  Code based on dvips 5.74, in
 * turn based on dvi2ps 2.49.
 */

struct pp_list {
    struct pp_list *next;	/* next in a series of alternates */
    int32_t ps_low, ps_high;	/* allowed range */
} *ppages = 0;	/* the list of allowed pages */

/*-->InPageList*/
/**********************************************************************/
/******************************  InPageList  **************************/
/**********************************************************************/
/* Return true iff i is one of the desired output pages */

bool InPageList(int32_t i)
{
  register struct pp_list *pl = ppages;

  while (pl) {
    if ( i >= pl -> ps_low && i <= pl -> ps_high)
      return(_TRUE);		/* success */
    pl = pl -> next;
  }
  return(_FALSE);
}

void ListPage(int32_t pslow, int32_t pshigh)
{
  register struct pp_list   *pl;

  /* Some added code, we want to reuse the list */
  no_ppage=_FALSE;
  pl = ppages;
  while (pl != NULL && pl->ps_low <= pl->ps_high)
    pl = pl->next;
  if (pl == NULL) {
    if ((pl = (struct pp_list *)malloc(sizeof(struct pp_list)))
	==NULL) 
      Fatal("cannot allocate memory for page queue");
    pl -> next = ppages;
    ppages = pl;
  }
  pl -> ps_low = pslow;
  pl -> ps_high = pshigh;
}

/* Parse a string representing a list of pages.  Return 0 iff ok.  As a
   side effect, the page selection(s) is (are) prepended to ppages. */

bool ParsePages(register char  *s)
{
  register int    c ;		/* current character */
  register int    n = 0,	/* current numeric value */
    innumber;	/* true => gathering a number */
  int32_t ps_low = 0, ps_high = 0 ;
  int     range,		/* true => saw a range indicator */
    negative = 0;	/* true => number being built is negative */
  
#define white(x) ((x) == ' ' || (x) == '\t' || (x) == ',')
  
  range = 0;
  innumber = 0;
  for (;;) {
    c = *s++;
    if ( !innumber && !range) {/* nothing special going on */
      if (c == 0)
	return(_FALSE);
      if (white (c))
	continue;
    }
    if (c == '-' && !innumber) {
      innumber++;
      negative++;
      n = 0;
      continue;
    }
    if ('0' <= c && c <= '9') {	/* accumulate numeric value */
      if (!innumber) {
	innumber++;
	negative = 0;
	n = c - '0';
	continue;
      }
      n *= 10;
      n += negative ? '0' - c : c - '0';
      continue;
    }
    if (c == '-' || c == ':') {/* here's a range */
      if (range)
	return(_TRUE);
      if (innumber) {	/* have a lower bound */
	ps_low = n;
      }
      else
	ps_low = PAGE_MINPAGE;
      range++;
      innumber = 0;
      continue;
    }
    if (c == 0 || white (c)) {/* end of this range */
      if (!innumber) {	/* no upper bound */
	ps_high = PAGE_MAXPAGE;
	if (!range)	/* no lower bound either */
	  ps_low = PAGE_MINPAGE;
      } else {		/* have an upper bound */
	ps_high = n;
	if (!range) {	/* no range => lower bound == upper */
	  ps_low = ps_high;
	}
      }
      ListPage(ps_low, ps_high);
      if (c == 0)
	return(_FALSE);
      range = 0;
      innumber = 0;
      continue;
    }
    return(_TRUE);
  }
#undef white
}

/* Addition, we want to be able to clear the pplist */
void ClearPpList(void)
{
  register struct pp_list *pl = ppages;

  while (pl) {
    pl -> ps_low = 0;
    pl -> ps_high = -1;
    pl = pl -> next;
  }
  first=PAGE_FIRSTPAGE;
  last=PAGE_LASTPAGE;
  abspage = _FALSE;
  no_ppage=_TRUE;
}

