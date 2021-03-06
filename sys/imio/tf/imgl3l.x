# Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.

include	<syserr.h>
include	<imhdr.h>
include	<imio.h>

# IMGL3? -- Get a line from an apparently three dimensional image.  If there
# is only one input buffer, no image section, we are not referencing out of
# bounds, and no datatype conversion needs to be performed, directly access
# the pixels to reduce the overhead per line.

pointer procedure imgl3l (im, line, band)

pointer	im		# image header pointer
int	line		# line number within band
int	band		# band number

int	fd, nchars
long	vs[3], ve[3], offset
pointer	bp, imggsl(), freadp()
errchk	imopsf, imerr

begin
	repeat {
	    if (IM_FAST(im) == YES && IM_PIXTYPE(im) == TY_LONG) {
		fd = IM_PFD(im)
		if (fd == NULL) {
		    call imopsf (im)
		    next
		}
		if (line < 1 || line > IM_LEN(im,2) ||
		    band < 1 || band > IM_LEN(im,3))
		    call imerr (IM_NAME(im), SYS_IMREFOOB)

		offset = (((band - 1) * IM_PHYSLEN(im,2) + line - 1) *
		    IM_PHYSLEN(im,1)) * SZ_LONG + IM_PIXOFF(im)
		nchars = IM_PHYSLEN(im,1) * SZ_LONG
		ifnoerr (bp = (freadp (fd, offset, nchars) - 1) / SZ_LONG + 1)
		    return (bp)
	    }

	    vs[1] = 1
	    ve[1] = IM_LEN(im,1)
	    vs[2] = line
	    ve[2] = line
	    vs[3] = band
	    ve[3] = band

	    return (imggsl (im, vs, ve, 3))
	}
end
