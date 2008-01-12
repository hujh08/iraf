/* Copyright(c) 1986 Association of Universities for Research in Astronomy Inc.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define import_spp
#include <iraf.h>

#include "../../bootlib/bootlib.h"
#include "xpp.h"


/*
 * C code for the first pass of the IRAF subset preprocessor (SPP).
 * The decision to initially organize the SPP compiler into two passes was
 * made to permit maximum use of the existing raftor preprocessor, which is
 * the basis for the second pass of the SPP.  Eventually the two passes
 * should be combined into a single program.  Most of the operations performed
 * by the first pass (XPP) should be performed AFTER macro substitution,
 * rather than before as is the case in the current implementation, which
 * processes macros in the second pass (RPP).
 *
 * Beware that this is not a very good program which was not carefully
 * designed and which was never intended to have a long lifetime.  The next
 * step is to replace the two passes by a single program which is functionally
 * very similar, but which is more carefully engineered and which is written
 * in the SPP language calling IRAF file i/o.  Eventually a true compiler
 * will be written, providing many new features, i.e., structures and pointers,
 * automatic storage class, mapped arrays, enhanced i/o support, and good
 * compile time error checking.  This compiler will also feature a table driven
 * code generator (generating primitive Fortran statements), which will provide
 * greater machine independence.
 */

#define F77			/* Fortran 77 target compiler?		*/

#define IRAFBASE	"iraf$base/"
#define HOSTCONFIG	"host$config/"
#define HBIN_INCLUDES	"hbin$arch_includes/"

/* Size limiting definitions.
 */
#define MAX_TASKS	100	/* max no. of tasks we can handle	*/
#define SZ_OBUF		131072	/* buffers procedure body		*/
#define SZ_DBUF		8192	/* for errchk, common, ect. decls	*/
#define SZ_SBUF		8192	/* buffers text of strings		*/
#define MAX_STRINGS	256	/* max strings in a procedure		*/
#define MAX_INCLUDE	5	/* maximum nesting of includes		*/
#define MIN_REALPREC	7	/* used by HMS				*/
#define SZ_NUMBUF	32	/* for numeric constants		*/
#define SZ_STBUF	4096	/* text of defined strings		*/
#define MAX_DEFSTR	128	/* max defined strings			*/

#define RUNTASK		"sysruk.x"
#define OCTAL		8
#define DECIMAL		10
#define HEX		16
#define CHARCON		1
#define SEXAG		2

/* Escape sequence characters and their binary equivalents.
 */
static const char *esc_ch  = "ntfr\\\"'";
static const char *esc_val = "\n\t\f\r\\\"\'";

#define U(x) x
#define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10\
?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
#define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}

int context = GLOBAL;			/* lexical context variable	*/

static const char *machdefs[] = { "mach.h", "config.h", /* ??? "" ??? */ NULL };

/* The task structure is used for TASK declarations.  Since this is a
 * throwaway program we do not bother with dynamic storage allocation,
 * which would remove the limit on the number of tasks in a task statment.
 */
struct task {
	char	*task_name;		/* logical task name		*/
	char	*proc_name;		/* name of procedure		*/
	short	name_offset;		/* offset of name in dictionary	*/
};

/* The string structure is used for STRING declarations and for inline
 * strings.  Strings are stored in a fixed size, statically allocated
 * string buffer.
 */
struct string {
	char	*str_name;		/* name of string		*/
	char	*str_text;		/* ptr to text of string	*/
	short	str_length;		/* length of string		*/
};

static struct task task_list[MAX_TASKS];
static struct string string_list[MAX_STRINGS];

int linenum[MAX_INCLUDE];		/* line numbers in files	    */
char fname[MAX_INCLUDE][SZ_PATHNAME];	/* file names			    */
int istkptr = 0;			/* istk pointer			    */
static FILE *istk[MAX_INCLUDE];		/* stack for input file descriptors */

static char obuf[SZ_OBUF];		/* buffer for body of procedure     */
static char dbuf[SZ_DBUF];		/* buffer for misc proc. decls.	    */
static char sbuf[SZ_SBUF];		/* string buffer		    */
static char *sbufptr = sbuf;		/* string buffer pointer	    */
static char *maxsbufptr = sbuf + SZ_SBUF -1;
static char *obufptr = obuf;		/* pointer in output buffer	    */
static char *maxobufptr = obuf + SZ_OBUF -1;
static char *dbufptr = dbuf;		/* pointer in decls buffer	    */
static char *maxdbufptr = dbuf + SZ_DBUF -1;
static int nstrings = 0;		/* number of strings so far	    */
static int strloopdecl;			/* data dummy do index declared?    */

#define sbuf_check() buf_check(sbufptr,maxsbufptr)
#define obuf_check() buf_check(obufptr,maxobufptr)
#define dbuf_check() buf_check(dbufptr,maxdbufptr)

int str_idnum = 0;		/* for generating unique string names */
int nbrace = 0;			/* must be zero when "end" is reached */
int nswitch = 0;		/* number switch stmts in procedure   */
int errchk = NO;		/* set if proc employs error checking */
static int ntasks = 0;		/* number of tasks in interpreter     */
static int errhand = NO;	/* set if proc employs error handler  */

static void do_type ( int );
static void traverse ( char );
static int parse_task_statement( void );
static int get_task ( char *, char *, size_t );
static int nextch( void );
static int get_name ( char *, size_t );
static void init_strings( void );
static void write_string_data_statement ( struct string * );
static void buf_check( char *, char * );
static char *str_fetch ( const char * );
static char *str_uniqid( void );
static void bob( void );

/* SKIPNL -- Skip to newline, e.g., when a comment is encountered.
 */
void skipnl( void )
{
	int c;
	while ((c=input()) != '\n')
	    ;
	unput ('\n');
}


/*
 * CONTEXT -- Package for setting, saving, and restoring the lexical context.
 * The action of the preprocessor in some cases depends upon the context, i.e.,
 * what type of statement we are processing, whether we are in global space,
 * within a procedure, etc.
 */

#define MAX_CONTEXT	5	/* max nesting of context	*/

static int cntxstk[MAX_CONTEXT];	/* for saving context		*/
static int cntxsp = 0;			/* save stack pointer		*/


/* SETCONTEXT -- Set the context.  Clears any saved context.
 */
static void setcontext ( int new_context )
{
	context = new_context;
	cntxsp = 0;
}


/* PUSHCONTEXT -- Push a temporary context.
 */
void pushcontext ( int new_context )
{
	if ( MAX_CONTEXT <= cntxsp ) {
	    error (XPP_COMPERR, "save context stack overflow");
	    cntxsp--;
	}
	cntxstk[cntxsp++] = context;
	context = new_context;
}


/* POPCONTEXT -- Pop the former context.  If the current context is PROCSTMT
 * (just finished compiling a procedure statement) then set the context to DECL
 * to indicate that we are entering the declarations section of a procedure.
 */
int popcontext( void )
{
	if (context & PROCSTMT) {
	    context = DECL;
	    if (cntxsp > 0)
		--cntxsp;
	} else if (cntxsp > 0)
	    context = cntxstk[--cntxsp];

	return (context);
}


/* Keyword table.  The simple hashing scheme requires that the keywords appear
 * in the table in sorted order.
 */
#define LEN_KWTBL	18

