/*
 *			M A T . C
 *
 * 4 x 4 Matrix manipulation functions..............
 *
 *	mat_atan2()			Wrapper for library atan2()
 *	mat_zero( &m )			Fill matrix m with zeros
 *	mat_idn( &m )			Fill matrix m with identity matrix
 *	mat_copy( &o, &i )		Copy matrix i to matrix o
 *	mat_mul( &o, &i1, &i2 )		Multiply i1 by i2 and store in o
 *	mat_mul2( &i, &o )
 *	matXvec( &ov, &m, &iv )		Multiply m by vector iv, store in ov
 *	mat_inv( &om, &im )		Invert matrix im, store result in om
 *	mat_print( &title, &m )		Print matrix (with title) on stdout.
 *	mat_trn( &o, &i )		Transpose matrix i into matrix o
 *	mat_ae( &o, azimuth, elev)	Make rot matrix from azimuth+elevation
 *	mat_ae_vec( &az, &el, v )		Find az/elev from dir vector
 *	mat_angles( &o, alpha, beta, gama )	Make rot matrix from angles
 *	mat_eigen2x2()			Eigen values and vectors
 *	mat_lookat			Make rot mat:  xform from D to -Z
 *	mat_fromto			Make rot mat:  xform from A to B
 *	rt_mat_is_equal()		Is mat a equal to mat b?
 *
 *
 * Matrix array elements have the following positions in the matrix:
 *
 *				|  0  1  2  3 |		| 0 |
 *	  [ 0 1 2 3 ]		|  4  5  6  7 |		| 1 |
 *				|  8  9 10 11 |		| 2 |
 *				| 12 13 14 15 |		| 3 |
 *
 *
 *     preVector (vect_t)	 Matrix (mat_t)    postVector (vect_t)
 *
 *  Authors -
 *	Robert S. Miles
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSmat[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"

CONST double	mat_degtorad = 0.0174532925199433;
CONST double	mat_radtodeg = 57.29577951308230698802;

#if USE_PROTOTYPES
extern void	mat_print(CONST char *title, CONST mat_t m);
extern void	mat_vec_perp(vect_t new, CONST vect_t old);
#else
extern void	mat_print();
extern void	mat_vec_perp();
#endif

/*
 *			M A T _ A T A N 2
 *
 *  A wrapper for the system atan2().  On the Silicon Graphics,
 *  and perhaps on others, x==0 incorrectly returns infinity.
 */
double
mat_atan2(y,x)
double	y,x;
{
	if( x > -1.0e-20 && x < 1.0e-20 )  {
		/* X is equal to zero, check Y */
		if( y < -1.0e-20 )  return( -3.14159265358979323/2 );
		if( y >  1.0e-20 )  return(  3.14159265358979323/2 );
		return(0.0);
	}
	return( atan2( y, x ) );
}

/*
 *			M A T _ Z E R O
 *
 * Fill in the matrix "m" with zeros.
 */
void
mat_zero( m )
mat_t	m;
{
	register int i = 0;
	register matp_t mp = m;

	/* Clear everything */
	for(; i<16; i++)
		*mp++ = 0.0;
}


/*
 *			M A T _ I D N
 *
 * Fill in the matrix "m" with an identity matrix.
 */
void
mat_idn( m )
register mat_t	m;
{
	/* Clear everything first */
	mat_zero( m );

	/* Set ones in the diagonal */
	m[0] = m[5] = m[10] = m[15] = 1.0;
}


/*
 *			M A T _ C O P Y
 *
 * Copy the matrix
 */
void
mat_copy( dest, src )
register mat_t		dest;
register CONST mat_t	src;
{
	register int i;

	/* Copy all elements */
#	include "noalias.h"
	for( i=15; i>=0; i--)
		dest[i] = src[i];
}


/*
 *			M A T _ M U L
 *
 * Multiply matrix "a" by "b" and store the result in "o".
 * NOTE:  This is different from multiplying "b" by "a"
 * (most of the time!)
 * NOTE: "o" must not be the same as either of the inputs.
 */
