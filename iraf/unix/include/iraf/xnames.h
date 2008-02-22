#ifndef	_IRAF_XNAMES_H
#define	_IRAF_XNAMES_H

/*
 * XNAMES.H -- C callable external names of the SPP library procedures.
 * The C version of the name is identical to the SPP name except that it is
 * given as a macro in upper case.  The definition is the host system external
 * name of the Fortran procedure.  The trailing underscore in these names is
 * UNIX dependent; other systems use a leading underscore, or no special
 * characters at all (the purpose of the underscore on UNIX systems is to
 * avoid name collisions between C and Fortran procedures, since the F77
 * runtime library on UNIX is built on the UNIX/C library).  Change the names
 * in the column at the right if your system employs a different convention.
 *
 * If your system does not employ something like the underscore to avoid
 * name collisions, name collisions can be expected.  To fix these change
 * the name given here and add a define to hconfig$iraf.h to change the 
 * external name generated by the preprocessor.  It is NOT necessary to resolve
 * name collisions by changing the actual program sources.
 *
 * The external names defined herein MUST agree with those in 
 * "hinclude$iraf.h".
 */

#include <iraf/spptypes.h>

/* Error handling.
 */
#define	XERPSH		xerpsh_
#define	XERPOP		xerpop_
#define	XERPOPI		xerpoi_
/* ../../../sys/etc/xerpop.x ; for iferr() macro. */
extern int XERPSH( void );
extern XBOOL XERPOP( void );
extern XINT XERPOPI( void );

#ifndef F2C_INCLUDE		/* for native C code */
#define	iferr(stmt)	{XERPSH();stmt;}if(XERPOPI())
#endif

#define	ACCESS		xfaccs_		/* to avoid name collisions */
#define	CALLOC		xcallc_
#define	CLOSE		xfcloe_
#define	DELETE		xfdele_
#define	ERROR		xerror_
#define	FLUSH		xffluh_
#define	GETC		xfgetc_
#define	GETCHAR		xfgetr_
#define	MALLOC		xmallc_
#define	MFREE		xmfree_
#define	MKTEMP		xmktep_
#define	NOTE		xfnote_
#define	OPEN		xfopen_
#define	PRINTF		xprinf_
#define	PUTC		xfputc_
#define	PUTCHAR		xfputr_
#define	QSORT		xqsort_
#define	READ		xfread_
#define	REALLOC		xrealc_
#define	SEEK		xfseek_
#define	SIZEOF		xsizef_
#define	UNGETC		xfungc_
#define	WRITE		xfwrie_

/* ../../../sys/fio/access.x */
extern XINT ACCESS ( XCHAR *, XINT *, XINT * );
/* ../../../sys/fio/delete.x */
extern int DELETE ( XCHAR * );
/* ../../../sys/fio/close.x */
extern int CLOSE ( XINT * );
/* ../../../sys/etc/error.x */
extern int ERROR ( XINT *, XCHAR * );
/* ../../../sys/fio/flush.x */
extern int FLUSH ( XINT * );
/* ../../../sys/memio/mfree.x */
extern int MFREE ( XPOINTER *, XINT * );
/* ../../../sys/memio/malloc.x */
extern int MALLOC ( XPOINTER *, XSIZE_T *, XINT * );
/* ../../../sys/memio/realloc.x */
extern int REALLOC ( XPOINTER *, XSIZE_T *, XINT * );


