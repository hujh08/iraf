# Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.

include	<imio.h>

# IMLOOP -- Increment the vector V from VS to VE (nested do loops cannot
# be used because of the variable number of dimensions).  Return LOOP_DONE
# when V exceeds VE.

int procedure imloop (v, vs, ve, vinc, ndim)

long	v[ndim], vs[ndim], ve[ndim], vinc[ndim]
int	ndim, dim

begin
	for (dim=2;  dim <= ndim;  dim=dim+1) {
	    v[dim] = v[dim] + vinc[dim]
	    if (abs(v[dim] - ve[dim]) == abs(vinc[dim])) {
		if (dim < ndim)
		    v[dim] = vs[dim]			# advance to next dim
		else
		    break
	    } else
		return (LOOP_AGAIN)
	}

	return (LOOP_DONE)
end