# Global IRAF base environment list.

set	pkglibs		= "hlib$libc/"

# The following should be commented out or reset to "no" to enable world
# coordinate support (MWCS) in V2.9.  Note however that the IRAF packages
# have not yet been modified to use MWCS, and enabling MWCS may cause these
# packages to misbehave.

#set	nomwcs		= yes

# The following can be used to en/disable the VO modifications in the system.
# The 'use_vo' variable can be used to globally disable features, finer 
# grained controls might also be added.

set	use_vo		= yes
set	use_new_imt	= yes
set	vo_prefetch	= yes
set	vo_nthreads	= 4
set	vo_runid	= "iraf2160"

set	samp_auto	= yes
set	samp_onstart	= no


# Default cache directory.
set	cache		= "tmp$cache/"
set	cache_age	= 30


# Local system defaults.

set	clobber		= no
set	imclobber	= no
set	cmbuflen	= 128000
set	editor		= vi
set	filewait	= yes
set	glbcolor	= "pt=3,fr=9,al=3,tl=6,ax=5,tk=5"
set	graphcap	= dev$graphcap
set	imtype		= "fits"
set	imextn		= "oif:imh fxf:fits,fit plf:pl qpf:qp stf:hhh,??h"
set	min_lenuserarea	= 64000
set	multversions	= no
set	printer		= lp
set	pspage		= "letter"
set	stdgraph	= xgterm
set	stdimage	= imt512
set	stdimcur	= stdimage
set	stdplot		= lp
set	stdvdm		= uparm$vdm
set	tapecap		= dev$tapecap
set	termcap		= dev$termcap
set	terminal	= xgterm
set	ttybaud		= 9600
set	ttyncols	= 80
set	ttynlines	= 40
set	version		= "NOAO/IRAF V2.16"

# System directories.

set	as		= "host$as/"
set	bin		= "iraf$bin(arch)/"
set	boot		= "host$boot/"
set	dev		= "iraf$dev/"
set	doc		= "iraf$doc/"
set	hlib		= "host$hlib/"
set	lib		= "iraf$lib/"
set	math		= "iraf$math/"
set	os		= "host$os/"
set	osb		= "sys$osb/"
set	pkg		= "iraf$pkg/"
set	sys		= "iraf$sys/"

set	clio		= "sys$clio/"
set	dbio		= "sys$dbio/"
set	debug		= "sys$debug/"
set	etc		= "sys$etc/"
set	fio		= "sys$fio/"
set	flib		= "sys$flib/"
set	fmio		= "sys$fmio/"
set	fmtio		= "sys$fmtio/"
set	gio		= "sys$gio/"
set	gty		= "sys$gty/"
set	imfort		= "sys$imfort/"
set	imio		= "sys$imio/"
set	ki		= "sys$ki/"
set	libc		= "sys$libc/"
set	memio		= "sys$memio/"
set	mtio		= "sys$mtio/"
set	mwcs		= "sys$mwcs/"
set	plio		= "sys$plio/"
set	pmio		= "sys$pmio/"
set	psio		= "sys$psio/"
set	qpoe		= "sys$qpoe/"
set	tty		= "sys$tty/"
set	vops		= "sys$vops/"

# System package directories.

set	cl		= "pkg$cl/"
set	clpackage	= "hlib$"
set	dataio		= "pkg$dataio/"
set	dbms		= "pkg$dbms/"
set	images		= "pkg$images/"
set	language	= "pkg$language/"
set	lists		= "pkg$lists/"
set	obsolete	= "pkg$obsolete/"
set	plot		= "pkg$plot/"
set	proto		= "pkg$proto/"
set	softools	= "pkg$softools/"
set	system		= "pkg$system/"
set	utilities	= "pkg$utilities/"
set	xtools		= "pkg$xtools/"


# Load definitions for any locally added external packages.

set	@hlib$extern.pkg
set	@iraf$extern/.zzsetenv.def

# This is necessary for mkpkg to be able to compile things.
#
# You THINK that extern.pkg is written in cl, but it is not really.  It
# must have set commands that can be read by loadenv() in
#  $iraf/unix/boot/bootlib/envinit.c 

# set     @UR_DIR$/variants/common/extern.pkg
# set     @UR_DIR_PKG$/extern.pkg

set     @IRAF_EXTPKG$

