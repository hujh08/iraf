# Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.

include <mii.h>


# MII.X -- This is a stand-alone port of the miiread/miiwrite routines from
# etc/osb, modified for IMFORT to use bfio.
#
#	status = imiirdi (fp, spp, maxelem)
#	status = imiirdl (fp, spp, maxelem)
#	status = imiirdr (fp, spp, maxelem)
#
#	status = imiiwri (fp, spp, nelem)
#	status = imiiwrl (fp, spp, nelem)
#	status = imiiwrr (fp, spp, nelem)
#
#	status = imiirdc (fp, spp, maxchars)
#	status = imiiwrc (fp, spp, nchars)


# IMIIRDI -- Read a block of data stored externally in MII integer format.
# Data is returned in the format of the local host machine.

int procedure imiirdi (fp, spp, maxelem)

pointer	fp			#I input file
int	spp[ARB]		#O receives data
int	maxelem			#I max number of data elements to be read

size_t	sz_val
pointer	sp, bp
int	pksize, nchars, nelem
int	miipksize(), miinelem(), bfrseq()

begin
	pksize = miipksize (maxelem, MII_INT)
	nelem  = EOF

	if (pksize > maxelem * SZ_INT) {
	    # Read data into local buffer and unpack into user buffer.

	    call smark (sp)
	    sz_val = pksize
	    call salloc (bp, sz_val, TY_CHAR)

	    nchars = bfrseq (fp, Memc[bp], pksize)
	    if (nchars != EOF) {
		nelem = min (maxelem, miinelem (nchars, MII_INT))
		call miiupki (Memc[bp], spp, nelem, TY_INT)
	    }

	    call sfree (sp)
	
	} else {
	    # Read data into user buffer and unpack in place.

	    nchars = bfrseq (fp, spp, pksize)
	    if (nchars != EOF) {
		nelem = min (maxelem, miinelem (nchars, MII_INT))
		call miiupki (spp, spp, nelem, TY_INT)
	    }
	}

	return (nelem)
end


# IMIIRDL -- Read a block of data stored externally in MII long integer format.
# Data is returned in the format of the local host machine.

int procedure imiirdl (fp, spp, maxelem)

pointer	fp			#I input file
long	spp[ARB]		#O receives data
int	maxelem			#I max number of data elements to be read

size_t	sz_val
pointer	sp, bp
int	pksize, nchars, nelem
int	miipksize(), miinelem(), bfrseq()

begin
	pksize = miipksize (maxelem, MII_LONG)
	nelem  = EOF

	if (pksize > maxelem * SZ_LONG) {
	    # Read data into local buffer and unpack into user buffer.

	    call smark (sp)
	    sz_val = pksize
	    call salloc (bp, sz_val, TY_CHAR)

	    nchars = bfrseq (fp, Memc[bp], pksize)
	    if (nchars != EOF) {
		nelem = min (maxelem, miinelem (nchars, MII_LONG))
		call miiupkl (Memc[bp], spp, nelem, TY_LONG)
	    }

	    call sfree (sp)
	
	} else {
	    # Read data into user buffer and unpack in place.

	    nchars = bfrseq (fp, spp, pksize)
	    if (nchars != EOF) {
		nelem = min (maxelem, miinelem (nchars, MII_LONG))
		call miiupkl (spp, spp, nelem, TY_LONG)
	    }
	}

	return (nelem)
end


# IMIIRDR -- Read a block of data stored externally in MII real format.
# Data is returned in the format of the local host machine.

int procedure imiirdr (fp, spp, maxelem)

pointer	fp			#I input file
real	spp[ARB]		#O receives data
int	maxelem			# max number of data elements to be read

size_t	sz_val
pointer	sp, bp
int	pksize, nchars, nelem
int	miipksize(), miinelem(), bfrseq()

begin
	pksize = miipksize (maxelem, MII_REAL)
	nelem  = EOF

	if (pksize > maxelem * SZ_REAL) {
	    # Read data into local buffer and unpack into user buffer.

	    call smark (sp)
	    sz_val = pksize
	    call salloc (bp, sz_val, TY_CHAR)

	    nchars = bfrseq (fp, Memc[bp], pksize)
	    if (nchars != EOF) {
		nelem = min (maxelem, miinelem (nchars, MII_REAL))
		call miiupkr (Memc[bp], spp, nelem, TY_REAL)
	    }

	    call sfree (sp)
	
	} else {
	    # Read data into user buffer and unpack in place.

	    nchars = bfrseq (fp, spp, pksize)
	    if (nchars != EOF) {
		nelem = min (maxelem, miinelem (nchars, MII_REAL))
		call miiupkr (spp, spp, nelem, TY_REAL)
	    }
	}

	return (nelem)
