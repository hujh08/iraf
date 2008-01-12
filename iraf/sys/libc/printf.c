/* Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.
 */

#define import_spp
#define import_libc
#define import_xnames
#define import_stdio
#define import_ctype
#define import_stdarg
#include <iraf.h>

/* PRINTF -- Emulation of the UNIX printf facilities with the IRAF FMTIO
 * interface as the backend.  All features of the UNIX printf are supported
 * without modification.  Additional format codes are supported in conformance
 * with the IRAF printf, e.g., hms format, variable radix, tabstops, etc.,
 * but these are upward compatible with standard UNIX usage.
 */

#define SZ_FMTSPEC	25		/* max size single format spec	*/
#define SZ_OBUF		SZ_COMMAND	/* sz intermediate buffer	*/
#define MAX_PREC	4		/* max "*" deferred args	*/
#define NOARG		(-1)		/* % spec with no data value	*/


/* PRINTF -- Formatted print to the standard output.
 */
#ifdef USE_STDARG
int printf ( const char *format, ... )
#else
int printf (va_alist)
va_dcl
#endif
{
	int ret = 0;
        va_list argp;

#ifdef USE_STDARG
	va_start (argp, format);
#else
	const char *format;
	va_start (argp);
	format = va_arg (argp, const char *);
#endif
	ret = u_doprnt (format, &argp, stdout);
	va_end (argp);

	return ret;
}


/* FPRINTF -- Formatted print to a file.
 */
#ifdef USE_STDARG
int fprintf ( FILE *fp, const char *format, ... )
#else
int fprintf ( va_alist )
va_dcl
#endif
{
	int ret = 0;
        va_list argp;

#ifdef USE_STDARG
	va_start (argp, format);
#else
	FILE *fp;
	const char *format;
	va_start (argp);
	fp = va_arg (argp, FILE *);
	format = va_arg (argp, const char *);
#endif
	ret = u_doprnt (format, &argp, fp);
	va_end (argp);

	return ret;
}

int u_doarg ( FILE *, XCHAR *, va_list **, int [], int , int );

/* U_DOPRNT -- Process the format to the output file, taking arguments from
 * the list pointed to by argp as % format specs are encountered in the input.
 * The main point of this routine is to handle the variable number of arguments.
 * The actual encoding is all handled by the IRAF FPRINF and PARG calls.
 * N.B. we assume chars are stacked as ints, and floats are stacked as doubles.
 */