void
mat_mul( o, a, b )
register mat_t		o;
register CONST mat_t	a;
register CONST mat_t	b;
{
	o[ 0] = a[ 0]*b[ 0] + a[ 1]*b[ 4] + a[ 2]*b[ 8] + a[ 3]*b[12];
	o[ 1] = a[ 0]*b[ 1] + a[ 1]*b[ 5] + a[ 2]*b[ 9] + a[ 3]*b[13];
	o[ 2] = a[ 0]*b[ 2] + a[ 1]*b[ 6] + a[ 2]*b[10] + a[ 3]*b[14];
	o[ 3] = a[ 0]*b[ 3] + a[ 1]*b[ 7] + a[ 2]*b[11] + a[ 3]*b[15];

	o[ 4] = a[ 4]*b[ 0] + a[ 5]*b[ 4] + a[ 6]*b[ 8] + a[ 7]*b[12];
	o[ 5] = a[ 4]*b[ 1] + a[ 5]*b[ 5] + a[ 6]*b[ 9] + a[ 7]*b[13];
	o[ 6] = a[ 4]*b[ 2] + a[ 5]*b[ 6] + a[ 6]*b[10] + a[ 7]*b[14];
	o[ 7] = a[ 4]*b[ 3] + a[ 5]*b[ 7] + a[ 6]*b[11] + a[ 7]*b[15];

	o[ 8] = a[ 8]*b[ 0] + a[ 9]*b[ 4] + a[10]*b[ 8] + a[11]*b[12];
	o[ 9] = a[ 8]*b[ 1] + a[ 9]*b[ 5] + a[10]*b[ 9] + a[11]*b[13];
	o[10] = a[ 8]*b[ 2] + a[ 9]*b[ 6] + a[10]*b[10] + a[11]*b[14];
	o[11] = a[ 8]*b[ 3] + a[ 9]*b[ 7] + a[10]*b[11] + a[11]*b[15];

	o[12] = a[12]*b[ 0] + a[13]*b[ 4] + a[14]*b[ 8] + a[15]*b[12];
	o[13] = a[12]*b[ 1] + a[13]*b[ 5] + a[14]*b[ 9] + a[15]*b[13];
	o[14] = a[12]*b[ 2] + a[13]*b[ 6] + a[14]*b[10] + a[15]*b[14];
	o[15] = a[12]*b[ 3] + a[13]*b[ 7] + a[14]*b[11] + a[15]*b[15];
}

/*
 *			M A T _ M U L 2
 *
 *  o = i * o
 *
 *  A convenience wrapper for mat_mul().
 */
void
mat_mul2( i, o )
register CONST mat_t	i;
register mat_t		o;
{
	mat_t	temp;

	mat_mul( temp, i, o );
	mat_copy( o, temp );
}

/*
 *			M A T X V E C
 *
 * Multiply the matrix "im" by the vector "iv" and store the result
 * in the vector "ov".  Note this is post-multiply, and
 * operates on 4-tuples.  Use MAT4X3VEC() to operate on 3-tuples.
 */
void
matXvec(ov, im, iv)
register hvect_t ov;
register CONST mat_t im;
register CONST hvect_t iv;
{
	register int eo = 0;		/* Position in output vector */
	register int em = 0;		/* Position in input matrix */
	register int ei;		/* Position in input vector */

	/* For each element in the output array... */
	for(; eo<4; eo++) {

		ov[eo] = 0;		/* Start with zero in output */

		for(ei=0; ei<4; ei++)
			ov[eo] += im[em++] * iv[ei];
	}
}


/*
 *			M A T _ I N V
 *
 * The matrix pointed at by "im" is inverted and stored in the area
 * pointed at by "om".
 */
/* 
 * Invert a 4-by-4 matrix using Algorithm 120 from ACM.
 * This is a modified Gauss-Jordan alogorithm
 * Note:  Inversion is done in place, with 3 work vectors
 */
