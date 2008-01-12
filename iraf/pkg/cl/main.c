/* Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.
 */

#define	import_spp
#define	import_ctype
#define	import_libc
#define	import_fset
#define	import_main
#define	import_stdio
#define	import_error
#define	import_setjmp
#define	import_knames
#define	import_prtype
#define	import_xwhen
#define import_xnames
#include <iraf.h>

#include "config.h"
#include "grammar.h"
#include "opcodes.h"
#include "operand.h"
#include "param.h"
#include "clmodes.h"
#include "task.h"
#include "errs.h"
#include "mem.h"
#include "ytab.h"
#include "clreadline.h"

#define CLDIR	"cl$"
#define HOSTCONFIG "hconfig$"

/*
 * MAIN -- The main program of the CL.
 *
 * Repetitively call yyparse() and run() until hit eof (or "bye") during
 *   the lowest cl.  The instructions exec and bye change the pc so that
 *   new code is compiled and run in a recursive fashion without having to
 *   call run() itself recursively.
 *
 * TODO:
 *	check access rights of file-type params in inspect.
 *	add < and > chars to mode param.
 *	all the other TODO's and more i'm sure...
 */

#define	FOREGROUND	0
#define	BACKGROUND	1
#define	BKG_QUANTUM	30	/* period(sec) bkgjob checkup		*/
#define	MAX_INTERRUPTS	5	/* max interrupts of a task		*/
#define	LEN_INTRSTK	10	/* max nesting of saved interrupts	*/

int	cldebug = 0;		/* print out lots of goodies if > 0	*/
int	cltrace = 0;		/* trace instruction execution if > 0	*/

memel	cl_dictbuf[DICTSIZE];	/* the dictionary area			*/
static	XSIGFUNC old_onipc;	/* X_IPC handler chained to onint()	*/
static	XINT *jumpcom;		/* IRAF Main setjmp/longjmp buffer	*/
static	jmp_buf jmp_save;	/* save IRAF Main jump vector		*/
static	jmp_buf jmp_clexit;	/* clexit() jumps here			*/
static	int intr_sp;		/* interrupt save stack pointer		*/
static	XSIGFUNC intr_save[LEN_INTRSTK];	/* the interrupt save stack	*/

jmp_buf errenv;			/* cl_error() jumps here		*/
static jmp_buf intenv;		/* X_INT during process jumps here	*/

int	validerrenv;		/* stays 0 until errenv gets set	*/
int	loggingout;		/* set while processing logout file	*/
int	gologout;		/* set when logout() is typed		*/
int	alldone;		/* set by oneof when popping firstask	*/
int	errlev;			/* detect error recursion in CL_ERROR	*/
int	logout_status = 0;	/* optional status arg to logout()	*/
static int	recursion;	/* detect error recursion in ONERROR	*/
static int ninterrupts;		/* number of onint() calls per task	*/
static long cpustart, clkstart;	/* starting cpu, clock times if bkg	*/


static void cl_amovi ( XINT *, XINT *, int );
static void onerr( void );
static void execute( int );
static void login( const char * );
static void logout( void );
static void startup( void );
static void shutdown( void );

/* C_MAIN -- Called by the SPP procedure in cl.x to fire up the CL.
 * In effect we are chained to the IRAF Main, being called immediately after
 * the file system, etc. is initialized.  When we exit we signal that the
 * interpreter be skipped, proceeding directly to process shutdown.
 */
