#ifndef PARTICLES_H_
#define PARTICLES_H_

#include <gmath/gmath.h>

#include "mesh.h" 

struct HairStrand {
	Vec3 pos;
	Vec3 velocity;
	/* directions relative to the spawn point */
	Vec3 anchor_dirs[3];
	Vec3 spawn_pt;
	Vec3 spawn_dir;
};

class Hair {
private:
	float hair_length;
	std::vector<HairStrand> hair;

public:
	Hair();
	~Hair();

	bool init(const Mesh *m, int num_spawns, float thresh = 0.4);
	void draw() const;
};

#endif //PARTICLES_H_