void
mat_inv( output, input )
register mat_t	output;
CONST mat_t	input;
{
	register int i, j;			/* Indices */
	LOCAL int k;				/* Indices */
	LOCAL int	z[4];			/* Temporary */
	LOCAL fastf_t	b[4];			/* Temporary */
	LOCAL fastf_t	c[4];			/* Temporary */

	mat_copy( output, input );	/* Duplicate */

	/* Initialization */
	for( j = 0; j < 4; j++ )
		z[j] = j;

	/* Main Loop */
	for( i = 0; i < 4; i++ )  {
		FAST fastf_t y;				/* local temporary */

		k = i;
		y = output[i*4+i];
		for( j = i+1; j < 4; j++ )  {
			FAST fastf_t w;			/* local temporary */

			w = output[i*4+j];
			if( fabs(w) > fabs(y) )  {
				k = j;
				y = w;
			}
		}

		if( fabs(y) < SQRT_SMALL_FASTF )  {
			rt_log("mat_inv:  error! fabs(y)=%g\n", fabs(y));
			mat_print("singular matrix", input);
			rt_bomb("mat_inv: singular matrix\n");
			/* NOTREACHED */
		}
		y = 1.0 / y;

		for( j = 0; j < 4; j++ )  {
			FAST fastf_t temp;		/* Local */

			c[j] = output[j*4+k];
			output[j*4+k] = output[j*4+i];
			output[j*4+i] = - c[j] * y;
			temp = output[i*4+j] * y;
			b[j] = temp;
			output[i*4+j] = temp;
		}

		output[i*4+i] = y;
		j = z[i];
		z[i] = z[k];
		z[k] = j;
		for( k = 0; k < 4; k++ )  {
			if( k == i )  continue;
			for( j = 0; j < 4; j++ )  {
				if( j == i )  continue;
				output[k*4+j] = output[k*4+j] - b[j] * c[k];
			}
		}
	}

	/*  Second Loop */
	for( i = 0; i < 4; i++ )  {
		while( (k = z[i]) != i )  {
			LOCAL int p;			/* Local temp */

			for( j = 0; j < 4; j++ )  {
				FAST fastf_t w;		/* Local temp */

				w = output[i*4+j];
				output[i*4+j] = output[k*4+j];
				output[k*4+j] = w;
			}
			p = z[i];
			z[i] = z[k];
			z[k] = p;
		}
	}
}

/*
 *			M A T _ V T O H _ M O V E
 *
 * Takes a pointer to a [x,y,z] vector, and a pointer
 * to space for a homogeneous vector [x,y,z,w],
 * and builds [x,y,z,1].
 */
void
mat_vtoh_move( h, v )
register vect_t		h;
register CONST vect_t	v;
{
	h[X] = v[X];
	h[Y] = v[Y];
	h[Z] = v[Z];
	h[W] = 1.0;
}

/*
 *			M A T _ H T O V _ M O V E
 *
 * Takes a pointer to [x,y,z,w], and converts it to
 * an ordinary vector [x/w, y/w, z/w].
 * Optimization for the case of w==1 is performed.
 */
void
mat_htov_move( v, h )
register vect_t		v;
register CONST vect_t	h;
{
	register fastf_t inv;

	if( h[3] == 1.0 )  {
		v[X] = h[X];
		v[Y] = h[Y];
		v[Z] = h[Z];
	}  else  {
		if( h[W] == SMALL_FASTF )  {
			rt_log("mat_htov_move: divide by %f!\n", h[W]);
			return;
		}
		inv = 1.0 / h[W];
		v[X] = h[X] * inv;
		v[Y] = h[Y] * inv;
		v[Z] = h[Z] * inv;
	}
}

/*
 *			M A T _ P R I N T
 */
void
mat_print( title, m )
CONST char	*title;
CONST mat_t	m;
{
	register int	i;
	char		obuf[1024];	/* sprintf may be non-PARALLEL */
	register char	*cp;

	sprintf(obuf, "MATRIX %s:\n  ", title);
	cp = obuf+strlen(obuf);
	for(i=0; i<16; i++)  {
		sprintf(cp, " %8.3f", m[i]);
		cp += strlen(cp);
		if( i == 15 )  {
			break;
		} else if( (i&3) == 3 )  {
			*cp++ = '\n';
			*cp++ = ' ';
			*cp++ = ' ';
		}
	}
	*cp++ = '\0';
	rt_log("%s\n", obuf);
}

