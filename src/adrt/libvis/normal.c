#include "normal.h"
#include "hit.h"
#include "adrt_common.h"
#include <stdio.h>


void vis_normal_init(vis_t *vis) {
  vis->work = vis_normal_work;
  vis->free = vis_normal_free;
}


void vis_normal_free(vis_t *vis) {
}


static void* normal_hit(tie_ray_t *ray, tie_id_t *id, tie_tri_t *tri, void *ptr) {
  common_triangle_t *t = ((common_triangle_t *)(tri->ptr));

  if(t->normals) {
    id->norm.v[0] = (1.0 - (id->alpha + id->beta)) * t->normals[0] + id->alpha * t->normals[3] + id->beta * t->normals[6];
    id->norm.v[1] = (1.0 - (id->alpha + id->beta)) * t->normals[1] + id->alpha * t->normals[4] + id->beta * t->normals[7];
    id->norm.v[2] = (1.0 - (id->alpha + id->beta)) * t->normals[2] + id->alpha * t->normals[5] + id->beta * t->normals[8];
  }

  return( t->mesh );
}


void vis_normal_work(vis_t *vis, tie_t *tie, tie_ray_t *ray, TIE_3 *pixel) {
  tie_id_t	id;
  common_mesh_t	*m;

  if((m = (common_mesh_t *)tie_work(tie, ray, &id, normal_hit, NULL))) {
    pixel->v[0] = (id.norm.v[0]+1) * 0.5;
    pixel->v[1] = (id.norm.v[1]+1) * 0.5;
    pixel->v[2] = (id.norm.v[2]+1) * 0.5;
  }
}