/* prtype  : process type (connected, detached)	*/
/* bkgfile : bkgfile filename if detached	*/
/* cmd     : host command line			*/
XINT c_main ( XINT *prtype, PKCHAR *bkgfile, PKCHAR *cmd )
{
	XPOINTER bp;

	/* Save the setjmp vector of the IRAF Main for restoration at clexit
	 * time.  We need to intercept all errors and do error recovery
	 * ourselves during normal execution, but when the CL exits we are
	 * not prepared to deal with errors occuring during shutdown.
	 */
	XMJBUF (&bp);
	jumpcom = (XINT *)&Memc[bp];
	cl_amovi (jumpcom, jmp_save, LEN_JUMPBUF);

	/* Init clexit() in case we have to panic stop.  */
	if (setjmp (jmp_clexit))
	    goto exit_;

	/* Set up dictionary and catch signals.  If we are background, read in
	 * file and jump right into run, else hand craft first task.  Die if
	 * these fail.
	 */
	startup ();

	if (*prtype == PR_DETACHED) {
	    bkg_startup ((const char *)bkgfile);
	    cpustart = c_cputime (0L);
	    clkstart = c_clktime (0L);
	    execute (BACKGROUND);
	} else {
	    login ((const char *) cmd);
	    execute (FOREGROUND);
	    logout();
	    execute (FOREGROUND);
	}

	shutdown();

exit_:
	/* Return to the IRAF Main.  The PR_EXIT code commands the main to
	 * skip the interpreter loop and shutdown.  Restore the error
	 * jump vector in the IRAF Main so that it can handle errors occuring
	 * during shutdown; we are turning control back over to the Main.
	 * This is ugly, but the real problem is the jump vectors.  There
	 * seems to be no alternative to this sort of thing...
	 */
	cl_amovi (jmp_save, jumpcom, LEN_JUMPBUF);
	return (PR_EXIT | (logout_status << 1));
}


/* CLEXIT -- Called on fatal error from error() when get an error so bad that we
 * should commit suicide.
 */
void clexit( void )
{
	longjmp (jmp_clexit, 1);
}


/* CLSHUTDOWN -- Public entry for shutdown.
 */
void clshutdown( void )
{
	shutdown();
}


/* STARTUP -- CL startup code.  Called by onentry() at process startup.
 *   Allocate space for the dictionary, post exception handlers, initialize
 *   error recovery.
 *
 * NOTE: in the current implementation a fixed size buffer is allocated for
 *   the dictionary due to the difficulty of passing the dictionary to the
 *   bkg CL if a dynamically allocated dictionary is used.  The problem is
 *   that the dictionary is full of pointers to absolute addresses, and
 *   we cannot control where the memory allocator in the bkg CL will allocate
 *   a buffer.  A simple binary copy of the dictionary to different region
 *   of memory in the bkg CL will leave the pointers pointing into limbo.
 *
 * TODO: Write a pair of procedures for each major data structure to dump
 *   and restore the data structure in a binary array.  Passing the CL context
 *   to the bkg CL would then be a matter of calling the dump procedure for
 *   each major data structure to dump the structure into the bkgfile, then
 *   doing a matching restore in the bkg CL to restore the data structure
 *   to a different region of memory.  The ENV package does this already.
 *   The only alternative would be to use indices rather than pointers in
 *   the dictionary, which is not what C likes to do.
 */
static void startup( void )
{
	/* Set up pointers to dictionary buffer.
	 */
	dictionary = cl_dictbuf;
	topd = 0;
	maxd = DICTSIZE;

	if (cldebug)
	    printf ("dictionary starts at %d (0%o)\n", dictionary, dictionary);

	/* Post exception handlers for interrupt and write to IPC with no
	 * reader.  The remaining exceptions use the standard handler.
	 */
	c_xwhen (X_IPC, onipc, &old_onipc);
	intr_reset();

	/* The following is a temporary solution to an initialization problem
	 * with pseudofile i/o.
	 */
	PRPSINIT();

#ifndef NOREADLINE
	read_history(CLHISTORYFILE);
#endif
}


/* SHUTDOWN -- Call this to exit gracefully from the whole cl; never return.
 * Write out any remaining PF_UPDATE'd pfiles by restoring topd to just above
 *   first task unless we are in batch mode, then just flush io and die..
 * So that the restor will include the cl's pfile and any other pfiles that
 *   might have been cached or assigned into, we force its topd to be
 *   below its pfile head.  See the "pfp < topdp" loop in restor().
 * Don't bother with restor'ing if BATCH since we don't want to write out
 *   anything then anyway.
 */