end


# IMIIWRI -- Write a block of data to a file in MII integer format.
# The input data is in the host system native binary format.

int procedure imiiwri (fp, spp, nelem)

pointer	fp			#I output file
int	spp[ARB]		#I native format data to be written
int	nelem			#I number of data elements to be written

size_t	sz_val
pointer	sp, bp
int	bufsize, status
int	miipksize(), bfwseq()

begin
	status = OK
	call smark (sp)

	bufsize = miipksize (nelem, MII_INT)
	sz_val = bufsize
	call salloc (bp, sz_val, TY_CHAR)

	call miipaki (spp, Memc[bp], nelem, TY_INT)
	if (bfwseq (fp, Memc[bp], bufsize) == ERR)
	    status = ERR

	call sfree (sp)
	return (status)
end


# IMIIWRL -- Write a block of data to a file in MII long integer format.
# The input data is in the host system native binary format.

int procedure imiiwrl (fp, spp, nelem)

pointer	fp			#I output file
long	spp[ARB]		#I native format data to be written
int	nelem			#I number of data elements to be written

size_t	sz_val
pointer	sp, bp
int	bufsize, status
int	miipksize(), bfwseq()

begin
	status = OK
	call smark (sp)

	bufsize = miipksize (nelem, MII_LONG)
	sz_val = bufsize
	call salloc (bp, sz_val, TY_CHAR)

	call miipakl (spp, Memc[bp], nelem, TY_LONG)
	if (bfwseq (fp, Memc[bp], bufsize) == ERR)
	    status = ERR

	call sfree (sp)
	return (status)
end


# IMIIWRR -- Write a block of data to a file in MII real format.
# The input data is in the host system native binary format.

int procedure imiiwrr (fp, spp, nelem)

pointer	fp			#I output file
real	spp[ARB]		#I native format data to be written
int	nelem			#I number of data elements to be written

size_t	sz_val
pointer	sp, bp
int	bufsize, status
int	miipksize(), bfwseq()

begin
	status = OK
	call smark (sp)

	bufsize = miipksize (nelem, MII_REAL)
	sz_val = bufsize
	call salloc (bp, sz_val, TY_CHAR)

	call miipakr (spp, Memc[bp], nelem, TY_REAL)
	if (bfwseq (fp, Memc[bp], bufsize) == ERR)
	    status = ERR

	call sfree (sp)
	return (status)
end


# IMIIRDC -- Read a block of character data stored externally in MII format.
# Data is returned in the machine independent character format.

int procedure imiirdc (fp, spp, maxchars)

pointer	fp			#I input file
char	spp[ARB]		#O receives data
int	maxchars		#I max number of chars to be read

size_t	sz_val
pointer	sp, bp
int	pksize, nchars
int	miipksize(), miinelem(), bfrseq()

begin
	pksize = miipksize (maxchars, MII_BYTE)
	nchars = max (maxchars, pksize)

	if (nchars > maxchars) {
	    # Read data into local buffer and unpack into user buffer.

	    call smark (sp)
	    sz_val = nchars
	    call salloc (bp, sz_val, TY_CHAR)

	    nchars = bfrseq (fp, Memc[bp], pksize)
	    if (nchars != EOF) {
		nchars = min (maxchars, miinelem (nchars, MII_BYTE))
		call miiupk8 (Memc[bp], spp, nchars, TY_CHAR)
	    }

	    call sfree (sp)
	
	} else {
	    # Read data into user buffer and unpack in place.

	    nchars = bfrseq (fp, spp, pksize)
	    if (nchars != EOF) {
		nchars = min (maxchars, miinelem (nchars, MII_BYTE))
		call miiupk8 (spp, spp, nchars, TY_CHAR)
	    }
	}

	return (nchars)
end


# IMIIWRC -- Write a block of character data to a file in MII format.
# The input data is assumed to be in a machine independent format. 

int procedure imiiwrc (fp, spp, nchars)

pointer	fp			#I output file
char	spp[ARB]		#I data to be written
int	nchars			#I number of chars units to be written

size_t	sz_val
pointer	sp, bp
int	bufsize, status
int	miipksize(), bfwseq()

begin
	status = OK
	call smark (sp)

	bufsize = miipksize (nchars, MII_BYTE)
	sz_val = bufsize
	call salloc (bp, sz_val, TY_CHAR)

	call miipak8 (spp, Memc[bp], nchars, TY_CHAR)
	if (bfwseq (fp, Memc[bp], bufsize) == ERR)
	    status = ERR

	call sfree (sp)
	return (status)
end
