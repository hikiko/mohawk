#include "object.h"

CollSphere::CollSphere()
{
	radius = 1.0;
}

bool CollSphere::contains(const Vec3 &v) const
{
	return length_sq(v - center) <= radius * radius;
}

Vec3 CollSphere::project_surf(const Vec3 &v) const
{
	return center + normalize(v - center) * radius;
}
