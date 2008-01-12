/* Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.
 */

#define import_spp
#define import_knames
#include <iraf.h>

/* ACHTU_ -- Unpack an unsigned short integer array into an SPP datatype.
 * [MACHDEP]: The underscore appended to the procedure name is OS dependent.
 */
int ACHTUD ( XUSHORT *a, XDOUBLE *b, XINT *npix )
{
	XUSHORT *ip;
	XDOUBLE *op, *maxop;

	if (sizeof(*op) >= sizeof(*ip)) {
	    for ( ip = &a[*npix], op = &b[*npix] ; a < ip ; )
		    *--op = *--ip;
	} else {
	    maxop = b + *npix -1;
	    for ( ip=a, op=b ; op <= maxop ; )
		    *op++ = *ip++;
	}

	return 0;
}