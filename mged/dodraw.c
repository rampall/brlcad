/*
 *			D R A W S O L . C
 *
 * Functions -
 *	drawHsolid	Draw a COMGEOM solid for the VG
 *	freevgcore	De-allocate VG core
 *
 *	Ballistic Research Laboratory
 *	U. S. Army
 */

#include "ged_types.h"
#include "3d.h"
#include "ged.h"
#include "solid.h"
#include "dir.h"
#include "vmath.h"
#include "commo.h"
#include "dm.h"

extern void	perror();
extern int	printf(), write();

#define MAX(a,b)	if( (b) > (a) )  a = b

struct commo	commo;		/* Structure to be sent to GED2 */

struct commi	commi;		/* Structure to be rcvd from GED2 */

int	reg_error;	/* error encountered in region processing */
int	no_memory;	/* flag indicating memory for drawing is used up */

/*
 *			D R A W H S O L I D
 *
 * Returns -
 *	-1	on error
 *	 0	if NO OP
 *	 1	if solid was drawn
 */
int
drawHsolid( sp, flag, pathpos, xform, recordp )
register struct solid *sp;		/* solid structure */
int flag;
int pathpos;
matp_t xform;
union record *recordp;
{
	register int i;

	if( regmemb >= 0 ) {
		/* processing a member of a processed region */
		/* regmemb  =>  number of members left */
		/* regmemb == 0  =>  last member */
		/* reg_error > 0  =>  error condition  no more processing */
		if(reg_error) { 
			if(regmemb == 0) {
				reg_error = 0;
				regmemb = -1;
			}
			return(-1);		/* ERROR */
		}
		commo.o_flag = flag;
		commo.o_more = regmemb;		/* members left to process */
		commo.o_pos = memb_loc;		/* disk loc. of this member */
		if(memb_oper == UNION)
			commo.o_flag = 999;

		mat_copy( commo.o_mat, xform );

		/* The hard part */
		proc_reg( path[reg_pathpos], xform, flag );

		if(commi.i_type == MS_ERROR) {
			/* error somwhere */
			(void)printf("will skip region: %s\n",
					path[reg_pathpos]->d_namep);
			reg_error = 1;
			if(regmemb == 0) {
				regmemb = -1;
				reg_error = 0;
			}
			commi.i_type = MS_DRAW;
			return(-1);		/* ERROR */
		}
		reg_error = 0;		/* reset error flag */

		/* if type == MS_MORE  then more member solids to be processed
		 *    so no drawing was done
		 */
		if(commi.i_type == MS_MORE)
			return(0);		/* NOP */

		/* See if write to display memory succeeded */
		if( commi.i_size == 0 )  {
			no_memory = 1;
			return(-1);		/* ERROR */
		}
		sp->s_addr = commi.i_addr;
		sp->s_bytes = commi.i_size;
		VMOVE( sp->s_center, commi.i_center );
		sp->s_size = commi.i_scale;
	}  else  {
		/* Doing a normal solid */
		dl_preamble( flag == ROOT );
	
		switch( recordp->u_id )  {

		case ID_SOLID:
			switch( recordp->s.s_type )  {

			case GENARB8:
				draw_arb8( &recordp->s, xform );
				break;

			case GENTGC:
				draw_tgc( recordp->s.s_values, xform );
				break;

			case GENELL:
				draw_ell( recordp->s.s_values, xform );
				break;

			case TOR:
				draw_tor( recordp->s.s_values, xform );
				break;

			default:
				(void)printf("draw:  bad SOLID type %d.\n",
					recordp->s.s_type );
				return(-1);		/* ERROR */
			}
			break;

		case ID_ARS_A:
			draw_ars( &recordp->a, path[pathpos], xform );
			break;

		case ID_P_HEAD:
			draw_poly( path[pathpos], xform );
			break;

		default:
			(void)printf("draw:  bad database OBJECT type %d\n",
				recordp->u_id );
			return(-1);			/* ERROR */
		}

		/* Finish off the display subroutine */
		dl_epilogue();

		sp->s_center[X] = dl_xcent;
		sp->s_center[Y] = dl_ycent;
		sp->s_center[Z] = dl_zcent;
		sp->s_center[H] = 1;
		sp->s_size = dl_scale;

		/* Determine displaylist memory requirement */
		sp->s_bytes = dl_size();

		/* Allocate displaylist storage for object */
		sp->s_addr = memalloc( sp->s_bytes );
		if( sp->s_addr == 0 )  {
			no_memory = 1;
			(void)printf("draw: out of Displaylist\n");
			return(-1);		/* ERROR */
		}
		sp->s_bytes = dl_load( sp->s_addr, sp->s_bytes );
	}

	/* set solid/dashed line flag */
	if( sp != illump )  {
		sp->s_iflag = DOWN;
		sp->s_soldash = flag;

		if(regmemb == 0) {
			/* done processing a region */
			regmemb = -1;
			sp->s_last = reg_pathpos;
			sp->s_Eflag = 1;	/* This is processed region */
		}  else  {
			sp->s_Eflag = 0;	/* This is a solid */
			sp->s_last = pathpos;
		}
		/* Copy path information */
		for( i=0; i<=sp->s_last; i++ )
			sp->s_path[i] = path[i];

		/* Add to linked list of solid structs */
		APPEND_SOLID( sp, HeadSolid.s_back );
	} else {
		/* replacing illuminated solid -- struct already linked in */
		sp->s_iflag = UP;
	}

	/* Compute maximum */
	MAX( maxview, sp->s_center[X] + sp->s_size );
	MAX( maxview, sp->s_center[Y] + sp->s_size );
	MAX( maxview, sp->s_center[Z] + sp->s_size );

	return(1);		/* OK */
}

/*
 *			F R E E V G C O R E
 *
 * This routine is used to recycle displaylist memory.
 */
void
freevgcore( addr, bytes )
unsigned	addr;
unsigned	bytes;
{
	memfree( bytes, addr );

	/* reset memory used up flag */
	no_memory = 0;
	return;		/* OK */
}



#ifdef never
/*
 *			M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This KLUDGE is necessary because pipes
 * may not return any more than 512 characters on a single read.
 */
static int
mread(fd, bufp, n)
int fd;
register char	*bufp;
unsigned	n;
{
	register unsigned	count = 0;
	register int		nread;

	do {
		nread = read(fd, bufp, n-count);
		if(nread == -1)
			return(nread);
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}
#endif