#define	AREAD		aread_		/* other VOS names */
#define	AREADB		areadb_
#define	AWAIT		await_
#define	AWAITB		awaitb_
#define	AWRITE		awrite_
#define	AWRITEB		awritb_
#define	BEGMEM		begmem_
#define	BRKTIME		brktie_
#define	BTOI		btoi_
#define	CLKTIME		clktie_
#define	CNVDATE		cnvdae_
#define	CNVTIME		cnvtie_
#define	COERCE		coerce_
#define	CPUTIME		cputie_
#define	CTOD		ctod_
#define	CTOX		ctox_
#define	DIROPEN		diropn_
#define	DTOC		dtoc_
#define	ENVFIND		envfid_
#define	ENVFREE		envfre_
#define	ENVGETB		envgeb_
#define	ENVGETI		envgei_
#define	ENVGETS		envges_
#define	ENVINIT		envint_
#define	ENVLIST		envlit_
#define ENVMARK		envmak_
#define	ENVPUTS		envpus_
#define	ENVRESET	envret_
#define	ENVSCAN		envscn_
#define	ERRACT		erract_
#define	ERRCODE		errcoe_
#define	ERRGET		errget_
#define	FALLOC		falloc_
#define	FATAL		xfatal_
#define	FCHDIR		xfchdr_
#define	FCOPY		fcopy_
#define	FCOPYO		fcopyo_
#define	FDEBUG		fdebug_
#define	FDELPF		fdelpf_
#define	FDEVBLK		fdevbk_
#define	FDIRNAME	fdirne_
#define	FILBUF		filbuf_
#define	FINFO		finfo_
#define	FIXMEM		fixmem_
#define	FLSBUF		flsbuf_
#define	FMAPFN		fmapfn_
#define	FMKDIR		fmkdir_
#define	FNEXTN		fnextn_
#define	FNLDIR		fnldir_
#define	FNROOT		fnroot_
#define	FNTCLS		fntcls_
#define	FNTGFN		fntgfn_
#define	FNTOPN		fntopn_
#define	FOWNER		fowner_
#define	FPATHNAME	fpathe_
#define	FPRINTF		fprinf_
#define	FREDIR		fredir_
#define	FSETI		fseti_
#define	FSTATI		fstati_
#define	FSTATL		fstatl_
#define	FSTATS		fstats_
#define	GETPID		xgtpid_
#define	GCTOD		gctod_
#define	GCTOL		gctol_
#define	GCTOX		gctox_
#define	GETLINE		getlie_
#define	GETUID		xgtuid_
#define	GLTOC		gltoc_
#define	GPATMAKE	gpatme_
#define	GPATMATCH	gpatmh_
#define	GSTRMATCH	gstrmh_
#define	GTR_GFLUSH	gtrgfh_
#define	IMACCESS	imaccs_
#define	IMDRCUR		imdrcr_
#define	IRAF_MAIN	irafmn_
#define	XISATTY		xisaty_
#define	XTTYSIZE	xttyse_
#define	ITOB		itob_
#define	KI_EXTNODE	kiexte_
#define	KI_MAPCHAN	kimapn_
#define	LEXNUM		lexnum_
#define	LPOPEN		lpopen_
#define	NDOPEN		ndopen_
#define	ONENTRY		onenty_
#define	ONERROR		onerrr_
#define	ONEXIT		onexit_
#define	OSCMD		oscmd_
#define	PARGB		pargb_
#define	PARGC		pargc_
#define	PARGD		pargd_
#define	PARGI		pargi_
#define	PARGL		pargl_
#define	PARGR		pargr_
#define	PARGS		pargs_
#define	PARGSTR		pargsr_
#define	PARGX		pargx_
#define POLL            xfpoll_
#define POLL_OPEN       pollon_
#define POLL_CLOSE      pollce_
#define POLL_ZERO       pollzo_
#define POLL_SET        pollst_
#define POLL_CLEAR      pollcr_
#define POLL_TEST       polltt_
#define POLL_GET_NFDS   pollgs_
#define POLL_PRINT      pollpt_
#define	PRCHDIR		prchdr_
#define	PRCLCPR		prclcr_
#define	PRCLDPR		prcldr_
#define	PRCLOSE		prcloe_
#define	PRDONE		prdone_
#define	PRENVFREE	prenve_
#define	PRENVSET	prenvt_
#define	PRFILBUF	prfilf_
#define	PRKILL		prkill_
#define	PROPCPR		propcr_
#define	PROPDPR		propdr_
#define	PROPEN		propen_
#define	PROTECT		protet_
#define	PRREDIR		prredr_
#define	PRSIGNAL	prsigl_
#define	PRSTATI		prstai_
#define	PRUPDATE	prupde_
#define	PRPSINIT	prpsit_
#define	PUTCC		putcc_
#define	PUTLINE		putlie_
#define	RCURSOR		rcursr_
#define	RDUKEY		rdukey_
#define	RENAME		xfrnam_
#define	REOPEN		reopen_
#define	SALLOC		salloc_
#define	SFREE		sfree_
#define	SMARK		smark_
#define	SPRINTF		sprinf_
#define	STG_GETLINE	stggee_
#define	STG_PUTLINE	stgpue_
#define	STKCMP		stkcmp_
#define	STRMATCH	strmah_
#define	STROPEN		stropn_
#define STRSETMODE	strsee_
#define	STRGETMODE	strgee_
#define STRCLOSE	strcle_
#define	STRTBL		strtbl_
#define	STTYCO		sttyco_
#define	SYSRUK		sysruk_
#define	TSLEEP		tsleep_
#define	TTSETI		ttseti_
#define	TTSETS		ttsets_
#define	TTSTATI		ttstai_
#define	TTSTATS		ttstas_
#define	TTYCDES		ttycds_
#define	TTYCLEAR	ttyclr_
#define	TTYCLEARLN	ttycln_
#define	TTYCLOSE	ttycls_
#define	TTYCTRL		ttyctl_
#define	TTYGDES		ttygds_
#define	TTYGETB		ttygeb_
#define	TTYGETI		ttygei_
#define	TTYGETR		ttyger_
#define	TTYGETS		ttyges_
#define	TTYGOTO		ttygoo_
#define	TTYINIT		ttyint_
#define	TTYODES		ttyods_
#define	TTYOPEN		ttyopn_
#define	TTYPUTLINE	ttypue_
#define	TTYPUTS		ttypus_
#define	TTYSETI		ttysei_
#define	TTYSO		ttyso_
#define	TTYSTATI	ttysti_
#define	UNGETLINE	ungete_
#define	UNREAD		unread_
#define	URAND		urand_
#define	VFNOPEN		vfnopn_
#define	VFNCLOSE	vfncle_
#define	VFNMAP		vfnmap_
#define	VFNADD		vfnadd_
#define	VFNDEL		vfndel_
#define	VFNUNMAP	vfnunp_
#define	VMALLOC		vmallc_
#define	XALLOCATE	xalloe_
#define	XDEALLOCATE	xdeale_
#define	XDEVOWNER	xdevor_
#define	XDEVSTATUS	xdevss_
#define XER_RESET	xerret_
#define	XMJBUF		xmjbuf_
#define	XONERR		xonerr_
#define	XTOC		xtoc_
#define	XWHEN		xwhen_

