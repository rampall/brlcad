/*
 *			E L L . C
 *
 *  Purpose -
 *	Intersect a ray with a Generalized Ellipsoid
 *
 *  Authors -
 *	Edwin O. Davisson	(Analysis)
 *	Michael John Muuss	(Programming)
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
static char RCSell[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./debug.h"

/*
 *  Algorithm:
 *  
 *  Given V, A, B, and C, there is a set of points on this ellipsoid
 *  
 *  { (x,y,z) | (x,y,z) is on ellipsoid defined by V, A, B, C }
 *  
 *  Through a series of Affine Transformations, this set will be
 *  transformed into a set of points on a unit sphere at the origin
 *  
 *  { (x',y',z') | (x',y',z') is on Sphere at origin }
 *  
 *  The transformation from X to X' is accomplished by:
 *  
 *  X' = S(R( X - V ))
 *  
 *  where R(X) =  ( A/(|A|) )
 *  		 (  B/(|B|)  ) . X
 *  		  ( C/(|C|) )
 *  
 *  and S(X) =	 (  1/|A|   0     0   )
 *  		(    0    1/|B|   0    ) . X
 *  		 (   0      0   1/|C| )
 *  
 *  To find the intersection of a line with the ellipsoid, consider
 *  the parametric line L:
 *  
 *  	L : { P(n) | P + t(n) . D }
 *  
 *  Call W the actual point of intersection between L and the ellipsoid.
 *  Let W' be the point of intersection between L' and the unit sphere.
 *  
 *  	L' : { P'(n) | P' + t(n) . D' }
 *  
 *  W = invR( invS( W' ) ) + V
 *  
 *  Where W' = k D' + P'.
 *  
 *  Let dp = D' dot P'
 *  Let dd = D' dot D'
 *  Let pp = P' dot P'
 *  
 *  and k = [ -dp +/- sqrt( dp*dp - dd * (pp - 1) ) ] / dd
 *  which is constant.
 *  
 *  Now, D' = S( R( D ) )
 *  and  P' = S( R( P - V ) )
 *  
 *  Substituting,
 *  
 *  W = V + invR( invS[ k *( S( R( D ) ) ) + S( R( P - V ) ) ] )
 *    = V + invR( ( k * R( D ) ) + R( P - V ) )
 *    = V + k * D + P - V
 *    = k * D + P
 *  
 *  Note that ``k'' is constant, and is the same in the formulations
 *  for both W and W'.
 *  
 *  NORMALS.  Given the point W on the ellipsoid, what is the vector
 *  normal to the tangent plane at that point?
 *  
 *  Map W onto the unit sphere, ie:  W' = S( R( W - V ) ).
 *  
 *  Plane on unit sphere at W' has a normal vector of the same value(!).
 *  
 *  The plane transforms back to the tangent plane at W, and this
 *  new plane (on the ellipsoid) has a normal vector of N, viz:
 *  
 *  N = inverse[ transpose(invR o invS) ] ( W' )
 *    = inverse[ transpose(invS) o transpose(invR) ] ( W' )
 *    = inverse[ inverse(S) o R ] ( W' )
 *    = invR o S ( W' )
 *    = invR( S( S( R( W - V ) ) ) )
 *
 *  Note that the normal vector produced above will not have unit length.
 */

struct ell_specific {
	vect_t	ell_V;		/* Vector to center of ellipsoid */
	vect_t	ell_Au;		/* unit-length A vector */
	vect_t	ell_Bu;
	vect_t	ell_Cu;
	vect_t	ell_invsq;	/* [ 1/(|A|**2), 1/(|B|**2), 1/(|C|**2) ] */
	mat_t	ell_SoR;	/* Scale(Rot(vect)) */
	mat_t	ell_invRSSR;	/* invRot(Scale(Scale(Rot(vect)))) */
};

/*
 *  			E L L _ P R E P
 *  
 *  Given a pointer to a GED database record, and a transformation matrix,
 *  determine if this is a valid ellipsoid, and if so, precompute various
 *  terms of the formula.
 *  
 *  Returns -
 *  	0	ELL is OK
 *  	!0	Error in description
 *  
 *  Implicit return -
 *  	A struct ell_specific is created, and it's address is stored in
 *  	stp->st_specific for use by ell_shot().
 */
