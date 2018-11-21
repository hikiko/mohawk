#include <GL/glew.h>

#include <float.h>
#include <gmath/gmath.h>
#include <stdlib.h>
#include <string>

#include "kdtree.h"
#include "hair.h"

struct Triangle {
	Vec3 v[3];
	Vec3 n[3];
};

Hair::Hair()
{
	hair_length = 0.5;
}

Hair::~Hair()
{
}

static Vec3 calc_rand_point(const Triangle &tr, Vec3 *bary)
{
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;

	if(u + v > 1) {
		u = 1 - u;
		v = 1 - v;
	}

	float c = 1 - (u + v);

	Vec3 rp = u * tr.v[0] + v * tr.v[1] + c * tr.v[2];

	bary->x = u;
	bary->y = v;
	bary->z = c;

	return rp;
}

static void get_spawn_triangles(const Mesh *m, float thresh, std::vector<Triangle> *faces)
{
	if (!m) {
		fprintf(stderr, "Func: %s, invalid mesh.\n", __func__);
		exit(1);
	}
	float min_y = FLT_MAX;
	float max_y = -FLT_MAX;

	for(size_t i=0; i<m->indices.size() / 3; i++) {
		bool is_spawn = true;
		int idx[3];
		for(int j=0; j<3; j++) {
			idx[j] = m->indices[i * 3 + j];
			float c = (m->colors[idx[j]].x + m->colors[idx[j]].y + m->colors[idx[j]].z) / 3;
			if (c >= thresh) {
				is_spawn = false;
				break;
			}
		}

		if(is_spawn) {
			Triangle t;
			for(int j=0; j<3; j++) {
				t.v[j] = m->vertices[idx[j]];
				t.n[j] = m->normals[idx[j]];
				if(t.v[j].y < min_y)
					min_y = t.v[j].y;
				if(t.v[j].y > max_y)
					max_y = t.v[j].y;
			}
			faces->push_back(t);
		}
	}
	printf("spawn tri AABB: min y: %f max y: %f\n", min_y, max_y);
}

bool Hair::init(const Mesh *m, int max_num_spawns, float thresh)
{
	std::vector<Triangle> faces;
	kdtree *kd = kd_create(3);
	const float min_dist = 0.05;

	if(!m) {
		fprintf(stderr, "Func %s: invalid mesh.\n", __func__);
		return false;
	}

	get_spawn_triangles(m, thresh, &faces);

	for(int i = 0; i < max_num_spawns; i++) {
		// Poisson
		int rnum = (float)((float)rand() / (float)RAND_MAX) * faces.size();
		Triangle rtriangle = faces[rnum];
		Vec3 bary;
		Vec3 rpoint = calc_rand_point(rtriangle, &bary);

		kdres *res = kd_nearest3f(kd, rpoint.x, rpoint.y, rpoint.z);

		if (res && !kd_res_end(res)) {
			Vec3 nearest;
			kd_res_item3f(res, &nearest.x, &nearest.y, &nearest.z);
			if(distance_sq(rpoint, nearest) < min_dist * min_dist)
				continue;
		}

		HairStrand strand;
		/* weighted sum of the triangle's vertex normals */
		strand.spawn_dir = normalize(rtriangle.n[0] * bary.x + rtriangle.n[1] * bary.y + rtriangle.n[2] * bary.z);
		strand.spawn_pt = rpoint;
		hair.push_back(strand);

		kd_insert3f(kd, rpoint.x, rpoint.y, rpoint.z, 0);
	}

	kd_free(kd);

	for(size_t i=0; i<hair.size(); i++) {
		hair[i].pos = hair[i].spawn_pt + hair[i].spawn_dir * hair_length;
		
		/* orthonormal basis */
		Vec3 vk = hair[i].spawn_dir;
		Vec3 vi = Vec3(1, 0, 0);
		if(fabs(vk.x > 0.99)) {
			vi = Vec3(0, -1, 0);
		}
		Vec3 vj = normalize(cross(vk, vi));
		vi = cross(vj, vk);

		/* identity when the hair points to the z axis */
		Mat4 basis = Mat4(vi, vj, vk);

		for(int j=0; j<3; j++) {
			float angle = (float)j / 3.0 * 2 * M_PI;
			/* dir of each anchor relative to hair end */
			Vec3 dir = Vec3(cos(angle), sin(angle), 0);
			dir = basis * dir;
			hair[i].anchor_dirs[j] = hair[i].pos + dir - hair[i].spawn_pt;
		}
	}
	return true;
}

void Hair::draw() const
{
	glPushAttrib(GL_ENABLE_BIT);
//	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glPointSize(5);
	glLineWidth(2);

	glBegin(GL_LINES);
	for(size_t i=0; i<hair.size(); i++) {
		glColor3f(1, 0, 1);
		glVertex3f(hair[i].pos.x, hair[i].pos.y, hair[i].pos.z);
		Vec3 p = hair[i].spawn_pt;
		glVertex3f(p.x, p.y, p.z);
	}
	glEnd();

	glBegin(GL_POINTS);
	glColor3f(0.5, 1.0, 0.5);
	for(size_t i=0; i<hair.size(); i++) {
		for(int j=0; j<3; j++) {
			Vec3 p = hair[i].spawn_pt + hair[i].anchor_dirs[j];
			glVertex3f(p.x, p.y, p.z);
		}
	}
	glEnd();

	glPopAttrib();
}
