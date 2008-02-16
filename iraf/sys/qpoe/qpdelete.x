# Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.

include	"qpoe.h"

# QP_DELETE -- Delete a poefile.

procedure qp_delete (poefile)

char	poefile[ARB]		#I poefile name
size_t	sz_val
pointer	sp, fname

begin
	call smark (sp)
	sz_val = SZ_PATHNAME
	call salloc (fname, sz_val, TY_CHAR)

	call qp_mkfname (poefile, QPOE_EXTN, Memc[fname], SZ_PATHNAME)
	call delete (Memc[fname])

	call sfree (sp)
end