static void shutdown( void )
{
	float	cpu, clk;

	pr_dumpcache (0, YES);		/* flush process cache	*/
	clgflush();			/* flush graphics output */

	if (firstask->t_flags & T_BATCH) {
	    iofinish (currentask);
	    if (notify()) {
		cpu = (float)c_cputime(cpustart) / 1000.;
		clk = (float)c_clktime(clkstart);
		fprintf (stderr, "\n[%d] done  %.1f %.0m %d%%\n", bkgno,
		    cpu, clk/60., (int)((clk > 0 ? cpu / clk : 0.) * 100.));
	    }
	} else {
	    firstask->t_topd = dereference (firstask->t_ltp) + LTASKSIZ;
	    restor (firstask);
	}

	yy_startblock (LOG);			/* flush and close log	*/
	close_logfile (logfile());
#ifndef NOREADLINE
	write_history(CLHISTORYFILE);
#endif
	clexit();
}


/* EXECUTE -- Each loop corresponds to an exec in the interpreted code.
 * This occurs when a script task or process is ready to run.  In background
 * mode, we skip the preliminaries and jump right in and interpret the
 * compiled code.
 */
static void execute ( int mode )
{
	int	parsestat;
	int	old_parhead;

	alldone = 0;
	gologout = 0;
	if (mode == BACKGROUND) {
	    if (setjmp (jumpcom))
		onerr();
	    goto bkg;
	}

	/* Called when control stack contains only the firsttask.  ONEOF sets
	 * alldone true when eof/bye is seen and currentask=firstask,
	 * terminating the loop and returning to main.
	 */
	do {
	    /* Bkg_update() checks for blocked or finished bkg jobs and prints
	     * a message if it finds one.  This involves one or more access()
	     * calls so don't call it more than every 5 seconds.  The errenv
	     * jump vector is used by cl_error() for error restart.  The JUMPCOM
	     * vector is used to intercept system errors which would otherwise
	     * restart the CL.
	     */
	    if (currentask->t_flags & T_INTERACTIVE) {
		static	long last_clktime;

		if (c_clktime (last_clktime) > BKG_QUANTUM) {
		    last_clktime = c_clktime (0L);
		    bkg_update (1);
		}
		validerrenv = 1;
		setjmp (errenv);
		ninterrupts = 0;
		if (setjmp (jumpcom))
		    onerr();
	    } else if (!(currentask->t_flags & T_SCRIPT))
		setjmp (intenv);
    
	    pc = currentask->t_bascode;
	    currentask->t_topd = topd;
	    currentask->t_topcs = topcs;
	    recursion = 0;
	    errlev = 0;
	    c_erract (OK);
	    yeof = 0;

	    /* In the new CL the parser needs to know more about parameters
	     * than before.  Hence param files may be read in during parsing.
	     * Since we discard the dictionary after parsing we must unlink
	     * these param files, and re-read them when the
	     * program is run.  This is inefficient but appears to work.
	     */
	    old_parhead = parhead;

	    if (gologout)
		yeof++;
	    else {
		yy_startblock (LOG);		/* start new history blk   */
		parsestat = yyparse();		/* parse command block	   */
		topd = currentask->t_topd;	/* discard addconst()'s    */
		topcs = currentask->t_topcs;	/* discard compiler temps  */
		parhead = old_parhead;		/* forget param files.	   */
		if (parsestat != 0)
		    cl_error (E_IERR, "parser gagged");
	    }

	    if (dobkg) {
		bkg_spawn (curcmd());
	    } else {
bkg:
		if (yeof)
		    oneof();			/* restores previous task  */
		else {
		    /* set stack above pc, point pc back to code */
		    topos = basos = pc - 1;
		    pc = currentask->t_bascode;
		}

		if (!alldone)
		    run();			/* run code starting at pc */
	    }
	} until (alldone);
}


/* LOGIN -- Hand-craft the first cl process.  Push the first task to become
 * currentask, set up clpackage at pachead and set cl as its first ltask.
 * Add the builtin function ltasks.  Run the startup file as the stdin of cl.
 * If any of this fails, we die.
 */