static struct {
	const char *keyw;	/* keyword name string			*/
	short opcode;		/* opcode from above definitions	*/
	short nelem;		/* number of table elements to skip if
				 * to get to next character class.
				 */
} kwtbl[] = {
	{ "FALSE",	XTY_FALSE,	0 },
	{ "TRUE",	XTY_TRUE,	0 },
	{ "bool",	XTY_BOOL,	0 },
	{ "char",	XTY_CHAR,	1 },
	{ "complex",	XTY_COMPLEX,	0 },
	{ "double",	XTY_DOUBLE,	0 },
	{ "error",	XTY_ERROR,	1 },
	{ "extern",	XTY_EXTERN,	0 },
	{ "false",	XTY_FALSE,	0 },
	{ "iferr",	XTY_IFERR,	2 },
	{ "ifnoerr",	XTY_IFNOERR,	1 },
	{ "int",	XTY_INT,	0 },
	{ "long",	XTY_LONG,	0 },
	{ "pointer",	XTY_POINTER,	1 },
	{ "procedure",	XTY_PROC,	0 },
	{ "real",	XTY_REAL,	0 },
	{ "short",	XTY_SHORT,	0 },
	{ "true",	XTY_TRUE,	0 }
};

/* short kwindex[30];		simple alphabetic hash index		*/
/* #define CINDEX(ch)		(isupper(ch)?ch-'A':ch-'a')		*/

#define MAXCH 128
static short kwindex[MAXCH+1];	/* simple alphabetic hash index		*/
#define CINDEX(ch)		(ch)


/* HASHTBL -- Hash the keyword table.  Initializes the "kwindex" hash table.
 * For each character in the alphabet, the index gives the index into the
 * sorted keyword table.  If there is no keyword name beginning with the index
 * character, the index entry is set to -1.
 */
static void hashtbl( void )
{
	int	i, j;

	for (i=j=0;  i <= MAXCH;  i++) {
	    if (i == CINDEX (kwtbl[j].keyw[0])) {
		kwindex[i] = j;
		j = min (LEN_KWTBL-1, j + kwtbl[j].nelem + 1);
	    } else
		kwindex[i] = -1;
	}
}


/* FINDKW -- Lookup an indentifier in the keyword table.  Return the opcode
 * of the keyword, or ERR if no match.
 */
static int findkw( void )
{
	char ch;
	const char *p, *q;
	int i, ilimit;

	if (kwindex[0] == 0)
	    hashtbl();

	i = CINDEX (yytext[0]);
	if (i < 0 || i >= MAXCH || (i = kwindex[i]) < 0)
	    return (ERR);
	ilimit = i + kwtbl[i].nelem;

	for (;  i <= ilimit;  i++) {
	    p = kwtbl[i].keyw + 1;
	    q = yytext + 1;

	    for (;  *p != EOS;  q++, p++) {
		ch = *q;
		/* 5DEC95 - Don't case convert keywords.
		if (isupper (ch))
		    ch = tolower (ch);
		 */
		if (*p != ch)
		    break;
	    }
	    if (*p == EOS && *q == EOS)
		return (kwtbl[i].opcode);
	}
	return (ERR);
}
		

/* MAPIDENT -- Lookup an identifier in the keyword table.  If the identifier is
 * not a keyword, output it as is.  If a datatype keyword, the action depends
 * on whether we are in a procedure body or not (i.e., whether the keyword
 * begins a declaration or is a type coercion function).  Most of the other
 * keywords are mapped into special x$.. identifiers for further processing
 * by the second pass.
 */
void mapident( void )
{
	int i;
	const char *ip;
	char *op, *maxop;

	/* If not keyword and not defined string, output as is.  The first
	 * char must be upper case for the name to be recognized as that of
	 * a defined string.  If we are processing a "define" macro expansion
	 * is disabled.
	 */
	if ((i = findkw()) == ERR) {
	    if (!isupper(yytext[0]) || (context & DEFSTMT) ||
		(ip = str_fetch (yytext)) == NULL) {

		outstr (yytext);
		return;

	    } else {
		yyleng = 0;
		maxop = yytext + BUFSIZ -1;
		for ( op=yytext ; op < maxop && *ip != EOS ; op++, ip++ ) {
		    *op = *ip;
		    yyleng++;
		}
		*op = EOS;
		do_string ('"', STR_DEFINE);
		return;
	    }
	}

	/* If datatype keyword, call do_type. */
	if (i <= XTY_POINTER) {
	    do_type (i);
	    return;
	}
	
	switch (i) {
	case XTY_TRUE:
	    outstr (".true.");
	    break;
	case XTY_FALSE:
	    outstr (".false.");
	    break;
	case XTY_IFERR:
	case XTY_IFNOERR:
	    outstr (yytext);
	    errhand = YES;
	    errchk = YES;
	    break;
	case XTY_ERROR:
	    outstr (yytext);
	    errchk = YES;
	    break;

	case XTY_EXTERN:
	    /* UNREACHABLE (due to decl.c additions).
	     */
	    outstr ("x$extn");
	    break;

	default:
	    error (XPP_COMPERR, "Keyword lookup error");
	}
}


static char st_buf[SZ_STBUF];
static char *st_next = st_buf;
static char *st_max = st_buf + SZ_STBUF -1;

static struct st_def {
	char	*st_name;
	char	*st_value;
} st_list[MAX_DEFSTR];

static int st_nstr = 0;

/* STR_ENTER -- Enter a defined string into the string table.  The string
 * table is a kludge to provide the capability to define strings in SPP.
 * The problem is that XPP handles strings but RPP handles macros, hence
 * strings cannot be defined.  We get around this by recognizing defines
 * of the form  'define NAME "..."'.  If a macro with a quoted value is
 * encounted we are called to enter the name and the string into the
 * table.  LOOKUP, above, subsequently searches the table for defined
 * strings.  The name must be upper case or the table will not be searched.
 *
 * N.B.: we are called by the lexical analyser with 'define name "' in
 * yytext.  The next input() will return the first char of the string.
 */
void str_enter( void )
{
	char *op, *maxop, ch;
	const char *ip;
	struct st_def *s;
	int n;
	char name[SZ_FNAME+1];

	/* Skip to the first char of the name string.
	 */
	ip = yytext;
	while (isspace (*ip))
	    ip++;
	while (!isspace (*ip))
	    ip++;
	while (isspace (*ip))
	    ip++;

	/* Do not accept statement unless the name is upper case.
	 */
	if (!isupper (*ip)) {
	    outstr (yytext);
	    return;
	}

	/* Extract macro name. */
	maxop = name + SZ_FNAME+1 -1;
	for ( op=name ; op < maxop && (isalnum(*ip) || *ip == '_') ; op++, ip++ )
	    *op = *ip;
	*op = EOS;

	/* Check for a redefinition. */
	for ( n=st_nstr, s=st_list, ch=name[0] ; n > 0 ; s++, n-- ) {
	    if (*(s->st_name) == ch)
		if (strcmp (s->st_name, name) == 0)
		    break;
	}

	/* Make a new entry?. */
	if (n <= 0) {
	    s = &st_list[st_nstr++];
	    if ( MAX_DEFSTR < st_nstr ) {
		error (XPP_COMPERR, "Too many defined strings");
		st_nstr--;
		return;
	    }

	    /* Put defined NAME in string buffer.  */
	    s->st_name = st_next;
	    for ( ip=name ; *ip != EOS ; st_next++, ip++ ) {
		if ( st_max < st_next ) goto err_quit;
		*st_next = *ip;
	    }
	    if ( st_max < st_next ) goto err_quit;
	    *st_next++ = EOS;
	}

	/* Put value in string buffer.
	 */
	s->st_value = st_next;
	traverse ('"');
	for ( ip=yytext ; *ip != EOS ; st_next++, ip++ ) {
	    if ( st_max < st_next ) goto err_quit;
	    *st_next = *ip;
	}
	if ( st_max < st_next ) goto err_quit;
	*st_next++ = EOS;

	return;

 err_quit:
	if (n <= 0) st_nstr--;
	error (XPP_COMPERR, "Too many defined strings");
	return;
}


