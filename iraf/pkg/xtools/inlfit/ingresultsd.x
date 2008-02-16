include <pkg/inlfit.h>

# ING_RESULTS -- Print the results of the fit.

procedure ing_resultsd (in, file, nl, x, y, wts, names, npts, nvars, len_name)

pointer	in			# INLFIT pointer
char	file[ARB]		# Output file name
pointer	nl			# NLFIT pointer
double	x[ARB]			# Ordinates (npts * nvars)
double	y[ARB]			# Abscissas
double	wts[ARB]		# Weights
char	names[ARB]		# Object names
int	npts		        # Number of data points
int	nvars		        # Number of variables
int	len_name		# Length of a name

size_t	sz_val
int	i, fd, rejected
pointer	sp, fit, wts1, rejpts
int	open(), in_geti()
pointer	in_getp()
errchk	open

begin
	# Open the output file.
	if (file[1] == EOS)
	    return
	fd = open (file, APPEND, TEXT_FILE)

	# Test the number of points.
	if (npts == 0) {
	    call eprintf ("Incomplete output - no data points for fit\n")
	    return
	}

	# Allocate memory.
	call smark (sp)
	sz_val = npts
	call salloc (fit, sz_val, TY_DOUBLE)
	call salloc (wts1, sz_val, TY_DOUBLE)

	# Evaluate the fit.
	call nlvectord (nl, x, Memd[fit], npts, nvars)

	# Assign a zero weight to the rejected points.
	rejected = in_geti (in, INLNREJPTS)
	rejpts = in_getp (in, INLREJPTS)
	call amovd (wts, Memd[wts1], npts)
	if (rejected > 0) {
	    do i = 1, npts {
		if (Memi[rejpts+i-1] == YES)
		    Memd[wts1+i-1] = double (0.0)
	    }
	}

	# Print the title.
	call fprintf (fd, "\n#%14.14s %14.14s %14.14s")
	    call pargstr ("objectid")
	    call pargstr ("function")
	    call pargstr ("fit")
	call fprintf (fd, " %14.14s %14.14s\n")
	    call pargstr ("residuals")
	    call pargstr ("sigma")
	
	# List function value, fit value, residual and error values.
	do i = 1, npts {
	    call fprintf (fd, " %14.14s %14.7g %14.7g %14.7g %14.7g\n")
	    call pargstr (names[(i-1)*len_name+1])
	    if (Memd[wts1+i-1] <= 0.0) {
		call pargd (INDEFD)
		call pargd (INDEFD)
		call pargd (INDEFD)
		call pargd (INDEFD)
	    } else {
		call pargd (y[i])
		call pargd (Memd[fit+i-1])
		call pargd (y[i] - Memd[fit+i-1])
		call pargd (sqrt (double (1.0) / Memd[wts1+i-1]))
	    }
	}
	call fprintf (fd, "\n")

	# Free allocated memory, and close output file.
	call sfree (sp)
	call close (fd)
end
