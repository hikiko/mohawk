#include <GL/glew.h>
#include <GL/glut.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "mesh.h"
#include "hair.h"

#define MAX_NUM_SPAWNS 4
#define THRESH 0.5

static bool init();
static void cleanup();
static void display();
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);

static std::vector<Mesh*> meshes;
static Mesh *mesh_head;
static Hair hair;

static int win_width, win_height;
static float cam_theta, cam_phi = 25, cam_dist = 8;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("hair test");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	if(!init()) {
		return 1;
	}
	atexit(cleanup);

	glutMainLoop();
	return 0;
}

static bool init()
{
	glewInit();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glClearColor(0.5, 0.5, 0.5, 1);
	meshes = load_meshes("data/head.fbx");
	if (meshes.empty()) {
		fprintf(stderr, "Failed to load mesh.\n");
		return false;
	}

	for(size_t i=0; i<meshes.size(); i++) {
		meshes[i]->calc_bbox();
/*
		Vec3 v0 = meshes[i]->bbox.v0;
		Vec3 v1 = meshes[i]->bbox.v1;

		printf("mesh: %s\n", meshes[i]->name.c_str());
		printf("AABB mesh %d: v0: (%f, %f, %f) v1: (%f, %f, %f)\n",
				(int)i, v0.x, v0.y, v0.z, v1.x, v1.y, v1.z);
*/
		meshes[i]->update_vbo(MESH_ALL);
/*
		printf("num vertices: %d num triangles: %d\n",
				(int)meshes[i]->vertices.size(),
				(int)meshes[i]->indices.size() / 3);
*/
		if(meshes[i]->name == "head") {
			mesh_head = meshes[i];
		}
	}
	if(!mesh_head) {
		fprintf(stderr, "Failed to find the head mesh.\n");
		return false;
	}

	if(!hair.init(mesh_head, MAX_NUM_SPAWNS, THRESH)) {
		fprintf(stderr, "Failed to initialize hair\n");
		return false;
	}

	return true;
}

static void cleanup()
{
	for(size_t i=0; i<meshes.size(); i++) {
		delete meshes[i];
	}
}

static void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);

	for(size_t i=0; i<meshes.size(); i++) {
		meshes[i]->draw();
	}

	hair.draw();

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

static void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	win_width = x;
	win_height = y;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, (float)x / (float)y, 0.5, 500.0);
}

static void keydown(unsigned char key, int /*x*/, int /*y*/)
{
	switch(key) {
	case 27:
		exit(0);
	}
}

bool bnstate[8];
int prev_x, prev_y;

static void mouse(int bn, int st, int x, int y)
{
	bnstate[bn] = st == GLUT_DOWN;
	prev_x = x;
	prev_y = y;
}

static void motion(int x, int y)
{
	int dx = x - prev_x;
	int dy = y - prev_y;
	prev_x = x;
	prev_y = y;

	if(!dx && !dy) return;

	if(bnstate[0]) {
		cam_theta += dx * 0.5;
		cam_phi += dy * 0.5;

		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
		glutPostRedisplay();
	}
	if(bnstate[2]) {
		cam_dist += dy * 0.1;
		if(cam_dist < 0) cam_dist = 0;
		glutPostRedisplay();
	}
}