/* STR_FETCH -- Search the defined string table for the named string
 * parameter and return a pointer to the string if found, NULL otherwise.
 */
static char *str_fetch ( const char *strname )
{
	struct st_def *s = st_list;
	int n = st_nstr;
	char ch = strname[0];

	for ( ; n > 0 ; n-- ) {
	    if (*(s->st_name) == ch)
		if (strcmp (s->st_name, strname) == 0)
		    return (s->st_value);
	    s++;
	}
	
	return (NULL);
}


/* SETLINE -- Set the file line number.  Used by the first pass to set
 * line number after processing an include file and in various other
 * places.  Necessary to get correct line numbers in error messages from
 * the second pass.
 */
void setline( void )
{
	char	msg[20];

	if (istkptr == 0) {			/* not in include file */
	    snprintf (msg, 20, "#!# %d\n", linenum[istkptr] - 1);
	    outstr (msg);
	}
}


/* OUTPUT -- Output a character.  If we are processing the body of a procedure
 * or a data statement, put the character into the output buffer.  Otherwise
 * put the character to the output file.
 *
 * NOTE -- the redirection logic shown below is duplicated in OUTSTR.
 */
void output ( char ch )
{
	if (context & (BODY|DATASTMT)) {
	    /* In body of procedure or in a data statement (which is output
	     * just preceding the body).
	     */
	    obuf_check();
	    *obufptr++ = ch;
	} else if (context & DECL) {
	    /* Output of a miscellaneous declaration in the declarations
	     * section.
	     */
	    dbuf_check();
	    *dbufptr++ = ch;
	} else {
	    /* Outside of a procedure.
	     */
	    putc (ch, yyout);
	}
}


/* DO_INCLUDE -- Process an include statement, i.e., eat up the include
 * statement, push the current input file on a stack, and open the new file.
 * System include files are referenced as "<file>", other files as "file".
 */
void do_include( void )
{
	char *p, *maxp, *op, *maxop;
	char hfile[SZ_FNAME+1];
	char delim, ch;
	int root_len;

	/* Push current input file status on the input file stack istk.
	 */
	istk[istkptr] = yyin;
	if (++istkptr >= MAX_INCLUDE) {
	    --istkptr;
	    error (XPP_COMPERR, "Maximum include nesting exceeded");
	    return;
	}

	/* If filespec "<file>", call os_sysfile to get the pathname of the
	 * system include file.
	 */
	if (yytext[yyleng-1] == '<') {
	    maxop = hfile + SZ_FNAME+1 -1;
	    for ( op=hfile ; (ch = input()) != EOF ; ) {
		if (ch == '\n') {
		    --istkptr;
		    error (XPP_SYNTAX, "missing > delim in include statement");
		    return;
		} else if (ch == '>')
		    break;
		if ( op < maxop ) {
		    *op = ch;
		    op++;
		}
	    }
	    *op = EOS;

	    if (os_sysfile (hfile, "sppincludes", fname[istkptr], SZ_PATHNAME) == ERR) {
		--istkptr;
		error (XPP_COMPERR, "cannot find include file");
		return;
	    }

	} else {
	    /* Prepend pathname leading to the file in which the current
	     * include statement was found.  Compiler may not have been run
	     * from the directory containing the source and include file.
	     */
	    if (!hbindefs) {
	        if ((p = rindex (fname[istkptr-1], '/')) == NULL)
		    root_len = 0;
	        else
		    root_len = p - fname[istkptr-1] + 1;
	        strncpy (fname[istkptr], fname[istkptr-1], root_len);

	    } else {
		const char *vp;
	        if ((vp = vfn2osfn (HBIN_INCLUDES, 0))) {
		    root_len = strlen (vp);
	            strncpy (fname[istkptr], vp, root_len);
	        } else {
		    --istkptr;
		    error (XPP_COMPERR, "cannot find hbin$ directory");
		    return;
	        }
	    }
	    fname[istkptr][root_len] = EOS;

	    delim = '"';

	    /* Advance to end of whatever is in the file name string.
	     */
	    maxp = fname[istkptr] + SZ_PATHNAME -1;
	    for (p=fname[istkptr];  *p != EOS;  p++)
		;
	    /* Concatenate name of referenced file.
	     */
	    while ((ch = input()) != delim) {
		if (ch == '\n' || ch == EOF) {
		    --istkptr;
		    error (XPP_SYNTAX, "bad include file name");
		    return;
		}
		if ( p < maxp ) {
		    *p = ch;
		    p++;
		}
	    }
	    *p = EOS;
	}

	/* If the foreign defs option is in effect, the machine dependent defs
	 * for a foreign machine are given by a substitute "iraf.h" file named
	 * on the command line.  This foreign machine header file includes
	 * not only the iraf.h for the foreign machine, but the equivalent of
	 * all the files named in the array of strings "machdefs".  Ignore any
	 * attempts to include any of these files since they have already been
	 * included in the foreign definitions header file.
	 */
	if (foreigndefs) {
	    char sysfile[SZ_PATHNAME];
	    const char **files;

	    for (files=machdefs;  *files != NULL;  files++) {
		snprintf (sysfile, SZ_PATHNAME, "%s%s", HOSTCONFIG, *files);
		if (strcmp (sysfile, fname[istkptr]) == 0) {
		    --istkptr;
		    return;
		}
	    }
	}

	if ((yyin = fopen (vfn2osfn(fname[istkptr],0), "r")) == NULL) {
	    yyin = istk[--istkptr];
	    error (XPP_SYNTAX, "Cannot open include file");
	    return;
	}

	/* Keep track of the line number within the include file.  */
	linenum[istkptr] = 1;

	/* Put the newline back so that LEX "^..." matches will work on
	 * first line of include file.
	 */
	unput ('\n');
}


/* YYWRAP -- Called by LEX when end of file is reached.  If input stack is
 * not empty, close off include file and continue on in old file.  Return 
 * nonzero when the stack is empty, i.e., when we reach the end of the
 * main file.
 */
int yywrap( void )
{
	/* The last line of a file is not necessarily newline terminated.
	 * Output a newline just in case.
	 */
	fprintf (yyout, "\n");

	if (istkptr <= 0) {
	    /* ALL DONE with main file.
	     */
	    return (1);

	} else {
	    /* End of include file.  Pop old input file and set line number
	     * for error messages.
	     */
	    fclose (yyin);
	    yyin = istk[--istkptr];
	    if (istkptr == 0)
		setline();
	    return (0);
	}
}


/* YY_INPUT -- Get a character from the input stream.
 */
int yy_input( void )
{
	return (input());
}


/* YY_UNPUT -- Put a character back into the input stream.
 */
void yy_unput( char ch )
{
	unput(ch);
}


/* Datatype keywords for declarations.  The special x$.. keywords are 
 * for communication with the second pass.  Note that this table is machine
 * dependent, since it maps char into type short.
 */
const char *type_decl[] = RPP_TYPES;