ell_prep( vec, stp, mat, rtip )
register fastf_t	*vec;
struct soltab		*stp;
matp_t			mat;
struct rt_i		*rtip;
{
	register struct ell_specific *ell;
	LOCAL fastf_t	magsq_a, magsq_b, magsq_c;
	LOCAL mat_t	R;
	LOCAL mat_t	Rinv;
	LOCAL mat_t	SS;
	LOCAL mat_t	mtemp;
	LOCAL vect_t	A, B, C;
	LOCAL vect_t	Au, Bu, Cu;	/* A,B,C with unit length */
	LOCAL vect_t	w1, w2, P;	/* used for bounding RPP */
	LOCAL fastf_t	f;

	/*
	 *  For a fast way out, hand this solid off to the SPH routine.
	 *  If it takes it, then there is nothing to do, otherwise
	 *  the solid is an ELL.
	 */
	if( sph_prep( vec, stp, mat ) == 0 )
		return(0);		/* OK */

#define ELL_V	&vec[0*ELEMENTS_PER_VECT]
#define ELL_A	&vec[1*ELEMENTS_PER_VECT]
#define ELL_B	&vec[2*ELEMENTS_PER_VECT]
#define ELL_C	&vec[3*ELEMENTS_PER_VECT]

	/*
	 * Apply rotation only to A,B,C
	 */
	MAT4X3VEC( A, mat, ELL_A );
	MAT4X3VEC( B, mat, ELL_B );
	MAT4X3VEC( C, mat, ELL_C );

	/* Validate that |A| > 0, |B| > 0, |C| > 0 */
	magsq_a = MAGSQ( A );
	magsq_b = MAGSQ( B );
	magsq_c = MAGSQ( C );
	if( NEAR_ZERO(magsq_a, 0.005) ||
	     NEAR_ZERO(magsq_b, 0.005) ||
	     NEAR_ZERO(magsq_c, 0.005) ) {
		rt_log("ell(%s):  zero length A, B, or C vector\n",
			stp->st_name );
		return(1);		/* BAD */
	}

	/* Create unit length versions of A,B,C */
	f = 1.0/sqrt(magsq_a);
	VSCALE( Au, A, f );
	f = 1.0/sqrt(magsq_b);
	VSCALE( Bu, B, f );
	f = 1.0/sqrt(magsq_c);
	VSCALE( Cu, C, f );