static void login ( const char *cmd )
{ 
	register struct task *tp;
	char *op;
	char *maxop;
	const char *ip;
	struct	ltask *ltp;
	struct	operand o;
	char	loginfile[SZ_PATHNAME];
	char	alt_loginfile[SZ_PATHNAME];
	char	clstartup[SZ_PATHNAME];
	char	clprocess[SZ_PATHNAME];
	const char *arglist;

	strncpy(loginfile,LOGINFILE,SZ_PATHNAME);
	loginfile[SZ_PATHNAME-1] = EOS;

	snprintf(clstartup, SZ_PATHNAME, "%s%s", HOSTCONFIG, CLSTARTUP);
	snprintf(clprocess, SZ_PATHNAME, "%s%s", CLDIR, CLPROCESS);

	tp = firstask = currentask = pushtask();
	tp->t_in = tp->t_stdin = stdin;
	tp->t_out = tp->t_stdout = stdout;
	tp->t_stderr = stderr;
	tp->t_stdgraph = fdopen (STDGRAPH, "w");
	tp->t_stdimage = fdopen (STDIMAGE, "w");
	tp->t_stdplot  = fdopen (STDPLOT,  "w");
	tp->t_pid = -1;
	tp->t_flags |= (T_INTERACTIVE|T_CL);

	/* Make root package.  Avoid use of newpac() since pointers are not
	 * yet set right.
	 */
	pachead = topd;
	curpack = (struct package *) memneed (PACKAGESIZ);
	curpack->pk_name = comdstr (ROOTPACKAGE);
	curpack->pk_ltp = NULL;
	curpack->pk_pfp = NULL;
	curpack->pk_npk = NULL;
	curpack->pk_flags = 0;

	/* Make first ltask.
	 */
	ltp = newltask (curpack, "cl", clprocess, (struct ltask *) NULL);
	tp->t_ltp = ltp;
	ltp->lt_flags |= (LT_PFILE|LT_CL);

	tp->t_pfp = pfileload (ltp);	/* call newpfile(), read cl.par */
	tp->t_pfp->pf_npf = NULL;
	setclmodes (tp);		/* uses cl's params		*/

	setbuiltins (curpack);		/* add more ltasks off clpackage*/

	/* Define the second package, the "clpackage", and make it the
	 * current package (default package at startup).  Tasks subsequently
	 * defined by the startup script will get put in clpackage.
	 */
	curpack = newpac (CLPACKAGE, "bin$");

	/* Compile code that will run the startup script then, if it exists
 	 * in the current directory, a login.cl script.  We need to do as
	 * much by hand here as the forever loop in main would have if this
	 * code came from calling yyparse().
	 */
	if (c_access (clstartup,0,0) == NO)
	    cl_error (E_FERR, "Cannot find startup file `%s'", clstartup);

	currentask->t_bascode = 0;
	pc = 0;
	o.o_type = OT_STRING;
	o.o_val.v_s = clstartup;
	compile (CALL, "cl");
	compile (PUSHCONST, &o);
	compile (REDIRIN);
	compile (EXEC);
	compile (FIXLANGUAGE);

	/* The following is to permit error recovery in the event that an
	 * error occurs while reading the user's LOGIN.CL file.
	 */
	validerrenv = 1;
	if (setjmp (errenv)) {
	    eprintf ("Error while reading login.cl file");
	    eprintf (" - may need to rebuild with mkiraf\n");
	    eprintf ("Fatal startup error.  CL dies.\n");
	    clexit();
	}
	ninterrupts = 0;
	if (setjmp (jumpcom))
	    onerr();

	/* Nondestructively decompose the host command line into the startup
	 * filename and/or the argument string.
	 */
	if (strncmp (cmd, "-f", 2) == 0) {
	    for (ip=cmd+2;  *ip && isspace(*ip);  ip++)
		;
	    maxop = alt_loginfile + SZ_PATHNAME -1;
	    for ( op=alt_loginfile ; op < maxop && *ip && ! isspace(*ip) ;
		  op++, ip++ )
		*op = *ip;
	    *op = EOS;

	    for (  ;  *ip && isspace(*ip);  ip++)
		;
	    arglist = ip;

	} else {
	    *alt_loginfile = EOS;
	    arglist = cmd;
	}

	/* Copy any user supplied host command line arguments into the
	 * CL parameter $args to use in the startup script (for instance).
	 */
	o.o_type = OT_STRING;
	strncpy (o.o_val.v_s, arglist, SZ_PATHNAME);
	o.o_val.v_s[SZ_PATHNAME-1] = EOS;
	compile (PUSHCONST, &o);
	compile (ASSIGN, "args");

	if (alt_loginfile[0]) {
	    if (c_access (alt_loginfile,0,0) == NO)
		printf ("Warning: script file %s not found\n", alt_loginfile);
	    else {
		o.o_val.v_s = alt_loginfile;
		compile (CALL, "cl");
		compile (PUSHCONST, &o);
		compile (REDIRIN);
		compile (EXEC);
	    }

	} else if (c_access (loginfile,0,0) == NO) {
	    printf ("Warning: no login.cl found in login directory\n");

	} else {
	    o.o_val.v_s = loginfile;
	    compile (CALL, "cl");
	    compile (PUSHCONST, &o);
	    compile (REDIRIN);
	    compile (EXEC);
	}

	compile (END);
	topos = basos = pc - 1;
	pc = 0;
	run();			/* returns after doing the first EXEC	*/

	/* Add nothing here that will effect the dictionary or the stacks.
	 */
	if (cldebug)
	    printf ("topd, pachead, parhead: %u, %u, %u\n",
		topd, pachead, parhead);
}