/*
 *			M A T _ T R N
 */
void
mat_trn( om, im )
mat_t			om;
register CONST mat_t	im;
{
	register matp_t op = om;

	*op++ = im[0];
	*op++ = im[4];
	*op++ = im[8];
	*op++ = im[12];

	*op++ = im[1];
	*op++ = im[5];
	*op++ = im[9];
	*op++ = im[13];

	*op++ = im[2];
	*op++ = im[6];
	*op++ = im[10];
	*op++ = im[14];

	*op++ = im[3];
	*op++ = im[7];
	*op++ = im[11];
	*op++ = im[15];
}

/*
 *			M A T _ A E
 *
 *  Compute a 4x4 rotation matrix given Azimuth and Elevation.
 *  
 *  Azimuth is +X, Elevation is +Z, both in degrees.
 *
 *  Formula due to Doug Gwyn, BRL.
 */
void
mat_ae( m, azimuth, elev )
register mat_t	m;
double		azimuth;
double		elev;
{
	LOCAL double sin_az, sin_el;
	LOCAL double cos_az, cos_el;

	azimuth *= mat_degtorad;
	elev *= mat_degtorad;

	sin_az = sin(azimuth);
	cos_az = cos(azimuth);
	sin_el = sin(elev);
	cos_el = cos(elev);

	m[0] = cos_el * cos_az;
	m[1] = -sin_az;
	m[2] = -sin_el * cos_az;
	m[3] = 0;

	m[4] = cos_el * sin_az;
	m[5] = cos_az;
	m[6] = -sin_el * sin_az;
	m[7] = 0;

	m[8] = sin_el;
	m[9] = 0;
	m[10] = cos_el;
	m[11] = 0;

	m[12] = m[13] = m[14] = 0;
	m[15] = 1.0;
}

/*
 *			M A T _ A E _ V E C
 *
 *  Find the azimuth and elevation angles that correspond to the
 *  direction (not including twist) given by a direction vector.
 */
void
mat_ae_vec( azp, elp, v )
fastf_t		*azp;
fastf_t		*elp;
CONST vect_t	v;
{
	register fastf_t	az;

	if( (az = mat_atan2( v[Y], v[X] ) * mat_radtodeg) < 0 )  {
		*azp = 360 + az;
	} else if( az >= 360 ) {
		*azp = az - 360;
	} else {
		*azp = az;
	}
	*elp = mat_atan2( v[Z], hypot( v[X], v[Y] ) ) * mat_radtodeg;
}

/*
 *			M A T _ A N G L E S
 *
 * This routine builds a Homogeneous rotation matrix, given
 * alpha, beta, and gamma as angles of rotation, in degrees.
 *
 * Alpha is angle of rotation about the X axis, and is done third.
 * Beta is angle of rotation about the Y axis, and is done second.
 * Gamma is angle of rotation about Z axis, and is done first.
 */
void
mat_angles( mat, alpha, beta, ggamma )
register mat_t	mat;
double alpha, beta, ggamma;
{
	LOCAL double calpha, cbeta, cgamma;
	LOCAL double salpha, sbeta, sgamma;

	if( alpha == 0.0 && beta == 0.0 && ggamma == 0.0 )  {
		mat_idn( mat );
		return;
	}

	alpha *= mat_degtorad;
	beta *= mat_degtorad;
	ggamma *= mat_degtorad;

	calpha = cos( alpha );
	cbeta = cos( beta );
	cgamma = cos( ggamma );

	salpha = sin( alpha );
	sbeta = sin( beta );
	sgamma = sin( ggamma );

	mat[0] = cbeta * cgamma;
	mat[1] = -cbeta * sgamma;
	mat[2] = sbeta;
	mat[3] = 0.0;

	mat[4] = salpha * sbeta * cgamma + calpha * sgamma;
	mat[5] = -salpha * sbeta * sgamma + calpha * cgamma;
	mat[6] = -salpha * cbeta;
	mat[7] = 0.0;

	mat[8] = salpha * sgamma - calpha * sbeta * cgamma;
	mat[9] = salpha * cgamma + calpha * sbeta * sgamma;
	mat[10] = calpha * cbeta;
	mat[11] = 0.0;
	mat[12] = mat[13] = mat[14] = 0.0;
	mat[15] = 1.0;
}

