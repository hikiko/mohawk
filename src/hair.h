#ifndef PARTICLES_H_
#define PARTICLES_H_

#include <gmath/gmath.h>

#include "mesh.h" 

class Hair {
private:
	std::vector<Vec3> spawn_points;
	std::vector<Vec3> spawn_directions;

public:
	Hair();
	~Hair();

	bool init(const Mesh *m, int num_spawns, float thresh = 0.4);
	void draw() const;
};

#endif //PARTICLES_H_

