#include <GL/glew.h>
#include <GL/glut.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include <gmath/gmath.h>

#include "mesh.h"
#include "hair.h"
#include "object.h"

#define MAX_NUM_SPAWNS 800
#define THRESH 0.5

static bool init();
static void cleanup();
static void display();
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);
static void idle();

static unsigned int gen_grad_tex(int sz, const Vec3 &c0, const Vec3 &c1);
static void draw_text(const char *text, int x, int y, float sz, const Vec3 &color);

static std::vector<Mesh*> meshes;
static Mesh *mesh_head;
static Hair hair;

static unsigned int grad_tex;

static int win_width, win_height;
static float cam_theta, cam_phi = 25, cam_dist = 8;
static float head_rz, head_rx; /* rot angles x, z axis */
static Mat4 head_xform;
//static CollSphere coll_sphere; /* sphere used for collision detection */

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("hair test");

	/* for the keydown, keyup functions to work */
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutIdleFunc(idle);

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

	grad_tex = gen_grad_tex(32, Vec3(0, 0, 1), Vec3(0, 1, 0));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
//	glEnable(GL_COLOR_MATERIAL);

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

//	coll_sphere.radius = 1.0;
//	coll_sphere.center = Vec3(0, 0.6, 0.53);

	if(!hair.init(mesh_head, MAX_NUM_SPAWNS, THRESH)) {
		fprintf(stderr, "Failed to initialize hair\n");
		return false;
	}

//	hair.add_collider(&coll_sphere);

	return true;
}

static void cleanup()
{
	for(size_t i=0; i<meshes.size(); i++) {
		delete meshes[i];
	}
	glDeleteTextures(1, &grad_tex);
}

static void display()
{
	static unsigned long prev_time;
	unsigned long msec = glutGet(GLUT_ELAPSED_TIME);
	float dt = (float)(msec - prev_time) / 1000.0;
	prev_time = msec;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	head_xform = Mat4::identity;
	head_xform.rotate_x(gph::deg_to_rad(head_rx));
	head_xform.rotate_z(-gph::deg_to_rad(head_rz));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);
	/* multiplying with the head rot matrix */
	glPushMatrix();
	glMultMatrixf(head_xform[0]);
/*
	glPushAttrib(GL_LINE_BIT);
	glLineWidth(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
*/
	for(size_t i=0; i<meshes.size(); i++) {
		if(!meshes[i]->mtl.tex || meshes[i]->mtl.tex_opaque)
			meshes[i]->draw();
	}
	for(size_t i=0; i<meshes.size(); i++) {
		if(meshes[i]->mtl.tex && !meshes[i]->mtl.tex_opaque)
			meshes[i]->draw();
	}
/*
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopAttrib();
*/

	glPopMatrix();

	hair.set_transform(head_xform);
	hair.update(dt);
	hair.draw();

/*
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glBegin(GL_POINTS);
	for (int i=0; i<500; i++) {
		Vec3 p;
		p.x = (float)rand() / RAND_MAX * 8 - 4;
		p.y = (float)rand() / RAND_MAX * 4;
		p.z = 0;

		Vec3 tmp = inverse(head_xform) * p;
		if(coll_sphere.contains(tmp)) {
			glColor3f(1, 0, 0);
		}
		else glColor3f(0, 1, 0);

		glVertex3f(p.x, p.y, p.z);
	}
	glEnd();
	glPopAttrib();
*/
	float plane[4] = {
		0, 0, 0.5 / 350, 0.5
	};

	glPushMatrix();
	glRotatef(90, 1, 0, 0);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, grad_tex);
	glFrontFace(GL_CW);
	glEnable(GL_TEXTURE_GEN_S);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, plane);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glColor3f(1, 1, 1);

	glDepthMask(0);

	glutSolidSphere(350, 16, 8);
	glDisable(GL_TEXTURE_1D);

	glColor3f(0.2, 0.2, 0.2);
	glutWireSphere(350, 32, 16);

	glDepthMask(1);
	glFrontFace(GL_CCW);
	glPopAttrib();

	glPopMatrix();

	draw_text("Hold h to move the head with the mouse!", 15, 15, 0.0015 * win_width, Vec3(0, 0, 0));
	draw_text("Hold h to move the head with the mouse!", 12, 17, 0.0015 * win_width, Vec3(0.8, 0.5, 0.7));

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

static bool hpressed;
static void keydown(unsigned char key, int /*x*/, int /*y*/)
{
	switch(key) {
	case 'h':
	case 'H':
		hpressed = true;
		break;
	case 27:
		exit(0);
	default:
		break;
	}
}

static void keyup(unsigned char key, int /*x*/, int /*y*/)
{
	switch(key) {
	case 'h':
	case 'H':
		hpressed = false;
		break;
	default:
		break;
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

	if(hpressed) {
		if(bnstate[0]) {
			head_rz += dx * 0.5;
			head_rx += dy * 0.5;

			if(head_rx < -45) head_rx = -45;
			if(head_rx > 45) head_rx = 45;

			if(head_rz < -90) head_rz = -90;
			if(head_rz > 90) head_rz = 30;
		}
	}
	else {
		if(bnstate[0]) {
			cam_theta += dx * 0.5;
			cam_phi += dy * 0.5;

			if(cam_phi < -90) cam_phi = -90;
			if(cam_phi > 90) cam_phi = 90;
		}
		if(bnstate[2]) {
			cam_dist += dy * 0.1;
			if(cam_dist < 0) cam_dist = 0;
		}
	}
}

static void idle()
{
	glutPostRedisplay();
}

static unsigned int gen_grad_tex(int sz, const Vec3 &c0, const Vec3 &c1)
{
	unsigned char *pixels = new unsigned char[sz * 3];
	for(int i=0; i<sz; i++) {
		float t = (float)i / (float)(sz - 1);
		Vec3 color = c0 + (c1 - c0) * t;
		pixels[i * 3] = color.x * 255;
		pixels[i * 3 + 1] = color.y * 255;
		pixels[i * 3 + 2] = color.z * 255;
	}

	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_1D, tex);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, sz, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	delete [] pixels;

	return tex;
}

static void draw_text(const char *text, int x, int y, float sz, const Vec3 &color)
{
	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glLineWidth(3);
	glPointSize(3);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, win_width, 0, win_height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(x, y, 0);
	glScalef(0.1 * sz, 0.1 * sz, 1);

	glColor3f(color.x, color.y, color.z);
	while(*text != '\0') {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *text);
		text++;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();
}