/*
 *			M A T _ E I G E N 2 X 2
 *
 *  Find the eigenvalues and eigenvectors of a
 *  symmetric 2x2 matrix.
 *	( a b )
 *	( b c )
 *
 *  The eigenvalue with the smallest absolute value is
 *  returned in val1, with its eigenvector in vec1.
 */
void
mat_eigen2x2( val1, val2, vec1, vec2, a, b, c )
fastf_t	*val1, *val2;
vect_t	vec1, vec2;
fastf_t	a, b, c;
{
	fastf_t	d, root;
	fastf_t	v1, v2;

	d = 0.5 * (c - a);

	/* Check for diagonal matrix */
	if( NEAR_ZERO(b, 1.0e-10) ) {
		/* smaller mag first */
		if( fabs(c) < fabs(a) ) {
			*val1 = c;
			VSET( vec1, 0.0, 1.0, 0.0 );
			*val2 = a;
			VSET( vec2, -1.0, 0.0, 0.0 );
		} else {
			*val1 = a;
			VSET( vec1, 1.0, 0.0, 0.0 );
			*val2 = c;
			VSET( vec2, 0.0, 1.0, 0.0 );
		}
		return;
	}

	root = sqrt( d*d + b*b );
	v1 = 0.5 * (c + a) - root;
	v2 = 0.5 * (c + a) + root;

	/* smaller mag first */
	if( fabs(v1) < fabs(v2) ) {
		*val1 = v1;
		*val2 = v2;
		VSET( vec1, b, d - root, 0.0 );
	} else {
		*val1 = v2;
		*val2 = v1;
		VSET( vec1, root - d, b, 0.0 );
	}
	VUNITIZE( vec1 );
	VSET( vec2, -vec1[Y], vec1[X], 0.0 );	/* vec1 X vec2 = +Z */
}

/*
 *			M A T _ F R O M T O
 *
 *  Given two vectors, compute a rotation matrix that will transform
 *  space by the angle between the two.  There are many
 *  candidate matricies.
 *
 *  The input 'from' and 'to' vectors need not be unit length.
 *  MAT4X3VEC( to, m, from ) is the identity that is created.
 */