/* Intrinsic functions used for type coercion.  These mappings are machine
 * dependent (MACHDEP).  If your machine has INTEGER*2 and INTEGER*4, and
 * integer cannot be passed as an argument when a short or long is expected,
 * and your compiler has INT2 and INT4 type coercion intrinsic functions,
 * you should use those here instead of INT (which happens to work for a VAX).
 * If you cannot pass an int when a short is expected (i.e., IBM), and you
 * do not have an INT2 intrinsic function, you should provide an external
 * INTEGER*2 function called "int2" and use that for type coercion.  Note
 * that it will then be necessary to have the preprocessor automatically
 * generate a declaration for the function.  This nonsense will all go away
 * when we set up a proper table driven code generator!!
 */
static const char *intrinsic_function[] = {
	"",			/* table is one-indexed		*/
	"(0 != ",		/* bool(expr)			*/
	"int",			/* char(expr)			*/
	"int",			/* short(expr)			*/
	"int",			/* int(expr)			*/
	"int",			/* long(expr)			*/
	"real",			/* real(expr)			*/
	"dble",			/* double(expr)			*/
	"cmplx",		/* complex(expr)		*/
	"int"			/* pointer(expr)		*/
};


/* DO_TYPE -- Process a datatype keyword.  The type of processing depends
 * on whether we are called when processing a declaration or an expression.
 * In expressions, the datatype keyword is the type coercion intrinsic
 * function.  DEFINE statements are a special case; we treat them as
 * expressions, since macros containing datatype keywords are used in
 * expressions more than in declarations.  This is a kludge until the problem
 * is properly resolved by processing macros BEFORE code generation.
 * In the current implementation, macros are handled by the second pass (RPP).
 */
static void do_type ( int type )
{
	char    ch;

	if (context & (BODY|DEFSTMT)) {
	    switch (type) {
	    case XTY_BOOL:
		for (ch=input();  ch == ' ' || ch == '\t';  ch=input())
		    ;
		if (ch != '(')
		    error (XPP_SYNTAX, "Illegal boolean expr");
		outstr (intrinsic_function[type]);
		return;

	    case XTY_CHAR:
	    case XTY_SHORT:
	    case XTY_INT:
	    case XTY_LONG:
	    case XTY_REAL:
	    case XTY_DOUBLE:
	    case XTY_COMPLEX:
	    case XTY_POINTER:
		outstr (intrinsic_function[type]);
		return;

	    default:
		error (XPP_SYNTAX, "Illegal type coercion");
	    }

	} else {
	    /* UNREACHABLE when in declarations section of a procedure.
	     */
	    fprintf (yyout, type_decl[type]);
	}
}


#if 0
/* DO_CHAR -- Process a char array declaration.  Add "+1" to the first
 * dimension to allow space for the EOS.  Called after LEX has recognized
 * "char name[".  If we reach the closing ']', convert it into a right paren
 * for the second pass.
 */
void do_char( void )
{
	char	ch;

	for (ch=input();  ch != ',' && ch != ']';  ch=input())
	    if (ch == '\n' || ch == EOS) {
		error (XPP_SYNTAX, "Missing comma or ']' in char declaration");
		unput ('\n');
		return;
	    } else
		output (ch);

	outstr ("+1");
	if (ch == ']')
	    output (')');
	else
	    output (ch);
}
#endif


/* SKIP_HELPBLOCK -- Skip over a help block (documentation section).
 */
void skip_helpblock( void )
{
	while (fgets (yytext, BUFSIZ, yyin) != NULL) {
	    if (istkptr == 0)
		linenum[istkptr]++;

	    if (yytext[0] == '.' && (yytext[1] == 'e' || yytext[1] == 'E')) {
		yytext[8] = EOS;
		if (strcmp (&yytext[1], "endhelp") == 0 ||
		    strcmp (&yytext[1], "ENDHELP") == 0)
			break;
	    }
	}
}


/* PROCESS_TASK_STATEMENT -- Parse the TASK statement.  The task statement
 * is replaced by the "sys_runtask" procedure (sysruk), which is called by
 * the IRAF main to run a task, or to print the dictionary (cmd "?").
 * The source for the basic sys_runtask procedure is in "base$sysruk.x".
 * We process the task statement into some internal tables, then open the
 * sysruk.x file as an include file.  Special macros therein are
 * replaced by the taskname dictionary as processing continues.
 */
void process_task_statement( void )
{
	char	ch;

	if (ntasks > 0) {		/* only one task statement permitted */
	    error (XPP_SYNTAX, "Only one TASK statement permitted per file");
	    return;
	}

	/* Process the task statement into the TASK_LIST structure.
	 */
	if (parse_task_statement() == ERR) {
	    error (XPP_SYNTAX, "Syntax error in TASK statement");
	    while ((ch = input()) != EOF && ch != '\n')
		;
	    unput ('\n');
	    return;
	}

	/* Open RUNTASK ("base$sysruk.x") as an include file.
	 */
	istk[istkptr] = yyin;
	if (++istkptr >= MAX_INCLUDE) {
	    istkptr--;
	    error (XPP_COMPERR, "Maximum include nesting exceeded");
	    return;
	}

	snprintf (fname[istkptr], SZ_PATHNAME, "%s%s", IRAFBASE, RUNTASK);
	if ((yyin = fopen (vfn2osfn (fname[istkptr],0), "r")) == NULL) {
	    yyin = istk[--istkptr];
	    error (XPP_SYNTAX, "Cannot read base$sysruk.x");
	    return;
	}

	linenum[istkptr] = 1;

	/* Put the newline back so that LEX "^..." matches will work on
	 * first line of the include file.
	 */
	unput ('\n');
}

	
/* PARSE_TASK_STATEMENT -- Parse the task statement, building up a list
 *   of task_name/procedure_name structures in the "task_list" array.
 *
 *	task	task1, task2, task3=proc3, task4, ...
 *
 * Task names are placed in the string buffer as one big string, with EOS
 *   delimiters between the names.  This "dictionary" string is converted
 *   into a data statement at "end_code" time, along with any other strings
 *   in the runtask procedure.  The procedure names, which may differ from
 *   the task names, are saved in the upper half of the output buffer.  We can
 *   do this because we know that the runtask procedure is small and will not
 *   come close to filling up the output buffer, which buffers only the body
 *   of the procedure currently being processed.
 * N.B.: Upon entry, the input is left positioned to just past the "task"
 *   keyword.
 */