/* LOGOUT -- Process the system logout file.  Called when the user logs
 * off in an interactive CL (not called by bkg cl's).  The standard input
 * of the CL is hooked to the system logout file and when the eof of the
 * logout file is seen the CL really does exit.
 */
static void logout ( void )
{ 
	register struct task *tp;
	char	logoutfile[SZ_PATHNAME];
	FILE	*fp;

	snprintf(logoutfile, SZ_PATHNAME, "%s%s", HOSTCONFIG, CLLOGOUT);

	if ((fp = fopen (logoutfile, "r")) == NULL)
	    cl_error (E_FERR,
		"Cannot open system logout file `%s'", logoutfile);

	tp = firstask;
	tp->t_in = tp->t_stdin = fp;
	yyin = fp;
	tp->t_flags = (T_CL|T_SCRIPT);
	loggingout = 1;
	gologout = 0;
}


/* MEMNEED -- Increase topd by incr INT's.  Since at present the dictionary
 * is fixed in size, abort if the dictionary overflows.
 */
/* incr : amount of space desired in ints, not bytes	*/
char *memneed ( int incr )
{
	memel *old;

	old = daddr (topd);
	topd += incr;

        /* Quad alignment is desirable for some architectures. */
        if (topd & 1)
            topd++;

	if (topd > maxd)
	    cl_error (E_IERR, "dictionary full");

	return ((char *)old);
}


/* ONINT -- Called when the interrupt exception occurs, i.e., the usual user
 *   attention-getter.  (cntrl-c on dec, delete on unix, etc.).  Also called
 *   when we are killed as a bkg job.
 * If the current task is a script or the terminal, abort execution and
 *   initiate error recovery.  If the task is in a child process merely send
 *   interrupt to the child and continue execution (giving the child a chance
 *   to cleanup before calling error, or to ignore the interrupt entirely).
 *   If the task wants to terminate it will send the ERROR statement to the CL.
 * If we are a bkg job, call bkg_abort to clean up (delete temp files, etc.)
 *   before shutting down.
 */