/* format : "%w.dC" etc. format spec   */
/* argp   : pointer to first value arg */
/* fp     : output file                */
int u_doprnt ( const char *format, va_list *argp, FILE *fp )
{
	int ch;				/* next format char reference	*/
	XCHAR formspec[SZ_FMTSPEC];	/* copy of single format spec	*/
	XCHAR *fsp, *maxfsp;		/* pointer into formspec	*/
	int done, dotseen;		/* one when at end of a format	*/
	int varprec, junk;		/* runtime precision is used	*/
	int prec[MAX_PREC];		/* values of prec args		*/

	while (ch = *format++) {
	    if (ch == '%') {
		fsp = formspec;
		maxfsp = formspec + SZ_FMTSPEC -1;
		*fsp++ = ch;
		varprec = 0;
		dotseen = 0;
		done = 0;

		while (!done) {
		    ch = *format++;
		    if ( fsp < maxfsp ) *fsp++ = ch;

		    switch (ch) {
		    case EOS:
			--format;
			done++;
			break;

		    case 'l':
			/* arg size modifier; ignored for now */
			fsp--;
			break;

		    case '*':
			if ( varprec < MAX_PREC )
			    prec[varprec++] = va_arg ((*argp), int);
			else
			    junk = va_arg ((*argp), int);
			break;

		    case '.':
			dotseen++;
			break;

		    case 'r':			/* nonstandard UNIX	*/
			ch = *format++;
			if ( fsp < maxfsp ) *fsp++ = ch;
			if ( ch  == '*' ) {
			    int		radix;
			    int		radchar;

			    radix = va_arg ((*argp), int);
			    if (radix < 0)
				radchar = 'A';
			    else if (radix > 9)
				radchar = radix - 10 + 'A';
			    else
				radchar = todigit (radix);
			    *(fsp-1) = radchar;
			} else if (ch == EOS) {
			    --format;
			    break;
			}
			/* fall through */

		    case 'b':			/* nonstandard UNIX	*/
		    case 'c':
		    case 'd':
		    case 'o':
		    case 'x':
		    case 'u':
			*fsp = XEOS;
			u_doarg (fp, formspec, &argp, prec, varprec, TY_INT);
			done++;
			break;

		    case 'E':			/* ANSI emulation	*/
			*(fsp-1) = 'e';
			goto rval;
		    case 'G':			/* ANSI emulation	*/
			*(fsp-1) = 'g';
			goto rval;

		    case 'z':			/* nonstandard UNIX	*/
		    case 'h':			/* nonstandard UNIX	*/
		    case 'H':			/* nonstandard UNIX	*/
		    case 'm':			/* nonstandard UNIX	*/
		    case 'M':			/* nonstandard UNIX	*/
		    case 'e':
		    case 'f':
		    case 'g':
			/* If no precision was specified, default to 14 digits
			 * for %[efgz] and 3 digits for %[hm].
			 */
rval:			if (!dotseen) {
			    *(fsp-1) = '.';
			    if (ch == 'h' || ch == 'm' ||
				ch == 'H' || ch == 'M') {
				if ( fsp < maxfsp ) *fsp++ = '3';
			    } else {
				if ( fsp < maxfsp ) *fsp++ = '1';
				if ( fsp < maxfsp ) *fsp++ = '4';
			    }
			    if ( fsp < maxfsp ) *fsp++ = ch;
			}

			*fsp = XEOS;
			u_doarg (fp, formspec, &argp, prec, varprec, TY_DOUBLE);
			done++;
			break;

		    case 's':
			*fsp = XEOS;
			u_doarg (fp, formspec, &argp, prec, varprec, TY_CHAR);
			done++;
			break;

		    case 't':			/* nonstandard UNIX	*/
		    case 'w':			/* nonstandard UNIX	*/
			*fsp = XEOS;
			u_doarg (fp, formspec, &argp, prec, varprec, NOARG);
			done++;
			break;

		    case '%':
			putc (ch, fp);
			done++;
			break;
		    }
		}

	    } else 
		putc (ch, fp);
	}

	return 0;
}


/* U_DOARG -- Encode a single argument acording to the simplified format
 * specification given by formspec.  This is the interface to the IRAF
 * formatted output procedures.
 */
/* fp       : output file            */
/* formspec : format string          */
/* argp     : pointer to data value  */
/* prec     : varprec args, if any   */
/* varprec  : number of varprec args */
/* dtype    : datatype of data value */
int u_doarg ( FILE *fp, XCHAR *formspec, va_list **argp, 
	       int prec[], int varprec, int dtype )
{
	int ret = 0;
	int p;
	XCHAR sbuf[SZ_OBUF+1];
	XINT fd = fileno (fp);
	XINT ival;
	XDOUBLE dval;
	const char *cptr;

	/* Pass format string and any variable precision arguments.
	 */
	FPRINTF (&fd, formspec);
	for (p=0;  p < varprec;  p++) {
	    ival = prec[p];
	    PARGI (&ival);
	}

	/* Pass the data value to be encoded, bump argument pointer by the
	 * size of the data object.  If there is no data value the case
	 * is a no-op.
	 */
	switch (dtype) {
	case TY_INT:
	    ival = va_arg ((**argp), int);
	    PARGI (&ival);
	    break;
	case TY_DOUBLE:
	    dval = va_arg ((**argp), double);
	    PARGD (&dval);
	    break;
	case TY_CHAR:
	    cptr = va_arg ((**argp), const char *);
	    PARGSTR (c_strupk (cptr, sbuf, SZ_OBUF));
	    break;
	}

	return ret;
}