static int parse_task_statement( void )
{
	struct task *tp;
	const char *ip;
	char ch;
	char task_name[SZ_FNAME], proc_name[SZ_FNAME];
	int name_offset;

	/* Set global pointers to where we put task and proc name strings.
	 */
	sbufptr = sbuf;
	obufptr = &obuf[SZ_OBUF/2];
	name_offset = 1;

	for (ntasks=0;  ntasks < MAX_TASKS;  ntasks++) {
	    /* Process "taskname" or "taskname=procname".  There must be
	     * at least one task name in the declaration.
	     */
	    if (get_task (task_name, proc_name, SZ_FNAME) == ERR)
		return (ERR);
	    
	    /* Set up the task declaration structure, and copy name strings
	     * into the string buffers.
	     */
	    tp = &task_list[ntasks];
	    tp->task_name = sbufptr;
	    tp->proc_name = obufptr;
	    tp->name_offset = name_offset;
	    name_offset += strlen (task_name) + 1;
	    
	    sbuf_check();
	    for (ip=task_name;  (*sbufptr++ = *ip++) != EOS;  )
		if ( maxsbufptr < sbufptr ) goto err;
	    obuf_check();
	    for (ip=proc_name;  (*obufptr++ = *ip++) != EOS;  )
		if ( maxobufptr < obufptr ) goto err;

	    /* If the next character is a comma, skip it and a newline if
	     * one follows and continue processing.  If the next character is
	     * a newline, we are done.  Any other character is an error.
	     * Note that nextch skips whitespace and comments.
	     */
	    ch = nextch();
	    if (ch == ',') {
		if ((ch = nextch()) != '\n')
		    unput (ch);
	    } else if (ch == '\n') {
		linenum[istkptr]++;
		ntasks++;			/* end of task statement */	
		break;
	    } else
		return (ERR);
	}

	if (ntasks >= MAX_TASKS) {
err:	    error (XPP_COMPERR, "too many tasks in task statement");
	    return (ERR);
	}

	/* Set up the task name dictionary string so that it gets output
	 * as a data statement when the runtask procedure is output.
	 */
	string_list[0].str_name = "dict";
	string_list[0].str_text = sbuf;
	string_list[0].str_length = (sbufptr - sbuf);
	nstrings = 1;

	/* Leave the output buffer pointer pointing to the first half of
	 * the buffer.
	 */
	obufptr = obuf;
	return (OK);
}


/* GET_TASK -- Process a single task declaration of the form "taskname" or
 * "taskname = procname".
 */
static int get_task ( char *task_name, char *proc_name, size_t bufsize )
{
	char ch;

	/* Get task name.
	 */
	if (get_name (task_name, bufsize) == ERR)
	    return (ERR);
	
	/* Get proc name if given, otherwise the procedure name is assumed
	 * to be the same as the task name.
	 */
	if ((ch = nextch()) == '=') {
	    if (get_name (proc_name, bufsize) == ERR)
		return (ERR);
	} else {
	    unput (ch);
	    snprintf (proc_name, bufsize, "%s", task_name);
	}

	return (XOK);
}


/* GET_NAME -- Extract identifier from input, placing in the output string.
 * ERR is returned if the output string overflows, or if the token is not
 * a legal identifier.
 */
static int get_name ( char *outstr, size_t bufsize )
{
	char ch, *op, *maxop;

	unput ((ch = nextch()));	/* skip leading whitespace	*/

	maxop = outstr + bufsize -1;
	op=outstr;
	for ( ; ; ) {
	    ch = input();
	    if (isalpha(ch)) {
		if (isupper(ch)) {
		    if ( op < maxop ) *op++ = tolower(ch);
		}
		else {
		    if ( op < maxop ) *op++ = ch;
		}
	    } else if ((isdigit(ch) && outstr < op) || ch == '_' || ch == '$') {
		if ( op < maxop ) *op++ = ch;
	    } else {
		if ( op <= maxop ) *op = EOS;
		unput (ch);
		return (outstr < op ? XOK : ERR);
	    }
	}

	return (ERR);
}


/* NEXTCH -- Get next nonwhite character from the input stream.  Ignore
 * comments.  Newline is not considered whitespace.
 */
static int nextch( void )
{
	char ch;

	while ((ch = input()) != EOF) {
	    if (ch == '#') {			/* discard comment */
		while ((ch = input()) != '\n')
		    ;
		return (ch);
	    } else if (ch != ' ' && ch != '\t')
		return (ch);
	}
	return (EOF);
}


/* PUT_DICTIONARY -- We are called when the keyword TN$DECL is encountered,
 * i.e., while processing "sysruk.x".  This should only happen after the
 * task statement has been successfully processed.  Our function is to replace
 * the TN$DECL macro by the declarations for the DP and DICT structures.
 * DP is an integer array giving the offsets of the task name strings in DICT,
 * the dictionary string buffer.
 */
#define NDP_PERLINE		8	/* num DP data elements per line */

void put_dictionary( void )
{
	struct task *tp;
	char buf[SZ_LINE];
	int i, j, offset;

	/* Discard anything found on line after the TN$DECL, which is only
	 * recognized as the first token on the line.
	 */
	while (input() != '\n')
	    ;
	unput ('\n');

	/* Output the data statements required to initialize the DP array.
	 * These statements are spooled into the output buffer and not output
	 * until all declarations have been processed, since the Fortran std
	 * requires that data statements follow declarations.
	 */
	pushcontext (DATASTMT);
	tp = task_list;
	
	for (j=0;  j <= ntasks;  j += NDP_PERLINE) {
	    if (!strloopdecl++) {
		pushcontext (DECL);
		snprintf (buf, SZ_LINE, "%s\tiyy\n", type_decl[TY_INT]);
		outstr (buf);
		popcontext();
	    }

	    snprintf (buf, SZ_LINE, "data\t(dp(iyy),iyy=%2d,%2d)\t/",
		j+1, min (j+NDP_PERLINE, ntasks+1));
	    outstr (buf);

	    for (i=j;  i < j+NDP_PERLINE && i <= ntasks;  i++) {
		offset = (tp++)->name_offset;
		if (i >= ntasks)
		    snprintf (buf, SZ_LINE, "%2d/\n", XEOS);
		else if (i == j + NDP_PERLINE - 1)
		    snprintf (buf, SZ_LINE, "%4d/\n", offset==EOS ? XEOS: offset);
		else
		    snprintf (buf, SZ_LINE, "%4d,", offset==EOS ? XEOS: offset);
		outstr (buf);
	    }
	}

	popcontext();

	/* Output type declarations for the DP and DICT arrays.  The string
	 * descriptor for string 0 (dict) was prepared when the TASK statement
	 * was processed.
	 */
	snprintf (buf, SZ_LINE, "%s\tdp(%d)\n", type_decl[XTY_INT], ntasks + 1);
	outstr (buf);
	snprintf (buf, SZ_LINE, "%s\tdict(%d)\n", type_decl[XTY_CHAR],
	    string_list[0].str_length);
	outstr (buf);
}


/* PUT_INTERPRETER -- Output the statements necessary to scan the dictionary
 * for a task and call the associated procedure.  We are called when the
 * keyword TN$INTERP is encountered in the input stream.
 */
void put_interpreter( void )
{
	char	lbuf[SZ_LINE];
	int	i;

	while (input() != '\n')		/* discard rest of line */
	    ;
	unput ('\n');

	for (i=0;  i < ntasks;  i++) {
	    snprintf (lbuf, SZ_LINE, "\tif (streq (task, dict(dp(%d)))) {\n", i+1);
		outstr (lbuf);
	    snprintf (lbuf, SZ_LINE, "\t    call %s\n", task_list[i].proc_name);
		outstr (lbuf);
	    snprintf (lbuf, SZ_LINE, "\t    return (OK)\n");
		outstr (lbuf);
	    snprintf (lbuf, SZ_LINE, "\t}\n");
		outstr (lbuf);
	}
}


/* OUTSTR -- Output a string.  Depending on the context, the string will
 * either go direct to the output file, or will be buffered in the output
 * buffer.
 */
