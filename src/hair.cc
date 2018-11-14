#include <gmath/gmath.h>
#include <stdlib.h>
#include <string>

#include "kdtree.h"
#include "hair.h"

struct Triangle {
	Vec3 v[3];
	Vec3 n[3];
};

Hair::Hair() {}
Hair::~Hair() {}

static Vec3 calc_rand_point(const Triangle &tr, Vec3 *bary)
{
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;

	if(u + v > 1) {
		u = 1 - u;
		v = 1 - v;
	}

	float c = 1 - (u + v);

	Vec3 rp = u * tr.v[0] + v * tr.v[1] + c * tr.v[3];

	bary->x = u;
	bary->y = v;
	bary->z = c;

	return rp;
}

static void get_spawn_triangles(const Mesh *m, float thresh, std::vector<Triangle> *faces)
{
	for(size_t i=0; i<m->indices.size() / 3; i++) {
		bool is_spawn = true;
		int idx[3];
		for(int j=0; j<3; j++) {
			idx[j] = i * 3 + j;
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
			}
			faces->push_back(t);
		}
	}
}

bool Hair::init(const Mesh *m, int max_num_spawns, float thresh)
{
	std::vector<Triangle> faces;
	kdtree *kd = kd_create(3);
	const float min_dist = 0.05;

	get_spawn_triangles(m, thresh, &faces);

	for(int i = 0; i < max_num_spawns; i++) {
		// Poisson
		int rnum = (float)((float)rand() / (float)RAND_MAX) * faces.size();
		Triangle rtriangle = faces[rnum];
		Vec3 bary;
		Vec3 rpoint = calc_rand_point(rtriangle, &bary);

		kdres *res = kd_nearest3f(kd, rpoint.x, rpoint.y, rpoint.z);
		if (!kd_res_end(res)) {
			Vec3 nearest;
			kd_res_item3f(res, &nearest.x, &nearest.y, &nearest.z);
			if(distance_sq(rpoint, nearest) < min_dist * min_dist)
				continue;
		}

		/* weighted sum of the triangle's vertex normals */
		Vec3 spawn_dir = rtriangle.n[0] * bary.x + rtriangle.n[1] * bary.y + rtriangle.n[2] * bary.z;
		spawn_directions.push_back(normalize(spawn_dir));
		spawn_points.push_back(rpoint);
		kd_insert3f(kd, rpoint.x, rpoint.y, rpoint.z, 0);
	}

	kd_free(kd);
	return true;
}

void Hair::draw() const
{
}
