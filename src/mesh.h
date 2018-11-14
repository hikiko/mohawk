#ifndef MESH_H_
#define MESH_H_

#include <stdint.h>
#include <vector>
#include <gmath/gmath.h>

#define MESH_ALL (0xffffffff)

enum {
	MESH_VERTEX = 1,
	MESH_NORMAL = 2,
	MESH_COLOR = 4,
	MESH_INDEX = 8
};

struct Aabb {
	Vec3 v0;
	Vec3 v1;
};

class Mesh {
private:
	unsigned int vbo_vertices;
	unsigned int vbo_normals;
	unsigned int vbo_colors;
	unsigned int ibo;

	int num_vertices;
	int num_indices;

public:
	Mesh();
	~Mesh();

	Aabb bbox;

	std::string name;
	std::vector<uint16_t> indices;
	std::vector<Vec3> vertices;
	std::vector<Vec3> normals;
	std::vector<Vec3> colors;

	void draw() const;
	void update_vbo(unsigned int which);

	void calc_bbox();
};

std::vector<Mesh*> load_meshes(const char *fname);

#endif // MESH_H_
