/*                    E X T R U D C I R C . C
 * BRL-CAD
 *
 * Copyright (c) 1990-2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file extrudcirc.c
 *
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 */

#include "./iges_struct.h"
#include "./iges_extern.h"


int
Extrudcirc( entityno , curve , evect )
int entityno;	/* extrusion entity number */
int curve;	/* circular arc entity number */
vect_t evect;	/* extrusion vector */
{
	point_t		base;		/* center of cylinder base */
	fastf_t		radius;		/* radius of cylinder */
	fastf_t		x1,y1;		/* Start point */
	fastf_t		x2,y2;		/* Terminate point */
	int		sol_num;	/* Solid number */

	/* Acquiring Data */

	if( dir[curve]->param <= pstart )
	{
		bu_log( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[curve]->direct , dir[curve]->name );
		return(0);
	}
	Readrec( dir[curve]->param );
	Readint( &sol_num , "" );
	Readcnv( &base[Z] , "" );
	Readcnv( &base[X] , "" );
	Readcnv( &base[Y] , "" );
	Readcnv( &x1 , "" );
	Readcnv( &y1 , "" );
	Readcnv( &x2 , "" );
	Readcnv( &y2 , "" );

	/* Check for closure */

	if( x1 != x2 || y1 != y2 )
	{
		bu_log( "Circular arc for extrusion is not closed:\n" );
		bu_log( "\textrusion entity D%07d (%s)\n" , dir[entityno]->direct ,
			dir[entityno]->name );
		bu_log( "\tarc entity D%07d (%s)\n" , dir[curve]->direct , dir[curve]->name );
		return( 0 );
	}

	radius = sqrt( (x1 - base[X])*(x1 - base[X]) + (y1 - base[Y])*(y1 - base[Y]) );


	/* Make an rcc */

	mk_rcc( fdout , dir[entityno]->name , base , evect , radius );

	return( 1 );
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
