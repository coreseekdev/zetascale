//----------------------------------------------------------------------------
// ZetaScale
// Copyright (c) 2016, SanDisk Corp. and/or all its affiliates.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published by the Free
// Software Foundation;
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License v2.1 for more details.
//
// A copy of the GNU Lesser General Public License v2.1 is provided with this package and
// can also be found at: http://opensource.org/licenses/LGPL-2.1
// You should have received a copy of the GNU Lesser General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
//----------------------------------------------------------------------------



#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<unistd.h>
#include	"zs.h"
#include	"utils/rico.h"
#include	"packet.h"


#define	PACKET_LINEMAX	1000000


struct rec_packet_structure {
	FILE	*f;
	char	*lvec[100000],
		lbuf0[PACKET_LINEMAX],
		lbuf1[PACKET_LINEMAX];
	bool	lbuf0full;
	uint	trxid;
};
struct stats_packet_structure {
	FILE	*f;
	char	lbuf0[PACKET_LINEMAX],
		lbuf1[PACKET_LINEMAX];
	uint	nbyte;
};


static char	**readlines( rec_packet_t *, uint *, int),
		*packetname( uint),
		*statspacketname( uint);
static bool	statsreadline( stats_packet_t *),
		badstatsdata( );
static void	baddata( ),
		nomem( ),
		zsmessage( char, char *, ...);


rec_packet_t	*
recovery_packet_open( uint cguid)
{

	rec_packet_t *r = 0;
	char *file = packetname( cguid);
	unless ((access( file, F_OK) == 0)
	and (access( file, R_OK|W_OK) == 0)
	and (access( file, X_OK) < 0))
		;//fprintf( stderr, "recovery packet not found (cguid %u)\n", cguid);
	else {
		zsmessage( 'I', "loading recovery packet %s", file);
		char *cmd;
		asprintf( &cmd, "gunzip < '%s'", file);
		unless ((cmd)
		and (r = malloc( sizeof *r)))
			nomem( );
		r->lbuf0full = FALSE;
		r->trxid = ~0;
		unless (r->f = popen( cmd, "r")) {
			zsmessage( 'E', "failed to load %s", file);
			free( r);
			r = 0;
		}
		free( cmd);
	}
	free( file);
	return (r);
}


rec_packet_trx_t	*
recovery_packet_get_trx( rec_packet_t *r)
{

	rec_packet_trx_t *t = malloc( sizeof *t);
	unless (t)
		nomem( );
	unless ((t->newnodes = readlines( r, &t->newcount, 0))
	and (t->oldnodes = readlines( r, &t->oldcount, 1)))
		return (0);
	return (t);
}


void
recovery_packet_free_trx( rec_packet_trx_t *t)
{
	uint	i;

	for (i=0; i<t->newcount; ++i)
		free( t->newnodes[i]);
	for (i=0; i<t->oldcount; ++i)
		free( t->oldnodes[i]);
	free( t);
}


void
recovery_packet_close( rec_packet_t *r)
{

	pclose( r->f);
	free( r);
}


/*
 * remove a packet
 *
 * Ensure packet contents are never applied, even if a crash happens
 * momentarily.
 */
void
recovery_packet_delete( uint cguid)
{

	char *file = packetname( cguid);
	int fd = open( file, O_WRONLY|O_TRUNC);
	unless (fd < 0) {
		ftruncate( fd, 0);
		fdatasync( fd);
		close( fd);
	}
	unlink( file);
	free( file);
}


stats_packet_t	*
stats_packet_open( uint cguid)
{

	stats_packet_t *s = 0;
	char *file = statspacketname( cguid);
	unless ((access( file, F_OK) == 0)
	and (access( file, R_OK|W_OK) == 0)
	and (access( file, X_OK) < 0))
		;//fprintf( stderr, "stats packet not found (cguid %u)\n", cguid);
	else {
		zsmessage( 'I', "loading stats packet %s", file);
		char *cmd;
		asprintf( &cmd, "gunzip < '%s'", file);
		unless ((cmd)
		and (s = malloc( sizeof *s)))
			nomem( );
		unless (s->f = popen( cmd, "r")) {
			zsmessage( 'E', "failed to load %s", file);
			free( s);
			s = 0;
		}
		free( cmd);
	}
	free( file);
	return (s);
}


stats_packet_node_t	*
stats_packet_get_node( stats_packet_t *s)
{

	unless (statsreadline( s))
		return (0);
	stats_packet_node_t *n = malloc( sizeof *n);
	unless (n)
		nomem( );
	n->datalen = s->nbyte;
	unless (n->data = malloc( n->datalen))
		nomem( );
	memcpy( n->data, s->lbuf1, n->datalen);
	return (n);
		
}


void
stats_packet_free_node( stats_packet_node_t *n)
{

	free( n->data);
	free( n);
}


void
stats_packet_close( stats_packet_t *s)
{

	fclose( s->f);
	free( s);
}


