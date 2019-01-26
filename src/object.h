#ifndef OBJECT_H_
#define OBJECT_H_

#include <gmath/gmath.h>

class CollSphere {
public:
	float radius;
	Vec3 center;

	CollSphere();

	bool contains(const Vec3 &v) const;
	Vec3 project_surf(const Vec3 &v) const;
};

#endif // OBJECT_H_