/* ../../../sys/etc/btoi.x */
extern XINT BTOI ( XBOOL * );
/* ../../../sys/etc/cnvdate.x */
extern int CNVDATE ( XLONG *, XCHAR *, XINT * );
/* ../../../sys/etc/cnvtime.x */
extern int CNVTIME ( XLONG *, XCHAR *, XINT * );
/* ../../../sys/fmtio/ctod.x */
extern XINT CTOD( XCHAR *, XINT *, XDOUBLE * );
/* ../../../sys/etc/clktime.x */
extern XLONG CLKTIME ( XLONG * );
/* ../../../sys/etc/clktime.x */
extern XLONG CPUTIME ( XLONG * );
/* ../../../sys/etc/environ.x */
extern XINT ENVFIND ( XCHAR *, XCHAR *, XINT * );
extern XINT ENVPUTS ( XCHAR *, XCHAR * );
extern int ENVMARK ( XINT * );
extern XINT ENVFREE ( XINT *, XPOINTER * );
/* ../../../sys/etc/envgets.x */
extern XINT ENVGETS ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/etc/envgetb.x */
extern XBOOL ENVGETB ( XCHAR * );
/* ../../../sys/etc/envgeti.x */
extern XINT ENVGETI ( XCHAR * );
/* ../../../sys/etc/envinit.c */
extern int ENVINIT ( void );
/* ../../../sys/etc/envreset.x */
extern int ENVRESET ( XCHAR *, XCHAR * );
/* ../../../sys/etc/envscan.x */
extern XINT ENVSCAN ( XCHAR * );
/* ../../../sys/etc/envlist.x */
extern int ENVLIST ( XINT *, XCHAR *, XINT * );
/* ../../../sys/etc/erract.x */
extern int ERRACT ( XINT * );
/* ../../../sys/etc/errcode.x */
extern XINT ERRCODE( void );
/* ../../../sys/etc/errget.x */
extern XINT ERRGET ( XCHAR *, XINT * );
/* ../../../sys/etc/error.x */
extern int FATAL ( XINT *, XCHAR * );
/* ../../../sys/fio/fchdir.x */
extern int FCHDIR ( XCHAR * );
/* ../../../sys/etc/prenvfree.x */
extern XINT PRENVFREE ( XINT *, XINT * );
/* ../../../sys/etc/prfilbuf.x */
extern XINT PRFILBUF ( XINT * );
/* ../../../sys/fio/filbuf.x */
extern XLONG FILBUF ( XINT * );
/* ../../../sys/fio/finfo.x */
extern XINT FINFO ( XCHAR *, XLONG * );
/* ../../../sys/fio/flsbuf.x */
extern int FLSBUF ( XINT *, XLONG * );
/* ../../../sys/fio/fmapfn.x */
extern int FMAPFN ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/fio/fmkdir.x */
extern int FMKDIR ( XCHAR * );
/* ../../../sys/fio/fnextn.x */
extern XINT FNEXTN ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/fio/fnldir.x */
extern XINT FNLDIR ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/fio/fnroot.x */
extern XINT FNROOT ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/fio/fpathname.x */
extern int FPATHNAME ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/fio/fredir.x */
extern int FREDIR ( XINT *, XCHAR *, XINT *, XINT * );
/* ../../../sys/fio/fseti.x */
extern int FSETI ( XINT *, XINT *, XINT * );
/* ../../../sys/fio/fstati.x */
extern XINT FSTATI ( XINT *, XINT * );
/* ../../../sys/etc/getpid.x */
extern XINT GETPID ( void );
/* ../../../sys/etc/getuid.x */
extern int GETUID ( XCHAR *, XINT * );
/* ../../../sys/gio/cursor/gtrgflush.x */
extern int GTR_GFLUSH ( XINT * );
/* ../../../sys/imio/imaccess.x */
extern XINT IMACCESS ( XCHAR *, XINT * );
/* ../../../pkg/images/tv/display/imdrcur.x */
extern XINT IMDRCUR ( XCHAR *, XREAL *, XREAL *, XINT *, XINT *,
		      XCHAR *, XINT *, XINT *, XINT * );