void
mat_fromto( m, from, to )
mat_t		m;
CONST vect_t	from;
CONST vect_t	to;
{
	vect_t	test_to;
	vect_t	unit_from, unit_to;
	fastf_t	dot;

	/*
	 *  The method used here is from Graphics Gems, A. Glasner, ed.
	 *  page 531, "The Use of Coordinate Frames in Computer Graphics",
	 *  by Ken Turkowski, Example 6.
	 */
	mat_t	Q, Qt;
	mat_t	R;
	mat_t	A;
	mat_t	temp;
	vect_t	N, M;
	vect_t	w_prime;		/* image of "to" ("w") in Qt */

	VMOVE( unit_from, from );
	VUNITIZE( unit_from );		/* aka "v" */
	VMOVE( unit_to, to );
	VUNITIZE( unit_to );		/* aka "w" */

	/*  If from and to are the same or opposite, special handling
	 *  is needed, because the cross product isn't defined.
	 *  asin(0.00001) = 0.0005729 degrees (1/2000 degree)
	 */
	dot = VDOT(unit_from, unit_to);
	if( dot > 1.0-0.00001 )  {
		/* dot == 1, return identity matrix */
		mat_idn(m);
		return;
	}
	if( dot < -1.0+0.00001 )  {
		/* dot == -1, select random perpendicular N vector */
		mat_vec_perp( N, unit_from );
	} else {
		VCROSS( N, unit_from, unit_to );
		VUNITIZE( N );			/* should be unnecessary */
	}
	VCROSS( M, N, unit_from );
	VUNITIZE( M );			/* should be unnecessary */

	/* Almost everything here is done with pre-multiplys:  vector * mat */
	mat_idn( Q );
	VMOVE( &Q[0], unit_from );
	VMOVE( &Q[4], M );
	VMOVE( &Q[8], N );
	mat_trn( Qt, Q );

	/* w_prime = w * Qt */
	MAT4X3VEC( w_prime, Q, unit_to );	/* post-multiply by transpose */

	mat_idn( R );
	VMOVE( &R[0], w_prime );
	VSET( &R[4], -w_prime[Y], w_prime[X], w_prime[Z] );
	VSET( &R[8], 0, 0, 1 );		/* is unnecessary */

	mat_mul( temp, R, Q );
	mat_mul( A, Qt, temp );
	mat_trn( m, A );		/* back to post-multiply style */

	/* Verify that it worked */
	MAT4X3VEC( test_to, m, unit_from );
	dot = VDOT( unit_to, test_to );
	if( dot < 0.98 || dot > 1.02 )  {
		rt_log("mat_fromto() ERROR!  from (%g,%g,%g) to (%g,%g,%g) went to (%g,%g,%g), dot=%g?\n",
			V3ARGS(from),
			V3ARGS(to),
			V3ARGS( test_to ), dot );
	}
}

/*
 *			M A T _ X R O T
 *
 *  Given the sin and cos of an X rotation angle, produce the rotation matrix.
 */
void
mat_xrot( m, sinx, cosx )
mat_t	m;
double	sinx, cosx;
{
	m[0] = 1.0;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;

	m[4] = 0.0;
	m[5] = cosx;
	m[6] = -sinx;
	m[7] = 0.0;

	m[8] = 0.0;
	m[9] = sinx;
	m[10] = cosx;
	m[11] = 0.0;

	m[12] = m[13] = m[14] = 0.0;
	m[15] = 1.0;
}

/*
 *			M A T _ Y R O T
 *
 *  Given the sin and cos of a Y rotation angle, produce the rotation matrix.
 */
void
mat_yrot( m, siny, cosy )
mat_t	m;
double	siny, cosy;
{
	m[0] = cosy;
	m[1] = 0.0;
	m[2] = -siny;
	m[3] = 0.0;

	m[4] = 0.0;
	m[5] = 1.0;
	m[6] = 0.0;
	m[7] = 0.0;

	m[8] = siny;
	m[9] = 0.0;
	m[10] = cosy;

	m[11] = 0.0;
	m[12] = m[13] = m[14] = 0.0;
	m[15] = 1.0;
}

/*
 *			M A T _ Z R O T
 *
 *  Given the sin and cos of a Z rotation angle, produce the rotation matrix.
 */
void
mat_zrot( m, sinz, cosz )
mat_t	m;
double	sinz, cosz;
{
	m[0] = cosz;
	m[1] = -sinz;
	m[2] = 0.0;
	m[3] = 0.0;

	m[4] = sinz;
	m[5] = cosz;
	m[6] = 0.0;
	m[7] = 0.0;

	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = 1.0;
	m[11] = 0.0;

	m[12] = m[13] = m[14] = 0.0;
	m[15] = 1.0;
}


/*
 *			M A T _ L O O K A T
 *
 *  Given a direction vector D of unit length,
 *  product a matrix which rotates that vector D onto the -Z axis.
 *  This matrix will be suitable for use as a "model2view" matrix.
 *
 *  This is done in several steps.
 *	1)  Rotate D about Z to match +X axis.  Azimuth adjustment.
 *	2)  Rotate D about Y to match -Y axis.  Elevation adjustment.
 *	3)  Rotate D about Z to make projection of X axis again point
 *	    in the +X direction.  Twist adjustment.
 *	4)  Optionally, flip sign on Y axis if original Z becomes inverted.
 *	    This can be nice for static frames, but is astonishing when
 *	    used in animation.
 */
