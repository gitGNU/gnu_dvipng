#include "dvipng.h"

/*
 * Handle a list of pages for dvipng.  Code based on dvips 5.78 which
 * in turn is based on dvi2ps 2.49, maintained by Piet van Oostrum,
 * piet@cs.ruu.nl.  Collected and modularized for inclusion in dvips
 * by metcalf@lcs.mit.edu. Included in dvipng by jalar@imf.au.dk.
 */

#define MAXPAGE (1000000000) /* assume no pages out of this range */
struct p_list_str {
    struct p_list_str *next;    /* next in a series of alternates */
    int ps_low, ps_high;    /* allowed range */
} *ppages = 0;  /* the list of allowed pages */

/*-->InPageList*/
/**********************************************************************/
/******************************  InPageList  **************************/
/**********************************************************************/
/* Return true iff i is one of the desired output pages */

int InPageList P1C(int, i)
{
    register struct p_list_str *pl = ppages;

    while (pl) {
      if ( i >= pl -> ps_low && i <= pl -> ps_high)
	return 1;		/* success */
      pl = pl -> next;
    }
    return 0;
}

void InstallPL P2C(int, pslow, int, pshigh)
{
    register struct p_list_str   *pl;

    if ((pl = (struct p_list_str *)malloc((int)(sizeof *pl)))==NULL)
      Fatal("cannot allocate memory for page structure");
    pl -> next = ppages;
    pl -> ps_low = pslow;
    pl -> ps_high = pshigh;
    ppages = pl;
}

/* Parse a string representing a list of pages.  Return 0 iff ok.  As a
   side effect, the page selection(s) is (are) prepended to ppages. */

int
ParsePages P1C(register char  *, s)
{
    register int    c ;		/* current character */
    register int  n = 0,	/* current numeric value */
		    innumber;	/* true => gathering a number */
    int ps_low = 0, ps_high = 0 ;
    int     range,		/* true => saw a range indicator */
	    negative = 0;	/* true => number being built is negative */

#define white(x) ((x) == ' ' || (x) == '\t' || (x) == ',')

    range = 0;
    innumber = 0;
    for (;;) {
	c = *s++;
	if ( !innumber && !range) {/* nothing special going on */
	    if (c == 0)
		return 0;
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
		return (-1);
	    if (innumber) {	/* have a lower bound */
		ps_low = n;
	    }
	    else
		ps_low = -MAXPAGE;
	    range++;
	    innumber = 0;
	    continue;
	}
	if (c == 0 || white (c)) {/* end of this range */
	    if (!innumber) {	/* no upper bound */
		ps_high = MAXPAGE;
		if (!range)	/* no lower bound either */
		    ps_low = -MAXPAGE;
	    }
	    else {		/* have an upper bound */
		ps_high = n;
		if (!range) {	/* no range => lower bound == upper */
		    ps_low = ps_high;
		}
	    }
	    InstallPL (ps_low, ps_high);
	    if (c == 0)
		return 0;
	    range = 0;
	    innumber = 0;
	    continue;
	}
	return (-1);
    }
#undef white
}