/* ../../../sys/ki/kimapchan.x */
extern XINT KI_MAPCHAN ( XINT *, XCHAR *, XINT * );
/* ../../../sys/fmtio/lexnum.x */
extern XINT LEXNUM ( XCHAR *, XINT *, XINT * );
/* ../../../sys/fio/mktemp.x */
extern int MKTEMP ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/fio/ndopen.x */
extern XINT NDOPEN ( XCHAR *, XINT * );
/* ../../../sys/fio/note.x */
extern XLONG NOTE ( XINT * );
/* ../../../sys/fio/open.x */
extern XINT OPEN ( XCHAR *, XINT *, XINT * );
/* ../../../sys/etc/oscmd.x */
extern XINT OSCMD ( XCHAR *, XCHAR *, XCHAR *, XCHAR * );
/* ../../../sys/fio/poll.x */
extern XPOINTER POLL_OPEN ( void );
extern XINT POLL ( XPOINTER *, XINT *, XINT * );
extern int POLL_CLOSE ( XPOINTER * );
extern int POLL_ZERO ( XPOINTER * );
extern int POLL_SET ( XPOINTER *, XINT *, XINT * );
extern XINT POLL_GET_NFDS ( XPOINTER * );
extern int POLL_CLEAR ( XPOINTER *, XINT *, XINT * );
extern XINT POLL_TEST ( XPOINTER *, XINT *, XINT * );
extern int POLL_PRINT ( XPOINTER * );
/* ../../../sys/etc/propen.x */
extern XINT PROPEN ( XCHAR *, XINT *, XINT * );
/* ../../../sys/etc/prclose.x */
extern XINT PRCLOSE ( XINT * );
/* ../../../sys/etc/prstati.x */
extern XINT PRSTATI ( XINT *, XINT * );
/* ../../../sys/etc/prsignal.x */
extern int PRSIGNAL ( XINT *, XINT * );
/* ../../../sys/etc/prredir.x */
extern int PRREDIR ( XINT *, XINT *, XINT * );
/* ../../../sys/etc/prchdir.x */
extern int PRCHDIR ( XINT *, XCHAR * );
/* ../../../sys/etc/prenvset.x */
extern int PRENVSET ( XINT *, XCHAR *, XCHAR * );
/* ../../../sys/etc/propdpr.x */
extern XINT PROPDPR ( XCHAR *, XCHAR *, XCHAR * );
/* ../../../sys/etc/prcldpr.x */
extern XINT PRCLDPR ( XINT * );
/* ../../../sys/etc/prdone.x */
extern XINT PRDONE ( XINT * );
/* ../../../sys/etc/prkill.x */
extern int PRKILL ( XINT * );
/* ../../../sys/fmtio/printf.x */
extern int PRINTF ( XCHAR * );
/* ../../../sys/fmtio/fprintf.x */
extern int FPRINTF ( XINT *, XCHAR * );
/* ../../../sys/fmtio/parg.x */
extern int PARGI ( XINT * );
extern int PARGC ( XCHAR * );
extern int PARGS ( XSHORT * );
extern int PARGL ( XLONG * );
extern int PARGR ( XREAL * );
extern int PARGD ( XDOUBLE * );
/* ../../../sys/fmtio/pargstr.x */
extern int PARGSTR ( XCHAR * );
/* ../../../sys/gio/cursor/rcursor.x */
extern XINT RCURSOR ( XINT *, XCHAR *, XINT * );
/* ../../../sys/clio/rdukey.x */
extern XINT RDUKEY ( XCHAR *, XINT * );
/* ../../../sys/fio/read.x */
extern XLONG READ ( XINT *, XCHAR *, XSIZE_T * );
/* ../../../sys/fio/rename.x */
extern int RENAME ( XCHAR *, XCHAR * );
/* ../../../sys/fio/reopen.x */
extern XINT REOPEN ( XINT *, XINT * );
/* ../../../sys/memio/salloc.x */
extern int SALLOC ( XPOINTER *, XSIZE_T *, XINT * );
extern int SMARK ( XPOINTER * );
extern int SFREE ( XPOINTER * );
/* ../../../sys/fio/seek.x */
extern int SEEK ( XINT *, XLONG * );
/* ../../../sys/fio/stropen.x */
extern XINT STROPEN ( XCHAR *, XINT *, XINT * );
extern int STRCLOSE ( XINT * );
extern int STRSETMODE ( XINT *, XINT * );
extern XINT STRGETMODE ( XINT * );
/* ../../../sys/etc/tsleep.x */
extern int TSLEEP ( XINT * );
/* ../../../sys/etc/sttyco.x */
extern int STTYCO ( XCHAR *, XINT *, XINT *, XINT * );
/* ../../../sys/etc/ttopen.x */
extern int TTSETI ( XINT *, XINT *, XINT * );
extern XINT TTSTATI ( XINT *, XINT * );
extern int TTSETS ( XINT *, XINT *, XCHAR * );
extern XINT TTSTATS ( XINT *, XINT *, XCHAR *, XINT * );
/* ../../../sys/tty/ttycdes.x */
extern int TTYCDES ( XPOINTER * );
/* ../../../sys/tty/ttyclear.x */
extern int TTYCLEAR ( XINT *, XPOINTER * );
/* ../../../sys/tty/ttyclln.x */
extern int TTYCLEARLN ( XINT *, XPOINTER * );
/* ../../../sys/tty/ttyctrl.x */
extern XINT TTYCTRL ( XINT *, XPOINTER *, XCHAR *, XINT * );
/* ../../../sys/tty/ttygetb.x */
extern XBOOL TTYGETB ( XPOINTER *, XCHAR * );
/* ../../../sys/tty/ttygeti.x */
extern XINT TTYGETI ( XPOINTER *, XCHAR * );
/* ../../../sys/tty/ttygetr.x */
extern XREAL TTYGETR ( XPOINTER *, XCHAR * );
/* ../../../sys/tty/ttygets.x */
extern XINT TTYGETS ( XPOINTER *, XCHAR *, XCHAR *, XINT * );
/* ../../../sys/tty/ttygoto.x */
extern int TTYGOTO ( XINT *, XPOINTER *, XINT *, XINT * );
/* ../../../sys/tty/ttyinit.x */
extern int TTYINIT ( XINT *, XPOINTER * );
/* ../../../sys/tty/ttyodes.x */
extern XPOINTER TTYODES ( XCHAR * );
/* ../../../sys/tty/ttyputl.x */
extern int TTYPUTLINE ( XINT *, XPOINTER *, XCHAR *, XINT * );
/* ../../../sys/tty/ttyputs.x */
extern int TTYPUTS ( XINT *, XPOINTER *, XCHAR *, XINT * );
/* ../../../sys/tty/ttyseti.x */
extern int TTYSETI ( XPOINTER *, XINT *, XINT * );
/* ../../../sys/tty/ttyso.x */
extern int TTYSO ( XINT *, XPOINTER *, XINT * );
/* ../../../sys/tty/ttystati.x */
extern XINT TTYSTATI ( XPOINTER *, XINT * );
/* ../../../sys/fio/ungetc.x */
extern int UNGETC ( XINT *, XCHAR * );
/* ../../../sys/fio/ungetline.x */
extern int UNGETLINE ( XINT *, XCHAR * );
/* ../../../sys/fio/write.x */
extern int WRITE ( XINT *, XCHAR *, XSIZE_T * );
/* ../../../sys/etc/onerror.x */
extern int XONERR ( XINT * );
/* ../../../sys/etc/xttysize.x */
extern int XTTYSIZE ( XINT *, XINT * );
/* ../../../sys/etc/xwhen.x */
extern int XWHEN ( XINT *, XINT *, XINT * );
/* ../../../sys/etc/onentry.x */
extern XINT ONENTRY ( XINT *, XCHAR *, XCHAR * );
/* ../../../base/sysruk.x */
extern XINT SYSRUK ( XCHAR *, XCHAR *, XINT *, XINT * );
/* ../../../sys/fio/vfnmap.x */
extern XPOINTER VFNOPEN ( XCHAR *, XINT * );
extern XINT VFNUNMAP ( XPOINTER *, XCHAR *, XCHAR *, XINT * );
extern int  VFNCLOSE ( XPOINTER *, XINT * );
extern XINT VFNADD ( XPOINTER *, XCHAR *, XINT * );
extern XINT VFNMAP ( XPOINTER *, XCHAR *, XINT * );
/* ../../../sys/etc/xalloc.x */
extern XINT XALLOCATE( XCHAR * );
extern XINT XDEALLOCATE ( XCHAR *, XINT * );
extern int XDEVSTATUS ( XCHAR *, XINT * );
extern XINT XDEVOWNER ( XCHAR *, XCHAR *, XINT * );
/* ../../../sys/etc/xisatty.x */
extern XINT XISATTY ( XINT * );
/* ../../../sys/gio/stdgraph/stgrtty.x */
extern XINT STG_GETLINE ( XINT *, XCHAR * );
/* ../../../gio/stdgraph/stgwtty.x */
extern int STG_PUTLINE ( XINT *, XCHAR * );
/* ../../../sys/etc/xerreset.x */
extern int XER_RESET ( void );
/* ../../../sys/etc/xmjbuf.x */
extern int XMJBUF ( XPOINTER * );
/* ../../../sys/gio/cursor/prpsinit.x */
extern int PRPSINIT( void );