/* ARGSUSED */
/* vex          : virtual exception code	*/
/* next_handler : next handler to be called	*/
static void onint ( XINT *vex, void (**next_handler)() )
{
	if (firstask->t_flags & T_BATCH) {
	    /* Batch task.
	     */
	    iofinish (currentask);
	    bkg_abort();
	    clexit();

	} else if (currentask->t_flags & (T_SCRIPT|T_CL|T_BUILTIN)) {
	    /* CL task.
	     */
	    cl_error (E_UERR, "interrupt!!!");

	} else {
	    /* External task connected via IPC.  Pass the interrupt on to
	     * the child.
	     */
	    c_prsignal (currentask->t_pid, X_INT);

	    /* Cancel any output and disable i/o on the tasks pseudofiles.
	     * This is necessary to cancel any i/o still buffered in the
	     * IPC channel.  Commonly when the task is writing to STDOUT,
	     * for example, the CL will be writing the last buffer sent
	     * to the terminal, while the task waits after having already
	     * pushed the next buffer into the IPC.  When we resume reading
	     * from the task we will see this buffered output on the next
	     * read and we wish to discard it.  Leave STDERR connected to
	     * give a path to the terminal for recovery actions such as
	     * turning standout or graphics mode off.  This gives the task
	     * a chance to cleanup but does not permit full recovery.  The
	     * pseudofiles will be reconnected for the next task run.
	     */
	    c_fseti (fileno(stdout),            F_CANCEL, OK);
	    c_fseti (fileno(currentask->t_in),  F_CANCEL, OK);
	    c_fseti (fileno(currentask->t_out), F_CANCEL, OK);

	    c_prredir (currentask->t_pid, STDIN, 0);
	    c_prredir (currentask->t_pid, STDOUT, 0);

	    /* If a subprocess is repeatedly interrupted we assume that it
	     * is hung in a loop and abort, advising the user to kill the
	     * process.
	     */
	    if (++ninterrupts >= MAX_INTERRUPTS)
		cl_error (E_UERR, "subprocess is hung; should be killed");
	    else
		longjmp (intenv, 1);
	}

	*next_handler = NULL;
}


/* INTR_DISABLE -- Disable interrupts, e.g., to protect a critical section
 * of code.
 */
void intr_disable( void )
{
	if (intr_sp >= LEN_INTRSTK)
	    cl_error (E_IERR, "interrupt save stack overflow");
	c_xwhen (X_INT, X_IGNORE, &intr_save[intr_sp++]);
}


/* INTR_ENABLE -- Reenable interrupts, reposting the interrupt vector saved
 * in a prior call to INTR_DISABLE.
 */
void intr_enable( void )
{
	XSIGFUNC junk;
	int sp_ok;

	if ( 0 < intr_sp ) sp_ok=1;
	else sp_ok=0;

	intr_sp--;
	if ( sp_ok == 0 )
	    cl_error (E_IERR, "interrupt save stack underflow");
	c_xwhen (X_INT, intr_save[intr_sp], &junk);
}


/* INTR_RESET -- Post the interrupt handler and clear the interrupt vector
 * save stack.
 */
void intr_reset( void )
{
	XSIGFUNC junk;

	c_xwhen (X_INT, &onint, &junk);
	intr_sp = 0;
}


/* ONERR -- Called when system error recovery takes place.  The setjmp in
 * execute() overrides the setjmp (ZSVJMP) in the IRAF Main.  When system error
 * recovery takes place, c_erract() calls ZDOJMP to restart the IRAF Main.
 * We do not want to lose the runtime context of the CL, so we restart the
 * CL main instead by intercepting the vector.  We get the error message from
 * the system and call cl_error() which eventually does a longjmp back to
 * the errenv in execute().
 */
static void onerr( void )
{
	char	errmsg[SZ_LINE];

	c_erract (EA_RESTART);
	c_errget (errmsg, SZ_LINE);

	if (recursion++)
	    longjmp (errenv, 1);
	else
	    cl_error (E_UERR, errmsg);
}


/* CL_AMOVI -- Copy an integer sized block of memory.
 */
static void cl_amovi ( XINT *ip, XINT *op, int len )
{
	while (--len)
	    *op++ = *ip++;
}