void outstr ( const char *string )
{
	const char *ip;

	if (context & (BODY|DATASTMT)) {
	    /* In body of procedure or in a data statement (which is output
	     * just preceding the body).
	     */
	    obuf_check();
	    for (ip=string;  (*obufptr++ = *ip++) != EOS;  )
		obuf_check();
	    --obufptr;
	} else if (context & DECL) {
	    /* Output of a miscellaneous declaration in the declarations
	     * section.
	     */
	    if (string[0] == '}') bob();
	    dbuf_check();
	    for (ip=string;  (*dbufptr++ = *ip++) != EOS;  )
		dbuf_check();
	    --dbufptr;
	} else {
	    /* Outside of a procedure.
	     */
	    fputs (string, yyout);
	}
}

static void bob( void )
{
	int i = 0;
	i++;
}


/* BEGIN_CODE -- Code that gets executed when the keyword BEGIN is encountered,
 * i.e., when we begin processing the executable part of a procedure
 * declaration.
 */
void begin_code( void )
{
	char	text[1024];

	/* If we are already processing the body of a procedure, we probably
	 * have a missing END.
	 */
	if (context & BODY)
	    xpp_warn ("Unmatched BEGIN statement");

	/* Set context flag noting that we are processing the body of a 
	 * procedure.  Output the BEGIN statement, for the benefit of the
	 * second pass (RPP), which needs to know where the procedure body
	 * begins.
	 */
	setcontext (BODY);
	d_runtime (text,1024);  outstr (text);
	outstr ("begin\n");

	/* Initialization. */
	nbrace = 0;
	nswitch = 0;
	str_idnum = 1;
	errhand = NO;
	errchk = NO;
}


/* END_CODE -- Code that gets executed when the keyword END is encountered
 * in the input.  If error checking is used in the procedure, we must declare
 * the boolean function XERPOP.  If any switches are employed, we must declare
 * the switch variables.  Next we format and output data statements for any
 * strings encountered while processing the procedure body.  If the procedure
 * being processed is sys_runtask, the task name dictionary string is also
 * output.  Finally, we output the spooled procedure body, followed by and END
 * statement for the benefit of the second pass.
 */
void end_code( void )
{
	int	i;

	/* If the END keyword is encountered outside of the body of a
	 * procedure, we leave it alone.
	 */
	if (!(context & BODY)) {
	    outstr (yytext);
	    return;
	}

	/* Output argument and local variable declarations (see decl.c).
	 * Note d_enter may have been called during processing of the body
	 * of a procedure to make entries in the symbol table for intrinsic
	 * functions, switch variables, etc. (this is not currently done).
	 */
	d_codegen (yyout);

	setcontext (GLOBAL);

	/* Output declarations for error checking and switches.  All variables
	 * and functions must be declared.
	 */
	if (errhand)
	    fprintf (yyout, "x$bool xerpop\n");
	if (errchk)
	    fprintf (yyout, "errchk error, erract\n");
	errhand = NO;
	errchk = NO;

	if (nswitch) {			/* declare switch variables */
	    fprintf (yyout, "%s\t", type_decl[XTY_INT]);
	    for (i=1;  i < nswitch;  i++)
		fprintf (yyout, "SW%04d,", i);
	    fprintf (yyout, "SW%04d\n", i);
	}

	/* Output any miscellaneous declarations.  These include ERRCHK and
	 * COMMON declarations - anything not a std type declaration or a
	 * data statement declaration.
	 */
	dbuf_check();
	*dbufptr++ = EOS;
	fputs (dbuf, yyout); fflush (yyout);

	{ int i; for (i=0; i < SZ_DBUF; ) dbuf[i++] = '\0'; }

	dbufptr = dbuf;

	/* Output the SAVE statement, which must come after all declarations
	 * and before any DATA statements.
	 */
	fputs ("save\n", yyout);

	/* Output data statements to initialize character strings, followed
	 * by any runtime procedure entry initialization statments, followed
	 * by the spooled text in the output buffer, followed by the END.
	 * Clear the string and output buffers.  Any user data statements
	 * will already have been moved into the output buffer, and they
	 * will come out at the end of the declarations section regardless
	 * of where they were given in the declarations section.  Data stmts
	 * are not permitted in the procedure body.
	 */
	init_strings();
	obuf_check();
	*obufptr++ = EOS;
	fputs (obuf, yyout); fflush (yyout);

	{ int i; for (i=0; i < SZ_OBUF; ) obuf[i++] = '\0'; }

	fputs ("end\n", yyout); fflush (yyout);

	obufptr = obuf;
	*obufptr = EOS;
	sbufptr = sbuf;

	if (nbrace != 0) {
	    error (XPP_SYNTAX, "Unmatched brace");
	    nbrace = 0;
	}
}


#define BIG_STRING	9
#define NPERLINE	8

/* INIT_STRINGS -- Output data statements to initialize all strings in a
 * procedure ("string" declarations, inline strings, and the runtask
 * dictionary).  Strings are implemented as integer arrays, using the
 * smallest integer datatype provided by the host Fortran compiler, usually
 * INTEGER*2 (XTY_CHAR).
 */
static void init_strings( void )
{
	int str;

	if (nstrings)
	    for (str=0;  str < nstrings && !strloopdecl;  str++)
		if (string_list[str].str_length >= BIG_STRING) {
		    fprintf (yyout, "%s\tiyy\n", type_decl[XTY_INT]);
		    strloopdecl++;
		}

	for (str=0;  str < nstrings;  str++)
	    write_string_data_statement (&string_list[str]);

	sbufptr = sbuf;			/* clear string buffer		*/
	nstrings = 0;
	strloopdecl = 0;
}


/* WRITE_STRING_DATA_STATEMENT -- Output data statement to initialize a single
 * string.  If short string, output a simple whole-array data statement
 * that fits all on one line.  Large strings are initialized with multiple
 * data statements, each of which initializes a section of the string
 * using a dummy subscript.  This is thought to be more portable than
 * a single large data statement with continuation, because the number of
 * continuation cards permitted in a data statement depends on the compiler.
 * The loop variable in an implied do loop in a data statement must be declared
 * on some compilers (crazy but true).  Determine if we will be generating any
 * implied dos and declare the variable if so.
 */
static void write_string_data_statement ( struct string *s )
{
	const char *ip, *name;
	int i, j, len;
	char ch;

	name = s->str_name;
	ip = s->str_text;
	len = s->str_length;

	if (len < BIG_STRING) {
	    fprintf (yyout, "data\t%s\t/", name);
	    for (i=0;  i < len-1;  i++) {
		if ((ch = *ip++) == EOS)
		    fprintf (yyout, "%3d,", XEOS);
		else
		    fprintf (yyout, "%3d,", ch);
	    }
	    fprintf (yyout, "%2d/\n", XEOS);

	} else {
	    for (j = 0;  j < len;  j += NPERLINE) {
		fprintf (yyout, "data\t(%s(iyy),iyy=%2d,%2d)\t/",
		    name, j+1, min(j+NPERLINE, len));
		for (i=j;  i < j+NPERLINE;  i++) {
		    if (i >= len-1) {
			fprintf (yyout, "%2d/\n", XEOS);
			return;
		    } else if (i == j+NPERLINE-1) {
			fprintf (yyout, "%3d/\n", ip[i]==EOS ? XEOS: ip[i]);
		    } else
			fprintf (yyout, "%3d,", ip[i]==EOS ? XEOS: ip[i]);
		}
	    }
	}
}


/* DO_STRING -- Process a STRING declaration or inline string.  Add a new
 * string descriptor to the string list, copy text of string into sbuf,
 * save name of string array in sbuf.  If inline string, manufacture the
 * name of the string array.
 */