#define	IMPAKD		impakd_
#define	IMPAKI		impaki_
#define	IMPAKL		impakl_
#define	IMPAKR		impakr_
#define	IMPAKS		impaks_
#define	IMPAKX		impakx_

#define	IMUPKD		imupkd_
#define	IMUPKI		imupki_
#define	IMUPKL		imupkl_
#define	IMUPKR		imupkr_
#define	IMUPKS		imupks_
#define	IMUPKX		imupkx_

extern int IMPAKD ( XDOUBLE *, void *, XSIZE_T *, XINT * );
extern int IMPAKI ( XINT *, void *, XSIZE_T *, XINT * );
extern int IMPAKL ( XLONG *, void *, XSIZE_T *, XINT * );
extern int IMPAKR ( XREAL *, void *, XSIZE_T *, XINT * );
extern int IMPAKS ( XSHORT *, void *, XSIZE_T *, XINT * );
extern int IMPAKX ( XCOMPLEX *, void *, XSIZE_T *, XINT * );

extern int IMUPKD ( void *, XDOUBLE *, XSIZE_T *, XINT * );
extern int IMUPKI ( void *, XINT *, XSIZE_T *, XINT * );
extern int IMUPKL ( void *, XLONG *, XSIZE_T *, XINT * );
extern int IMUPKR ( void *, XREAL *, XSIZE_T *, XINT * );
extern int IMUPKS ( void *, XSHORT *, XSIZE_T *, XINT * );
extern int IMUPKX ( void *, XCOMPLEX *, XSIZE_T *, XINT * );

#endif	/* ! _IRAF_XNAMES_H */
