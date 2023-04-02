/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)dohist.c	1.12 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)dohist.c 1.12 20/09/06 J. Schilling"
#endif
/*
 * @(#)dohist.c 1.7 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)dohist.c"
#pragma ident	"@(#)sccs:lib/comobj/dohist.c"
#endif
# include	<defines.h>
#	define	VMS_VFORK_OK
# include	<schily/vfork.h>
# include	<schily/wait.h>
# include	<schily/fcntl.h>
# include	<had.h>
# include       <i18n.h>

extern char *Mrs;
extern int Domrs;

static char	Mstr[RESPSIZE];
static char *getresp	__PR((char *repstr, char *result));
static char *getcomments __PR((void));


void
dohist(file)
char *file;
{
	char line[VBUF_SIZE];
	int doprmt;
	register char *p;
	FILE	*in;
	extern char *Comments;

	/*
	 * Quick and dirty way to find whether VALFLAG is set.
	 */
	in = xfopen(file, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
	setvbuf(in, NULL, _IOFBF, VBUF_SIZE);
#endif
	while ((p = fgets(line,sizeof(line),in)) != NULL)
		if (line[0] == CTLCHAR && line[1] == EUSERNAM)
			break;
	if (p != NULL) {
		while ((p = fgets(line,sizeof(line),in)) != NULL)
			if (line[3] == VALFLAG && line[1] == FLAG && line[0] == CTLCHAR)
				break;
			else if (line[1] == BUSERTXT && line[0] == CTLCHAR)
				break;
		if (p != NULL && line[1] == FLAG) {
			Domrs++;
		}
	}
	(void) fclose(in);

	doprmt = 0;
	if (isatty(0) == 1)
		doprmt++;
	if (Domrs && !Mrs) {
		if (doprmt)
			printf(NOGETTEXT("MRs? "));
		Mrs = getresp(" ",Mstr);
	}
	if (Domrs)
		mrfixup();
	if (!Comments) {
		if (doprmt)
			printf(gettext("comments? "));
		Comments = getcomments();
	}
}


static char *
getresp(repstr,result)
char *repstr;
char *result;
{
	char line[MAXLINE];	/* Line length for typed in MR numbers */
	register int done, sz;
	register char *p;
	extern char	had_standinp;

	result[0] = 0;
	done = 0;
	/*
	save old fatal flag values and change to
	values inside ()
	*/
	FSAVE(FTLEXIT | FTLMSG | FTLCLN);
	if ((had_standinp && (!HADY || (Domrs && !HADM)))) {
		Ffile = 0;
		fatal(gettext("standard input specified w/o -y and/or -m keyletter (de16)"));
	}
	/*
	restore the old flag values and process if above
	conditions were not met
	*/
	FRSTR();
	sz = sizeof(line) - size(repstr);
	while (!done && fgets(line,sz,stdin) != NULL) {
		p = strend(line);
		if (*--p == '\n') {
			if (*--p == '\\') {
				copy(repstr,p);
			}
			else {
				*++p = 0;
				++done;
			}
		}
		else
			fatal(gettext("line too long (co18)"));
		if ((int) (size(line) + size(result)) > RESPSIZE)
			fatal(gettext("response too long (co19)"));
		strcat(result,line);
	}
	return(result);
}

static
char *
getcomments()
{
  char *buffer;
  char *temp;
  int c;
  int bufsiz = BUFSIZ;	/* Just an increment for malloc() */
  int cnt = 0;
  extern char had_standinp;
  
  /*
	save old fatal flag values and change to
	values inside ()
	*/
	FSAVE(FTLEXIT | FTLMSG | FTLCLN);
	if ((had_standinp && (!HADY || (Domrs && !HADM)))) {
		Ffile = 0;
		fatal("standard input specified w/o -y and/or -m keyletter (de16)");
	}
	/*
	restore the old flag values and process if above
	conditions were not met
	*/
	FRSTR();

	buffer = (char *)malloc(bufsiz);
	while ((c = getchar()) != EOF) {
		if (c == '\n') {
			if (buffer[cnt-1] == '\\') {
				/*
				 * Just a continuation.
				 */
				cnt -= 1;
			} else {
				/*
				 * End of input. It would be nice if EOT
				 * were something modern like ^D.
				 */
				break;
			}
		}
		buffer[cnt++] = c;
		if (cnt == bufsiz-1) {
			bufsiz += BUFSIZ;
			temp = (char *)malloc(bufsiz);
			strncpy(temp,buffer,cnt);
			free(buffer);
			buffer = temp;
		}
	}
	if (buffer[cnt-1] == '\n')
		buffer[cnt-1] = '\0';
	else
		buffer[cnt] = '\0';
	return (buffer);
}