	/* Validate that A.B == 0, B.C == 0, A.C == 0 (check dir only) */
	f = VDOT( Au, Bu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  A not perpendicular to B, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( Bu, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  B not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( Au, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  A not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}

	/* Solid is OK, compute constant terms now */
	GETSTRUCT( ell, ell_specific );
	stp->st_specific = (int *)ell;

	/* Apply full 4x4mat to V */
	MAT4X3PNT( ell->ell_V, mat, ELL_V );

	VSET( ell->ell_invsq, 1.0/magsq_a, 1.0/magsq_b, 1.0/magsq_c );
	VMOVE( ell->ell_Au, Au );
	VMOVE( ell->ell_Bu, Bu );
	VMOVE( ell->ell_Cu, Cu );

	mat_idn( ell->ell_SoR );
	mat_idn( R );

	/* Compute R and Rinv matrices */
	VMOVE( &R[0], Au );
	VMOVE( &R[4], Bu );
	VMOVE( &R[8], Cu );
	mat_trn( Rinv, R );			/* inv of rot mat is trn */

	/* Compute SoS (Affine transformation) */
	mat_idn( SS );
	SS[ 0] = ell->ell_invsq[0];
	SS[ 5] = ell->ell_invsq[1];
	SS[10] = ell->ell_invsq[2];

	/* Compute invRSSR */
	mat_mul( mtemp, SS, R );
	mat_mul( ell->ell_invRSSR, Rinv, mtemp );

	/* Compute SoR */
	VSCALE( &ell->ell_SoR[0], A, ell->ell_invsq[0] );
	VSCALE( &ell->ell_SoR[4], B, ell->ell_invsq[1] );
	VSCALE( &ell->ell_SoR[8], C, ell->ell_invsq[2] );

	/* Compute bounding sphere */
	VMOVE( stp->st_center, ell->ell_V );
	f = magsq_a;
	if( magsq_b > f )
		f = magsq_b;
	if( magsq_c > f )
		f = magsq_c;
	stp->st_aradius = stp->st_bradius = sqrt(f);

	/* Compute bounding RPP */
	VSET( w1, magsq_a, magsq_b, magsq_c );

	/* X */
	VSET( P, 1.0, 0, 0 );		/* bounding plane normal */
	MAT3XVEC( w2, R, P );		/* map plane to local coord syst */
	VELMUL( w2, w2, w2 );		/* square each term */
	f = VDOT( w1, w2 );
	f = f / sqrt(f);
	stp->st_min[X] = ell->ell_V[X] - f;	/* V.P +/- f */
	stp->st_max[X] = ell->ell_V[X] + f;

	/* Y */
	VSET( P, 0, 1.0, 0 );		/* bounding plane normal */
	MAT3XVEC( w2, R, P );		/* map plane to local coord syst */
	VELMUL( w2, w2, w2 );		/* square each term */
	f = VDOT( w1, w2 );
	f = f / sqrt(f);
	stp->st_min[Y] = ell->ell_V[Y] - f;	/* V.P +/- f */
	stp->st_max[Y] = ell->ell_V[Y] + f;

	/* Z */
	VSET( P, 0, 0, 1.0 );		/* bounding plane normal */
	MAT3XVEC( w2, R, P );		/* map plane to local coord syst */
	VELMUL( w2, w2, w2 );		/* square each term */
	f = VDOT( w1, w2 );
	f = f / sqrt(f);
	stp->st_min[Z] = ell->ell_V[Z] - f;	/* V.P +/- f */
	stp->st_max[Z] = ell->ell_V[Z] + f;

	return(0);			/* OK */
}

ell_print( stp )
register struct soltab *stp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;

	VPRINT("V", ell->ell_V);
	mat_print("S o R", ell->ell_SoR );
	mat_print("invRSSR", ell->ell_invRSSR );
}

/*
 *  			E L L _ S H O T
 *  
 *  Intersect a ray with an ellipsoid, where all constant terms have
 *  been precomputed by ell_prep().  If an intersection occurs,
 *  a struct seg will be acquired and filled in.
 *  
 *  Returns -
 *  	0	MISS
 *  	segp	HIT
 */
struct seg *
ell_shot( stp, rp, ap )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	register struct seg *segp;
	LOCAL vect_t	dprime;		/* D' */
	LOCAL vect_t	pprime;		/* P' */
	LOCAL vect_t	xlated;		/* translated vector */
	LOCAL fastf_t	dp, dd;		/* D' dot P', D' dot D' */
	LOCAL fastf_t	k1, k2;		/* distance constants of solution */
	FAST fastf_t	root;		/* root of radical */

	/* out, Mat, vect */
	MAT4X3VEC( dprime, ell->ell_SoR, rp->r_dir );
	VSUB2( xlated, rp->r_pt, ell->ell_V );
	MAT4X3VEC( pprime, ell->ell_SoR, xlated );

	dp = VDOT( dprime, pprime );
	dd = VDOT( dprime, dprime );

	if( (root = dp*dp - dd * (VDOT(pprime,pprime)-1.0)) < 0 )
		return(SEG_NULL);		/* No hit */
	root = sqrt(root);

	GET_SEG(segp, ap->a_resource);
	segp->seg_stp = stp;
	if( (k1=(-dp+root)/dd) <= (k2=(-dp-root)/dd) )  {
		/* k1 is entry, k2 is exit */
		segp->seg_in.hit_dist = k1;
		segp->seg_out.hit_dist = k2;
	} else {
		/* k2 is entry, k1 is exit */
		segp->seg_in.hit_dist = k2;
		segp->seg_out.hit_dist = k1;
	}
	return(segp);			/* HIT */
}

