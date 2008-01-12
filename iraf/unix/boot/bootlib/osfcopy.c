/* Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.
 */

#include "bootlib.h"
#include <unistd.h>
#include <fcntl.h>

/* OS_FCOPY -- Copy a file.  Used by RTAR to resolve links.
 */
int os_fcopy ( const char *oldfile, const char *newfile )
{
	XCHAR	buf[SZ_FBUF+1];
	XINT	mode = 0;
	XINT	status, junk, x_maxch = SZ_FBUF;
	XINT	in, out;
	XINT	n;

	if (os_access (oldfile,0,0) == NO)
	    return (ERR);

	if (os_access (oldfile, 0, TEXT_FILE) == YES) {
	    if (bdebug)
		fprintf (stderr, "copy text file '%s' -> '%s'\n",
		    oldfile, newfile);

	    mode = READ_ONLY;
	    ZOPNTX ((PKCHAR *)vfn2osfn(oldfile,0), &mode, &in);
	    if (in == XERR)
		return (ERR);

	    mode = NEW_FILE;
	    ZOPNTX ((PKCHAR *)vfn2osfn(newfile,1), &mode, &out);
	    if (out == XERR) {
		ZCLSTX (&in, &status);
		return (ERR);
	    }

	    while (ZGETTX (&in, buf, &x_maxch, &n), n != XEOF) {
		if (n != XERR)
		    ZPUTTX (&out, buf, &n, &status);
		if (n == XERR || status == XERR) {
		    ZCLSTX (&in, &junk);
		    ZCLSTX (&out, &junk);
		    return (ERR);
		}
	    }

	    ZCLSTX (&in, &status);
	    ZCLSTX (&out, &status);

	    return (status);

	} else {
	    if (bdebug)
		fprintf (stderr, "copy binary file `%s' -> `%s'\n",
		    oldfile, newfile);

	    if ((in = open (vfn2osfn(oldfile,0), 0)) == ERR)
		return (ERR);
	    if ((out = creat (vfn2osfn(newfile,1), 0644)) == ERR) {
		close (in);
		return (ERR);
	    }

	    while ((n = read (in, (char *)buf, SZ_FBUF)) > 0)
		write (out, (char *)buf, n);

	    close (in);
	    close (out);
	    if (n < 0)
		return (ERR);
	}

	return (ERR);
}