/* delim   : char which delimits string	*/
/* strtype : string type		*/
void do_string ( char delim, int strtype )
{
	char ch;
	const char *ip;
	struct string *s;
	int readstr = 1;

	/* If we run out of space for string storage, print error message,
	 * dump string decls out early, clear buffer and continue processing.
	 */
	if (nstrings >= MAX_STRINGS) {
	    error (XPP_COMPERR, "Too many strings in procedure");
	    init_strings();
	}

	s = &string_list[nstrings];

	switch (strtype) {

	case STR_INLINE:
	case STR_DEFINE:
	    /* Inline strings are implemented as Fortran arrays; generate a
	     * dummy name for the array and set up the descriptor.
	     * Defined strings are inline strings, but the name of the text of
	     * the string is already in yytext when we are called.
	     */
	    s->str_name = sbufptr;
	    sbuf_check();
	    for (ip = str_uniqid();  (*sbufptr++ = *ip++) != EOS;  ) {
		sbuf_check();
	    }
	    break;

	case STR_DECL:
	    /* String declaration.  Read in name of string, used as name of
	     * Fortran array.
	     */
	    ch = nextch();			/* skip whitespace	*/
	    if (!isalpha (ch))
		goto sterr;
	    s->str_name = sbufptr;
	    sbuf_check();
	    *sbufptr++ = ch;

	    /* Get rest of string name identifier. */
	    while ((ch = input()) != EOF) {
		if (isalnum(ch) || ch == '_') {
		    sbuf_check();
		    *sbufptr++ = ch;
		} else if (ch == '\n') {
sterr:		    error (XPP_SYNTAX, "String declaration syntax");
		    while (input() != '\n')
			;
		    unput ('\n');
		    return;
		} else {
		    sbuf_check();
		    *sbufptr++ = EOS;
		    break;
		}
	    }

	    /* Advance to the ' or " string delimiter, in preparation for
	     * processing the string itself.  If syntax error occurs, skip
	     * to newline to avoid spurious error messages.  If the string
	     * is not quoted the string value field is taken to be the name
	     * of a string DEFINE.
	     */
	    delim = nextch();

	    if (!(delim == '"' || delim == '\'')) {
		char *op, *maxop;
		const char *ip;
		int ch;

		/* Fetch name of defined macro into yytext.
		 */
		op = yytext;
		*op++ = delim;
		while ((ch = input()) != EOF)
		    if (isalnum(ch) || ch == '_')
			*op++ = ch;
		    else
			break;
		unput (ch);
		*op = EOS;

		/* Fetch body of string into yytext.
		 */
		if ((ip = str_fetch (yytext)) != NULL) {
		    yyleng = 0;
		    maxop = yytext + BUFSIZ -1;
		    for ( op=yytext ; op < maxop && *ip != EOS ; op++, ip++ ) {
			*op = *ip;
			yyleng++;
		    }
		    *op = EOS;
		    readstr = 0;
		} else {
		    error (XPP_SYNTAX,
			"Undefined macro referenced in string declaration");
		}
	    }

	    break;
	}

	/* Get the text of the string.  Process escape sequences.  String may
	 * not span multiple lines.  In the case of a defined string, the text
	 * of the string will already be in yytext.
	 */
	s->str_text = sbufptr;
	if (readstr && strtype != STR_DEFINE)
	    traverse (delim);		    /* process string into yytext */
	sbuf_check();
	snprintf (sbufptr, SZ_SBUF - (sbufptr-sbuf), "%s", yytext);
	if ( strlen(sbufptr) < strlen(yytext) ) {
	    error (XPP_COMPERR, "String buffer overflow");
	    _exit (1);
	}
	sbufptr += yyleng + 1;
	s->str_length = yyleng + 1;
	
	/* Output array declaration for string.  We want the declaration to
	 * go into the miscellaneous declarations buffer, so toggle the
	 * the context to DECL before calling OUTSTR.
	 */
	{
	    char lbuf[SZ_LINE];

	    pushcontext (DECL);
	    snprintf (lbuf, SZ_LINE, "%s\t%s(%d)\n", type_decl[XTY_CHAR], s->str_name,
		s->str_length);
	    outstr (lbuf);
	    popcontext();
	}
	
	/* If inline string, replace the quoted string by the name of the
	 * string variable.  This text goes into the output buffer, rather
	 * than directly to the output file as is the case with the declaration
	 * above.
	 */
	if (strtype == STR_INLINE || strtype == STR_DEFINE)
	    outstr (s->str_name);

	if (++nstrings >= MAX_STRINGS)
	    error (XPP_COMPERR, "Too many strings in procedure");
}


/* DO_HOLLERITH -- Process and output a Fortran string.  If the output
 * compiler is Fortran 77, we output a quoted string; otherwise we output
 * a hollerith string.  Fortran (packed) strings appear in the SPP source
 * as in the statement 'call_f77_sub (arg, *"any string", arg)'.  Escape
 * sequences are not recognized.
 */
void do_hollerith( void )
{
	char *op, *maxop;
	char strbuf[SZ_LINE], outbuf[SZ_LINE];
	char ch;
	int len;

	/* Read the string into strbuf. */
	maxop = strbuf + SZ_LINE -1;
	for ( op=strbuf, len=0 ; (ch = input()) != '"' ; len++ ) {
	    if (ch == '\n' || ch == EOF) break;
	    if ( op < maxop ) {
		*op = ch;
		op++;
	    }
	}
	*op = EOS;				/* delete delimiter */
	if (ch == '\n')
	    error (XPP_COMPERR, "Packed string not delimited");

#ifdef F77
	snprintf (outbuf, SZ_LINE, "\'%s\'", strbuf);
#else
	snprintf (outbuf, SZ_LINE, "%dH%s", i, strbuf);
#endif

	outstr (outbuf);
}


/* BUF_CHECK -- Check to see that the string buffer has not overflowed.
 * It is a fatal error if it does.
 */
static void buf_check( char *bufptr, char *maxbufptr )
{
	if ( maxbufptr < bufptr ) {
	    error (XPP_COMPERR, "String buffer overflow");
	    _exit (1);
	}
}


/* STR_UNIQID -- Generate a unit identifier name for an inline string.
 */
static char *str_uniqid( void )
{
	static  char id[] = "ST0000";

	snprintf (&id[2], 5, "%04d", str_idnum++);
	return (id);
}


/* TRAVERSE -- Called by the lexical analyzer when a quoted string has
 * been recognized.  Characters are input and deposited in yytext (the
 * lexical analyzer token buffer) until the trailing quote is seen.
 * Strings may not span lines unless the newline is delimited.  The
 * recognized escape sequences are converted upon input; all others are
 * left alone, presumably to later be converted by other code.
 * Quotes may be included in the string by escaping them, or by means of
 * the double quote convention.
 */
static void traverse ( char delim )
{
	char *op, *maxop;
	char ch;

	maxop = yytext + BUFSIZ -1;
	for ( op=yytext ; (*op = input()) != EOF ; ) {
	    if (*op == delim) {
		if ((*op = input()) == EOF)
		    break;
		if (*op == delim)
		    goto next;		/* double quote convention; keep one */
		else {
		    unput (*op);
		    break;			/* normal exit		*/
		}

	    } else if (*op == '\n') {		/* error recovery exit	*/
		unput ('\n');
		xpp_warn ("Newline while processing string");
		break;

	    } else if (*op == '\\') {
		const char *cp;
		if ((*op = input()) == EOF) {
		    break;
		} else if (*op == '\n') {
		    --op;			/* explicit continuation */
		    goto next;
		} else if ((cp = index (esc_ch, *op)) != NULL) {
		    *op = esc_val[cp-esc_ch];
		} else if (isdigit (*op)) {	/* '\0DD' octal constant */
		    *op -= '0';
		    while (isdigit (ch = input()))
			*op = (*op * 8) + (ch - '0');
		    unput (ch);
		} else {
		    ch = *op;			/* unknown escape sequence, */
		    *op++ = '\\';		/* leave it alone.	    */
		    *op = ch;
		}
	    }
	next:
	    if ( op < maxop ) op++;
	}

	*op = EOS;
	yyleng = (op - yytext);
}