/*
 *  			E L L _ N O R M
 *  
 *  Given ONE ray distance, return the normal and entry/exit point.
 */
ell_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	LOCAL vect_t xlated;

	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
	VSUB2( xlated, hitp->hit_point, ell->ell_V );
	MAT4X3VEC( hitp->hit_normal, ell->ell_invRSSR, xlated );
	VUNITIZE( hitp->hit_normal );
}

/*
 *			E L L _ C U R V E
 *
 *  Return the curvature of the ellipsoid.
 */
ell_curve( cvp, hitp, stp, rp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
struct xray *rp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	vect_t	w4;		/* vector from V to hit point */
	fastf_t *aup, *bup, *cup;	/* ptr to Aunit, Bunit, Cunit used */
	fastf_t	fx, fy;
	fastf_t	fxx, fyy, fxy;
	fastf_t	e, f, g;
	fastf_t	t1, t2;
	fastf_t	t5, t6;
	fastf_t	a1, b1, c1;
	fastf_t	aa1, bb1, dd1, aa2, bb2, dd2;
	fastf_t	rc1sav, rc2sav;

	VSUB2( w4, hitp->hit_point, ell->ell_V );
	aup = ell->ell_Au;
	bup = ell->ell_Bu;
	cup = ell->ell_Cu;
	{
		fastf_t	*tup;
		fastf_t	*iap, *ibp, *icp, *itp;
		fastf_t	x0, y0, z0;
		FAST fastf_t	c4, a2, b2, c2;

		iap = &ell->ell_invsq[X];
		ibp = &ell->ell_invsq[Y];
		icp = &ell->ell_invsq[Z];
		/* find a coordinate system with non-zero z */
		while(1)  {
			x0 = VDOT( w4, aup );
			y0 = VDOT( w4, bup );
			z0 = VDOT( w4, cup );
			if( !NEAR_ZERO( z0, 0.001 ) )
				break;
			tup = aup;
			aup = bup;
			bup = cup;
			cup = tup;
			itp = iap;
			iap = ibp;
			ibp = icp;
			icp = itp;
		}
		a2 = 1.0 / (*iap);
		b2 = 1.0 / (*ibp);
		c2 = 1.0 / (*icp);
		c4 = c2 * c2;
		t2 = 1.0 / (a2*b2*z0*z0*z0);
		/* 1st and 2nd partials */
		fx = -(c2*x0) / (a2*z0);
		fy = -(c2*y0) / (b2*z0);
		fxx = -(c4*(b2 - y0*y0))*t2;
		fyy = -(c4*(a2 - x0*x0))*t2;
		fxy = -(c4*x0*y0)*t2;
	}
	e = 1.0 + fx*fx;
	f = fx*fy;
	g = 1.0 + fy*fy;
	t1 = e + fy*fy;
	t2 = 2.0*t1*sqrt(t1);
/*
	t5 = sqrt( (g*fxx - e*fyy)*(g*fxx - e*fyy) +
		4.0 * (g*fxy - f*fyy)*(e*fxy - f*fxx) );
*/
	/* XXX - t5 goes negative when we are nearly spherical */
	t5 = (g*fxx - e*fyy)*(g*fxx - e*fyy) +
		4.0 * (g*fxy - f*fyy)*(e*fxy - f*fxx);
	if( t5 < 0 ) {
		fprintf( stderr, "t5 Negative\n" );
		VPRINT( "Ray origin", rp->r_pt );
		VPRINT( "Ray dir", rp->r_dir );
		VPRINT( "w4", w4 );
		fprintf( stderr, "fx,fy,fxx,fyy,fxy = %e %e %e %e %e\n", fx, fy, fxx, fyy, fxy );
		fprintf( stderr, "e,f,g = %e %e %e\n", e, f, g );
		fprintf( stderr, "t3 = %e\n",  (g*fxx - e*fyy)*(g*fxx - e*fyy) );
		fprintf( stderr, "t4 = %e\n",  4.0*(g*fxy - f*fyy)*(e*fxy - f*fxx) );
		t5 = 0;
	}
	t5 = sqrt(t5);
	t6 = g*fxx + e*fyy - 2.0*f*fxy;

	rc1sav = (t6 - t5)/t2;
	rc2sav = (t6 + t5)/t2;
	if( (cvp->crv_c1 = rc1sav) < 0 )
		cvp->crv_c1 = -cvp->crv_c1;
	if( (cvp->crv_c2 = rc2sav) < 0 )
		cvp->crv_c2 = -cvp->crv_c2;

	/* smaller magnitude curvature first */
	if( cvp->crv_c1 > cvp->crv_c2 )  {
		FAST fastf_t f;
		f = cvp->crv_c2;
		cvp->crv_c2 = cvp->crv_c1;
		cvp->crv_c1 = f;
		f = rc2sav;
		rc2sav = rc1sav;
		rc1sav = f;
	}

	/* construct 1st possible eigenvector */
	aa1 = g*fxy - f*fyy;
	bb1 = f*fxy - g*fxx + rc1sav*t2/2.0;
	dd1 = aa1*aa1 + bb1*bb1;
	/* construct 2nd possible eigenvector */
	aa2 = f*fxy - e*fyy + rc1sav*t2/2.0;
	bb2 = e*fxy - f*fxx;
	dd2 = aa2*aa2 + bb2*bb2;
	if( (dd1 >= dd2) && (dd1 > 0) ) {
		a1 = aa1;
		b1 = bb1;
		c1 = aa1*fx + bb1*fy;
	} else if( dd2 > 0 ) {
		a1 = aa2;
		b1 = bb2;
		c1 = aa2*fx + bb2*fy;
	} else {
		/* point is umbilic, any tangent vector is OK */
		a1 = 1.0;
		b1 = 0.0;
		c1 = fx;
	}

	/* transform coords back to usual system */
	VCOMB3( cvp->crv_pdir, a1, aup, b1, bup, c1, cup );
	VUNITIZE( cvp->crv_pdir );

	if( VDOT( hitp->hit_normal, rp->r_dir ) > 0 )  {
		/* ray strikes surface from inside; make curv negative */
		cvp->crv_c1 = - cvp->crv_c1;
		cvp->crv_c2 = - cvp->crv_c2;
	}
}