void
stats_packet_delete( uint cguid)
{

	char *file = statspacketname( cguid);
	int fd = open( file, O_WRONLY|O_TRUNC);
	unless (fd < 0) {
		ftruncate( fd, 0);
		fdatasync( fd);
		close( fd);
	}
	unlink( file);
	free( file);
}


static char	**
readlines( rec_packet_t *r, uint *count, int old)
{
	uint	cguid,
		bracketid,
		trxid,
		lineno,
		oldflag,
		i;

	uint c = 0;
	loop {
		unless (r->lbuf0full) {
			unless (fgets( r->lbuf0, sizeof r->lbuf0, r->f))
				break;
			unless (r->lbuf0[strlen( r->lbuf0)-1] == '\n') {
				zsmessage( 'E', "line too long in recovery packet");
				return (0);
			}
		}
		r->lbuf0full = TRUE;
		unless (sscanf( r->lbuf0, "%u %u %u %u %u %s", &cguid, &bracketid, &trxid, &lineno, &oldflag, r->lbuf1) == 6) {
			baddata( );
			return (0);
		}
		if (r->trxid == ~0)
			r->trxid = trxid;
		else unless (trxid == r->trxid) {
			r->trxid = trxid;
			break;
		}
		unless (oldflag == old)
			break;
		char *p = r->lbuf1;
		char *q = r->lbuf1;
		while (*p)
			if (*p == '\\') {
				unless ((p[1])
				and (p[2])) {
					baddata( );
					return (0);
				}
				static char HEX[] = "0123456789ABCDEF";
				*q++ = strchr( HEX, p[1])-HEX<<4 | strchr( HEX, p[2])-HEX;
				p += 3;
			}
			else if (*p == '@')
				*q++ = 0, p++;
			else
				*q++ = *p++;
		unless (c < nel( r->lvec)) {
			zsmessage( 'E', "too many ops in trx");
			return (0);
		}
		const uint n = q - r->lbuf1;
		unless (r->lvec[c] = malloc( n))
			nomem( );
		memcpy( r->lvec[c], r->lbuf1, n);
		++c;
		r->lbuf0full = FALSE;
	}
	unless (c)
		return (0);
	char **lvec = malloc( c*sizeof( *lvec));
	unless (lvec)
		nomem( );
	for (i=0; i<c; ++i)
		lvec[i] = r->lvec[i];
	*count = c;
	return (lvec);
}


static bool
statsreadline( stats_packet_t *s)
{
	uint	cguid,
		bracketid,
		trxid,
		lineno,
		oldflag,
		i;

	unless (fgets( s->lbuf0, sizeof s->lbuf0, s->f))
		return (FALSE);
	unless (s->lbuf0[strlen( s->lbuf0)-1] == '\n') {
		zsmessage( 'E', "line too long in stats packet");
		return (FALSE);
	}
	unless (sscanf( s->lbuf0, "%u %s", &cguid, s->lbuf1) == 2)
		return (badstatsdata( ));
	char *p = s->lbuf1;
	char *q = s->lbuf1;
	while (*p)
		if (*p == '\\') {
			unless ((p[1])
			and (p[2])) {
				baddata( );
				return (0);
			}
			static char HEX[] = "0123456789ABCDEF";
			*q++ = strchr( HEX, p[1])-HEX<<4 | strchr( HEX, p[2])-HEX;
			p += 3;
		}
		else if (*p == '@')
			*q++ = 0, p++;
		else
			*q++ = *p++;
	s->nbyte = q - s->lbuf1;
	return (TRUE);
}


static char	*
packetname( uint cguid)
{
	char	*crashdir,
		*file;

	ZSTransactionService( 0, 3, &crashdir);
	asprintf( &file, "%s/cguid-%d.gz", crashdir, cguid);
	unless (file)
		nomem( );
	return (file);
}


static char	*
statspacketname( uint cguid)
{
	char	*crashdir,
		*file;

	ZSTransactionService( 0, 3, &crashdir);
	asprintf( &file, "%s/stats-cguid-%d.gz", crashdir, cguid);
	unless (file)
		nomem( );
	return (file);
}


static void
baddata( )
{

	zsmessage( 'E', "bad data in recovery packet");
}


static bool
badstatsdata( )
{

	zsmessage( 'E', "bad data in stats packet");
	return (FALSE);
}


static void
nomem( )
{

	zsmessage( 'F', "out of memory");
	abort( );
}


static void
zsmessage( char level, char *mesg, ...)
{
	va_list	va;
	char	*s;

	va_start( va, mesg);
	vasprintf( &s, mesg, va);
	va_end( va);
	if (s)
		switch (level) {
		case 'I':
			ZSTransactionService( 0, 5, s);
			break;
		case 'E':
			ZSTransactionService( 0, 6, s);
			break;
		case 'F':
			ZSTransactionService( 0, 7, s);
		}
}