void
mat_lookat( rot, dir, yflip )
mat_t		rot;
CONST vect_t	dir;
int		yflip;
{
	mat_t	first;
	mat_t	second;
	mat_t	prod12;
	mat_t	third;
	vect_t	x;
	vect_t	z;
	vect_t	t1;
	fastf_t	hypot_xy;
	vect_t	xproj;
	vect_t	zproj;

	/* First, rotate D around Z axis to match +X axis (azimuth) */
	hypot_xy = hypot( dir[X], dir[Y] );
	mat_zrot( first, -dir[Y] / hypot_xy, dir[X] / hypot_xy );

	/* Next, rotate D around Y axis to match -Z axis (elevation) */
	mat_yrot( second, -hypot_xy, -dir[Z] );
	mat_mul( prod12, second, first );

	/* Produce twist correction, by re-orienting projection of X axis */
	VSET( x, 1, 0, 0 );
	MAT4X3VEC( xproj, prod12, x );
	hypot_xy = hypot( xproj[X], xproj[Y] );
	if( hypot_xy < 1.0e-10 )  {
		rt_log("Warning: mat_lookat:  unable to twist correct, hypot=%g\n", hypot_xy);
		VPRINT( "xproj", xproj );
		mat_copy( rot, prod12 );
		return;
	}
	mat_zrot( third, -xproj[Y] / hypot_xy, xproj[X] / hypot_xy );
	mat_mul( rot, third, prod12 );

	if( yflip )  {
		VSET( z, 0, 0, 1 );
		MAT4X3VEC( zproj, rot, z );
		/* If original Z inverts sign, flip sign on resulting Y */
		if( zproj[Y] < 0.0 )  {
			mat_copy( prod12, rot );
			mat_idn( third );
			third[5] = -1;
			mat_mul( rot, third, prod12 );
		}
	}

	/* Check the final results */
	MAT4X3VEC( t1, rot, dir );
	if( t1[Z] > -0.98 )  {
		rt_log("Error:  mat_lookat final= (%g, %g, %g)\n", t1[X], t1[Y], t1[Z] );
	}
}

/*
 *			M A T _ V E C _ O R T H O
 *
 *  Given a vector, create another vector which is perpendicular to it,
 *  and with unit length.  This algorithm taken from Gift's arvec.f;
 *  a faster algorithm may be possible.
 */
void
mat_vec_ortho( out, in )
register vect_t	out;
register CONST vect_t	in;
{
	register int j, k;
	FAST fastf_t	f;
	register int i;

	if( NEAR_ZERO(in[X], 0.0001) && NEAR_ZERO(in[Y], 0.0001) &&
	    NEAR_ZERO(in[Z], 0.0001) )  {
		VSETALL( out, 0 );
		VPRINT("mat_vec_ortho: zero-length input", in);
		return;
	}

	/* Find component closest to zero */
	f = fabs(in[X]);
	i = X;
	j = Y;
	k = Z;
	if( fabs(in[Y]) < f )  {
		f = fabs(in[Y]);
		i = Y;
		j = Z;
		k = X;
	}
	if( fabs(in[Z]) < f )  {
		i = Z;
		j = X;
		k = Y;
	}
	f = hypot( in[j], in[k] );
	if( NEAR_ZERO( f, SMALL ) ) {
		VPRINT("mat_vec_ortho: zero hypot on", in);
		VSETALL( out, 0 );
		return;
	}
	f = 1.0/f;
	out[i] = 0.0;
	out[j] = -in[k]*f;
	out[k] =  in[j]*f;
}

/*
 *			M A T _ V E C _ P E R P
 *
 *  Given a vector, create another vector which is perpendicular to it.
 *  The output vector will have unit length only if the input vector did.
 */
