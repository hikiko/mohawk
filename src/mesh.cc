#include <GL/glew.h>

#include <assert.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>

#include <float.h>
#include <imago2.h>

#include "mesh.h"

static bool check_tex_opaque(unsigned int tex);

Mesh::Mesh()
{
	vbo_vertices = 0;
	vbo_normals = 0;
	vbo_texcoords = 0;
	vbo_colors = 0;
	ibo = 0;

	num_vertices = 0;
	num_indices = 0;

	mtl.tex = 0;
	mtl.diffuse = Vec3(1, 1, 1);
	mtl.shininess = 50;
}

Mesh::~Mesh()
{
	if(vbo_vertices)
		glDeleteBuffers(1, &vbo_vertices);
	if(vbo_normals)
		glDeleteBuffers(1, &vbo_normals);
	if(vbo_texcoords)
		glDeleteBuffers(1, &vbo_texcoords);
	if(vbo_colors)
		glDeleteBuffers(1, &vbo_colors);
	if(ibo)
		glDeleteBuffers(1, &ibo);

	vertices.clear();
	normals.clear();
	texcoords.clear();
	colors.clear();
}

void Mesh::draw() const
{
	/* set material */
	float diff[4] = {
		mtl.diffuse.x, mtl.diffuse.y, mtl.diffuse.z, 1.0
	};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diff);

	float spec[4] = {
		mtl.specular.x, mtl.specular.y, mtl.specular.z, 1.0
	};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);

	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mtl.shininess);

	glPushAttrib(GL_ENABLE_BIT);
	if(mtl.tex) {
		glBindTexture(GL_TEXTURE_2D, mtl.tex);
		glEnable(GL_TEXTURE_2D);

		if(!mtl.tex_opaque) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	if(vbo_normals) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
		glNormalPointer(GL_FLOAT, 0, 0);
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if(vbo_texcoords) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords);
		glTexCoordPointer(2, GL_FLOAT, 0, 0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(vbo_colors) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
		glColorPointer(3, GL_FLOAT, 0, 0);
		glEnableClientState(GL_COLOR_ARRAY);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnableClientState(GL_VERTEX_ARRAY);

	if(ibo) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	} else {
		glDrawArrays(GL_TRIANGLES, 0, num_vertices);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glPopAttrib();
}

void Mesh::update_vbo(unsigned int which)
{
	if((which & MESH_NORMAL) && !normals.empty()) {
		if(!vbo_normals) {
			glGenBuffers(1, &vbo_normals);
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
		if(num_vertices != (int)normals.size()) {
			glBufferData(GL_ARRAY_BUFFER, normals.size() * 3 * sizeof(float),
					&normals[0], GL_STATIC_DRAW);
		}
		else {
			glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size() * 3 * sizeof(float),
					&normals[0]);
		}
	}

	if((which & MESH_TEXCOORDS) && !texcoords.empty()) {
		if(!vbo_texcoords) {
			glGenBuffers(1, &vbo_texcoords);
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords);
		if(num_vertices != (int)texcoords.size()) {
			glBufferData(GL_ARRAY_BUFFER, texcoords.size() * 2 * sizeof(float),
					&texcoords[0], GL_STATIC_DRAW);
		}
		else {
			glBufferSubData(GL_ARRAY_BUFFER, 0, texcoords.size() * 2 * sizeof(float),
					&texcoords[0]);
		}
	}

	if((which & MESH_COLOR) && !colors.empty()) {
		if(!vbo_colors) {
			glGenBuffers(1, &vbo_colors);
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
		if(num_vertices != (int)colors.size()) {
			glBufferData(GL_ARRAY_BUFFER, colors.size() * 3 * sizeof(float),
					&colors[0], GL_STATIC_DRAW);
		}
		else {
			glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * 3 * sizeof(float),
					&colors[0]);
		}
	}


	if(which & MESH_VERTEX) {
		assert(!vertices.empty());
		if(!vbo_vertices) {
			glGenBuffers(1, &vbo_vertices);
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		if(num_vertices != (int)vertices.size()) {
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * 3 * sizeof(float),
					&vertices[0], GL_STATIC_DRAW);
		}
		else {
			glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * 3 * sizeof(float),
					&vertices[0]);
		}
		num_vertices = vertices.size();
	}

	if((which & MESH_INDEX) && !indices.empty()) {
		if(!ibo) {
			glGenBuffers(1, &ibo);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		if(num_indices != (int)indices.size()) {
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * 2,
					&indices[0], GL_STATIC_DRAW);
		}
		else {
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * 2,
					&indices[0]);
		}
		num_indices = indices.size();
	}
}

