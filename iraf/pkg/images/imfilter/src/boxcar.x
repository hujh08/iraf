# Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.

include <error.h>
include <imhdr.h>
include <imset.h>

# CNV_BOXCAR -- Convolve an image. The kernel dimensions are assumed to
# be odd

procedure cnv_boxcar (im1, im2, nxk, nyk, boundary, constant)

pointer	im1		# pointer to the input image
pointer	im2		# pointer to the output image
int	nxk, nyk	# dimensions of the kernel
int	boundary	# type of boundary extnsion
real	constant	# constant for constant boundary extension

size_t	sz_val
int	i, ncols, nlines, col1, col2, inline, outline
pointer	sp, lineptrs, accum, outbuf

pointer	imgs2r(), impl2r()

errchk	imgs2r, impl2r

begin
	# Number of columns and lines of output image
	ncols = IM_LEN(im2,1)
	if (IM_NDIM(im2) == 1)
	    nlines = 1
	else
	    nlines = IM_LEN(im2,2)

	# Set input image column limits
	col1 = 1 - nxk / 2
	col2 = IM_LEN(im1,1) + nxk / 2

	# Set up an array of linepointers and accumulators
	call smark (sp)
	sz_val = nyk
	call salloc (lineptrs, sz_val, TY_POINTER)
	sz_val = ncols + nxk - 1
	call salloc (accum, sz_val, TY_REAL)

	# Set boundary conditions on input image
	call imseti (im1, IM_NBUFS, nyk)
	call imseti (im1, IM_TYBNDRY, boundary)
	call imseti (im1, IM_NBNDRYPIX, max (nxk / 2 + 1, nyk / 2 + 1))
	if (boundary == BT_CONSTANT)
	    call imsetr (im1, IM_BNDRYPIXVAL, constant)

	# Clear the accumulator
	call aclrr (Memr[accum], ncols + nxk - 1)

	# Initialize the accumulator
	inline = 1 - nyk / 2
	do i = 1, nyk - 1 {
	    Memp[lineptrs+i] = imgs2r (im1, col1, col2, inline, inline)
	    call aaddr (Memr[accum], Memr[Memp[lineptrs+i]], Memr[accum],
	        ncols + nxk - 1)
	    inline = inline + 1
	}

	# Generate the remaining image lines image line by line
	do outline = 1, nlines {

	    # Scroll buffers
	    do i = 1, nyk - 1
		Memp[lineptrs+i-1] = Memp[lineptrs+i]

	    # Read in new image line, accumulate
	    Memp[lineptrs+nyk-1] = imgs2r (im1, col1, col2, inline, inline)
	    call aaddr (Memr[accum], Memr[Memp[lineptrs+nyk-1]], Memr[accum],
		ncols + nxk - 1)

	    # Write output image line
	    outbuf = impl2r (im2, outline)
	    if (outbuf == EOF)
		call error (0, "Error writing output image.")
	    call cnv_aboxr (Memr[accum], Memr[outbuf], ncols, nxk)
	    call adivkr (Memr[outbuf], real (nxk * nyk), Memr[outbuf], ncols)

	    # Subtract last line
	    call asubr (Memr[accum], Memr[Memp[lineptrs]], Memr[accum],
	        ncols + nxk - 1)

	    inline = inline + 1
	}

	# Free buffers
	call sfree (sp)
end