void
mat_vec_perp( new, old )
vect_t		new;
CONST vect_t	old;
{
	register int i;
	LOCAL vect_t another;	/* Another vector, different */

	i = X;
	if( fabs(old[Y])<fabs(old[i]) )  i=Y;
	if( fabs(old[Z])<fabs(old[i]) )  i=Z;
	VSETALL( another, 0 );
	another[i] = 1.0;
	if( old[X] == 0 && old[Y] == 0 && old[Z] == 0 )  {
		VMOVE( new, another );
	} else {
		VCROSS( new, another, old );
	}
}

/*
 *			M A T _ S C A L E _ A B O U T _ P T
 *
 *  Build a matrix to scale uniformly around a given point.
 *
 *  Returns -
 *	-1	if scale is too small.
 *	 0	if OK.
 */
int
mat_scale_about_pt( mat, pt, scale )
mat_t		mat;
CONST point_t	pt;
CONST double	scale;
{
	mat_t	xlate;
	mat_t	s;
	mat_t	tmp;

	mat_idn( xlate );
	MAT_DELTAS_VEC_NEG( xlate, pt );

	mat_idn( s );
	if( NEAR_ZERO( scale, SMALL ) )  {
		mat_zero( mat );
		return -1;			/* ERROR */
	}
	s[15] = 1/scale;

	mat_mul( tmp, s, xlate );

	MAT_DELTAS_VEC( xlate, pt );
	mat_mul( mat, xlate, tmp );
	return 0;				/* OK */
}

/*
 *			M A T _ X F O R M _ A B O U T _ P T
 *
 *  Build a matrix to apply arbitary 4x4 transformation around a given point.
 */
void
mat_xform_about_pt( mat, xform, pt )
mat_t		mat;
CONST mat_t	xform;
CONST point_t	pt;
{
	mat_t	xlate;
	mat_t	tmp;

	mat_idn( xlate );
	MAT_DELTAS_VEC_NEG( xlate, pt );

	mat_mul( tmp, xform, xlate );

	MAT_DELTAS_VEC( xlate, pt );
	mat_mul( mat, xlate, tmp );
}

/*
 *			R T _ M A T _ I S _ E Q U A L
 *
 *  Returns -
 *	0	When matrices are not equal
 *	1	When matricies are equal
 */
int
rt_mat_is_equal(a,b,tol)
CONST mat_t	a;
CONST mat_t	b;
CONST struct rt_tol	*tol;
{
	register int i;
	register double f;
	register double tdist, tperp;

	RT_CK_TOL(tol);

	tdist = tol->dist;
	tperp = tol->perp;

	/*
	 * Check that the rotation part of the matrix (cosines) are within
	 * the perpendicular tolerance.
	 */
	for (i = 0; i < 16; i+=4) {
		f = a[i] - b[i];
		if ( !NEAR_ZERO(f, tperp)) return 0;
		f = a[i+1] - b[i+1];
		if ( !NEAR_ZERO(f, tperp)) return 0;
		f = a[i+2] - b[i+2];
		if ( !NEAR_ZERO(f, tperp)) return 0;
	}
	/*
	 * Check that the scale part of the matrix (ratio) is within the
	 * perpendicular tolerance.  There is no ratio tolerance so we use
	 * the tighter of dist or perp.
	 */
	f = a[15] - a[15];
	if ( !NEAR_ZERO(f, tperp)) return 0;

	/*
	 * Last, check that the translation part of the matrix (dist) are within
	 * the distance tolerance.
	 */
	for (i=3; i<12; i+=4) {
		f = a[i] - b[i];
		if ( !NEAR_ZERO(f, tdist)) return 0;
	}
	return 1;
}

#if 0
/*
 *			R T _ V E C T _ M A X M A G
 *
 *  Return the subscript (X, Y, Z) of the element of a vector (3-tuple)
 *  that has the maximum absolute value.
 */
int
rt_vect_maxmag( v )
CONST vect_t	v;
{
	if( fabs(v[X]) >= fabs(v[Y]) )  {
		/* X -vs- Z */
		if( fabs(v[X]) >= fabs(v[Z]) )
			return X;
		else
			return Z;
	}

	/* Y -vs- Z */
	if( fabs(v[Y]) >= fabs(v[Z]) )
		return Y;

	return Z;
}

#endif