std::vector<Mesh*> load_meshes(const char *fname)
{
	std::vector<Mesh*> meshes;
	unsigned int ai_flags = aiProcessPreset_TargetRealtime_Quality;
	const aiScene *scene = aiImportFile(fname, ai_flags);

	for(unsigned int j=0; j<scene->mNumMeshes; j++) {
		aiMesh *amesh = scene->mMeshes[j];
		aiMaterial *amtl = scene->mMaterials[amesh->mMaterialIndex];

		if(!amesh->HasPositions() || !amesh->mNumFaces)
			continue;

		Mesh *mesh = new Mesh;
		mesh->name = std::string(amesh->mName.C_Str());
		printf("loading mesh: %s\n", mesh->name.c_str());

		for(unsigned int i=0; i<amesh->mNumVertices; i++) {
			Vec3 vertex = Vec3(amesh->mVertices[i].x,
					amesh->mVertices[i].y,
					amesh->mVertices[i].z);
			mesh->vertices.push_back(vertex);
		}
		printf(" %u vertices\n", amesh->mNumVertices);

		if(amesh->HasNormals()) {
			for(unsigned int i=0; i<amesh->mNumVertices; i++) {
				Vec3 normal = Vec3(amesh->mNormals[i].x,
						           amesh->mNormals[i].y,
								   amesh->mNormals[i].z);
				mesh->normals.push_back(normal);
			}
		}

		if(amesh->HasTextureCoords(0)) {
			for(unsigned int i=0; i<amesh->mNumVertices; i++) {
				Vec2 tc = Vec2(amesh->mTextureCoords[0][i].x,
							   1.0f - amesh->mTextureCoords[0][i].y);
				mesh->texcoords.push_back(tc);
			}
		}

		if(amesh->HasVertexColors(0)) {
			for(unsigned int i=0; i<amesh->mNumVertices; i++) {
				Vec3 color = Vec3(amesh->mColors[0][i].r,
								  amesh->mColors[0][i].g,
								  amesh->mColors[0][i].b);
				mesh->colors.push_back(color);
			}
		}

		for(unsigned int i=0; i<amesh->mNumFaces; i++) {
			for(int j=0; j<3; j++) {
				mesh->indices.push_back(amesh->mFaces[i].mIndices[j]);
			}
		}
		printf(" %d faces\n", amesh->mNumFaces);

		aiColor4D acol;
		aiGetMaterialColor(amtl, AI_MATKEY_COLOR_DIFFUSE, &acol);
		mesh->mtl.diffuse = Vec3(acol.r, acol.g, acol.b);

		float sstr;
		aiGetMaterialFloat(amtl, AI_MATKEY_SHININESS_STRENGTH, &sstr);

		aiGetMaterialColor(amtl, AI_MATKEY_COLOR_SPECULAR, &acol);
		mesh->mtl.specular = sstr * Vec3(acol.r, acol.g, acol.b) * 0.3;

		float shin;
		aiGetMaterialFloat(amtl, AI_MATKEY_SHININESS, &shin);
		mesh->mtl.shininess = shin * 6;

		aiString astr;
		if(aiGetMaterialTexture(amtl, aiTextureType_DIFFUSE, 0, &astr) == 0) {
			char *fname = astr.data;
			char *slash;
			char *path;

			if((slash = strrchr(fname, '/'))) {
				fname = slash + 1;
			}
			if((slash = strrchr(fname, '\\'))) {
				fname = slash + 1;
			}
			path = new char[strlen(fname) + 6];
			sprintf(path, "data/%s", fname);
			if(!(mesh->mtl.tex = img_gltexture_load(path))) {
				fprintf(stderr, "Failed to load texture %s\n", path);
			} else {
				mesh->mtl.tex_opaque = check_tex_opaque(mesh->mtl.tex);
				printf(" texture: %s (%s)\n", path, mesh->mtl.tex_opaque ? "opaque" : "transparent");
			}
			delete [] path;
		}

		meshes.push_back(mesh);
	}

	aiReleaseImport(scene);
	return meshes;
}

void Mesh::calc_bbox()
{
	if (vertices.empty()) {
		bbox.v0 = Vec3(0, 0, 0);
		bbox.v1 = Vec3(0, 0, 0);

		return;
	}

	bbox.v0 = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	bbox.v1 = -bbox.v0;

	for(size_t i=0; i<vertices.size(); i++) {
		for(int j=0; j<3; j++) {
			if(vertices[i][j] < bbox.v0[j])
				bbox.v0[j] = vertices[i][j];
			if(vertices[i][j] > bbox.v1[j])
				bbox.v1[j] = vertices[i][j];
		}
	}
}

static bool check_tex_opaque(unsigned int tex)
{
	int xsz, ysz;
	uint32_t *pixels, *pptr;

	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &xsz);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &ysz);
	assert(xsz > 0 && ysz > 0);

	pptr = pixels = new uint32_t[xsz * ysz];
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	for(int i=0; i<xsz * ysz; i++) {
		if((*pptr++ & 0xff000000) != 0xff000000) {
			delete [] pixels;
			return false;
		}
	}
	delete [] pixels;
	return true;
}