/* ERROR -- Output an error message and set exit flag so that no linking occurs.
 * Do not abort compiler, however, because it is better to keep going and
 * find all the errors in a single compilation.
 */
void error ( int errcode, const char *errmsg )
{
	fprintf (stderr, "Error on line %d of %s: %s\n", linenum[istkptr],
	    fname[istkptr], errmsg);
	fflush (stderr);
	errflag |= errcode;
}


/* WARN -- Output a warning message.  Do not set exit flag since this is only
 * a warning message; linking should occur if there are not any more serious
 * errors.
 */
void xpp_warn ( const char *warnmsg )
{
	fprintf (stderr, "Warning on line %d of %s: %s\n", linenum[istkptr],
	    fname[istkptr], warnmsg);
	fflush (stderr);
}


/* ACCUM -- Code for conversion of numeric constants to decimal.  Convert a
 * character string to a binary integer constant, doing the conversion in the
 * indicated base.
 */
static long accum ( int base, char **strp )
{
	char *ip;
	char digit;
	long sum;

	sum = 0;
	ip = *strp;

	switch (base) {
	case OCTAL:
	case DECIMAL:
	    for (digit = *ip++;  isdigit (digit);  digit = *ip++)
		sum = sum * base + (digit - '0');
	    *strp = ip - 1;
	    break;
	case HEX:
	    while ((digit = *ip++) != EOF) {
		if (isdigit (digit))
		   sum = sum * base + (digit - '0');
		else if (digit >= 'a' && digit <= 'f')
		   sum = sum * base + (digit - 'a' + 10);
		else if (digit >= 'A' && digit <= 'F')
		   sum = sum * base + (digit - 'A' + 10);
		else {
		    *strp = ip;
		    break;
		}
	    }
	    break;
	default:
	    error (XPP_COMPERR, "Accum: unknown numeric base");
	    return (ERR);
	}

	return (sum);
}


/* CHARCON -- Convert a character constant to a binary integer value.
 * The regular escape sequences are recognized; numeric values are assumed
 * to be octal.
 */
static int charcon ( char *string )
{
	char ch;
	char *ip, *nump;

	ip = string + 1;		/* skip leading apostrophe	*/
	ch = *ip++;

	/* Handle '\c' and '\0dd' notations.
	 */
	if (ch == '\\') {
	    const char *cc;
	    if ((cc = index (esc_ch, *ip)) != NULL) {
		return (esc_val[cc-esc_ch]);
	    } else if (isdigit (*ip)) {
		nump = ip;
		return (accum (OCTAL, &nump));
	    } else
		return (ch);
	} else {
	    /* Regular characters, i.e., 'c'; just return ASCII value of char.
	     */
	    return (ch);
	}
}


/* INT_CONSTANT -- Called to decode an integer constant, i.e., a decimal, hex,
 * octal, or sexagesimal number, or a character constant.  The numeric string
 * is converted in the indicated base and replaced by its decimal value.
 */
/* string : yytext */
void int_constant ( char *string, size_t bufsize, int base )
{
	char decimal_constant[SZ_NUMBUF];
	char *p, *maxp;
	char ch;
	long value;
	int i;

	p = string;
	maxp = string + bufsize -1;
	i = strlen (string);

	switch (base) {
	case DECIMAL:
	    value = accum (10, &p);
	    break;
	case SEXAG:
	    value = accum (10, &p);
	    break;
	case OCTAL:
	    value = accum (8, &p);
	    break;
	case HEX:
	    value = accum (16, &p);
	    break;

	case CHARCON:
	    while ((ch = input()) != EOF) {
		if (ch == '\n') {
		    error (XPP_SYNTAX, "Undelimited character constant");
		    if ( p+i <= maxp ) p[i] = EOS;
		    return;
		} else if (ch == '\\') {
		    if ( p+i < maxp ) p[i++] = ch;
		    ch = input();
		    if ( p+i < maxp ) p[i++] = ch;
		    continue;
		} else if (ch == '\'') {
		    if ( p+i < maxp ) p[i++] = ch;
		    break;
		}
		if ( p+i < maxp ) p[i++] = ch;
	    }
	    if ( p+i <= maxp ) p[i] = EOS;
	    value = charcon (p);
	    break;

	default:
	    error (XPP_COMPERR, "Unknown numeric base for integer conversion");
	    value = ERR;
	}

	/* Output the decimal value of the integer constant.  We are simply
	 * replacing the SPP constant by a decimal constant.
	 */
	snprintf (decimal_constant, SZ_NUMBUF, "%ld", value);
	outstr (decimal_constant);
}


/* HMS -- Convert number in HMS format into a decimal constant, and output
 * in that form.  Successive : separated fields are scaled to 1/60 th of
 * the preceeding field.  Thus "12:30" is equivalent to "12.5".  Some care
 * is taken to preserve the precision of the number.
 */
void hms ( char *number )
{
	char cvalue[SZ_NUMBUF];
	char *ip;
	int bvalue, ndigits;
	long scale = 10000000;
	long units = 1;
	long value = 0;

	for (ndigits=0, ip=number;  *ip;  ip++)
	    if (isdigit (*ip))
		ndigits++;

	/* Get the unscaled base value part of the number. */
	ip = number;
	bvalue = accum (DECIMAL, &ip);

	/* Convert any sexagesimal encoded fields.  */
	while (*ip == ':') {
	    ip++;
	    units *= 60;
	    value += (accum (DECIMAL, &ip) * scale / units);
	}

	/* Convert the fractional part of the number, if any.
	 */
	if (*ip++ == '.')
	    while (isdigit (*ip)) {
		units *= 10;
		value += (*ip++ - '0') * scale / units;
	    }

	/* Format the output number. */
	if (ndigits > MIN_REALPREC)
	    snprintf (cvalue, SZ_NUMBUF, "%d.%ldD0", bvalue, value);
	else
	    snprintf (cvalue, SZ_NUMBUF, "%d.%ld", bvalue, value);
	cvalue[ndigits+1] = '\0';

	/* Print the translated number. */
	outstr (cvalue);
}


/*
 *  Revision history (when i remembered) --
 *
 * 14-Dec-82:	Changed hms conversion, to produce degrees or hours,
 *		rather than seconds (lex pattern, add hms, delete ':'
 *		action from accum).
 *
 * 10-Mar-83	Broke C code and Lex code into separate files.
 *		Added support for error handling.
 *		Added additional type coercion functions.
 *
 * 20-Mar-83	Modified processing of TASK stmt to use file inclusion
 *		to read the RUNTASK file, making it possible to maintain
 *		the IRAF main as a .x file, rather than as a .r file.
 *
 * Dec-83	Fixed bug in processing of TASK stmt which prevented
 *		compilation of processes with many tasks.  Added many
 *		comments and cleaned up the code a bit.
 */