static char	*Qarg[NVARGS];
char	**Varg = Qarg;

int
valmrs(pkt,pgm)
struct packet *pkt;
char *pgm;
{
	register int i;
	int st;
	register char *p;

	Varg[0] = pgm;
	Varg[1] = auxf(pkt->p_file,'g');
	if ((p = pkt->p_sflags[TYPEFLAG - 'a']) != NULL)
		Varg[2] = p;
	else
		Varg[2] = Null;
	if ((i = vfork()) < 0) {
#ifdef	HAVE_VFORK
		Fflags |= FTLVFORK;
#endif
		fatal(gettext("cannot fork; try again (co20)"));
	}
	else if (i == 0) {
		for (i = 4; i < 15; i++) {
#ifdef	F_SETFD
			fcntl(i, F_SETFD, 1);
#else
			(void) close(i);
#endif
		}
		execvp(pgm,Varg);
		_exit(1);
	}
	else {
		wait(&st);
		return(st);
	}
	/*NOTREACHED*/
	return (0);	/* fake for gcc */
}

# define	LENMR	60

void
mrfixup()
{
	register char **argv, *p, c;
	char *ap;
	unsigned int len;

	argv = &Varg[VSTART];
	p = Mrs;
	NONBLANK(p);
	for (ap = p; *p; p++) {
		if (*p == ' ' || *p == '\t') {
			if (argv >= &Varg[(NVARGS - 1)])
				fatal(gettext("too many MRs (co21)"));
			c = *p;
			*p = 0;
			if ((len = size(ap)) > LENMR)
				fatal(gettext("MR number too long (co24)"));
			*argv = stalloc(len);
			copy(ap,*argv);
			*p = c;
			argv++;
			NONBLANK(p);
			ap = p;
		}
	}
	--p;
	if (*p != ' ' && *p != '\t') {
		if ((len = size(ap)) > LENMR)
			fatal(gettext("MR number too long (co24)"));
		copy(ap,*argv++ = stalloc(len));
	}
	*argv = 0;
}


# define STBUFSZ	500

char *
stalloc(n)
register unsigned int n;
{
	static char stbuf[STBUFSZ];
	static int stind = 0;
	register char *p;

	p = &stbuf[stind];
	if (&p[n] >= &stbuf[STBUFSZ-1])
		fatal(gettext("out of space (co22)"));
	stind += n;
	return(p);
}


char *
savecmt(p)
register char *p;
{
	register char	*p1, *p2;
	int	ssize, nlcnt;

	nlcnt = 0;
	for (p1 = p; *p1; p1++)
		if (*p1 == '\n')
			nlcnt++;
/*
 *	ssize is length of line plus mush plus number of newlines
 *	times number of control characters per newline.
*/
	ssize = (strlen(p) + 4 + (nlcnt * 3)) & (~1);
	p1 = fmalloc((unsigned) ssize);
	p2 = p1;
	/*CONSTCOND*/
	while (1) {
		while(*p && *p != '\n')
			*p1++ = *p++;
		if (*p == '\0') {
			*p1 = '\0';
			return(p2);
		}
		else {
			p++;
			*p1++ = '\n';
			*p1++ = CTLCHAR;
			*p1++ = COMMENTS;
			*p1++ = ' ';
		}
	}
	/*NOTREACHED*/
}

char *
get_Sccs_Comments () {
	return(getcomments());
}