/*
 *  			E L L _ U V
 *  
 *  For a hit on the surface of an ELL, return the (u,v) coordinates
 *  of the hit point, 0 <= u,v <= 1.
 *  u = azimuth
 *  v = elevation
 */
double rt_inv2pi =  0.15915494309189533619;		/* 1/(pi*2) */
double rt_invpi = 0.31830988618379067153;	/* 1/pi */

ell_uv( ap, stp, hitp, uvp )
struct application *ap;
struct soltab *stp;
register struct hit *hitp;
register struct uvcoord *uvp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	LOCAL vect_t work;
	LOCAL vect_t pprime;
	LOCAL fastf_t r;

	/* hit_point is on surface;  project back to unit sphere,
	 * creating a vector from vertex to hit point which always
	 * has length=1.0
	 */
	VSUB2( work, hitp->hit_point, ell->ell_V );
	MAT4X3VEC( pprime, ell->ell_SoR, work );
	/* Assert that pprime has unit length */

	uvp->uv_u = atan2( pprime[Y], pprime[X] ) * rt_inv2pi + 0.5;
	uvp->uv_v = asin( pprime[Z] ) * rt_invpi + 0.5;

	/* approximation: r / (circumference, 2 * pi * aradius) */
	r = ap->a_rbeam + ap->a_diverge * hitp->hit_dist;
	uvp->uv_du = uvp->uv_dv =
		rt_inv2pi * r / stp->st_aradius;
}

/*
 *			E L L _ F R E E
 */
ell_free( stp )
register struct soltab *stp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;

	rt_free( (char *)ell, "ell_specific" );
}

ell_class()
{
}

ell_plot()
{
}
