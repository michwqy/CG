#include <stdlib.h>
#include "glut.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "simplexnoise.h"


#define PI 3.1415926535;
//由于glut中并没有三维向量，所以自己定义了一个
class vector3 {
public:
	float x;
	float y;
	float z;
	vector3(float a, float b, float c) {
		x = a;
		y = b;
		z = c;
	}
	vector3(const vector3& a) {
		x = a.x;
		y = a.y;
		z = a.z;
	}
	vector3() {
		x = 0;
		y = 0;
		z = 0;
	}
	void operator=(const vector3& a)
	{
		x = a.x;
		y = a.y;
		z = a.z;
	}
	vector3 operator*(const float& a) {
		vector3 b;
		b.x = a * x;
		b.y = a * y;
		b.z = a * z;
		return b;
	}
	float operator*(const vector3& a) {
		float b;
		b = a.x * x + a.y * y + a.z * z;
		return b;
	}
	vector3 operator+(const vector3& a) {
		vector3 b;
		b.x = a.x + x;
		b.y = a.y + y;
		b.z = a.z + z;
		return b;
	}

	float size() {
		return sqrt(x * x + y * y + z * z);
	}
	vector3 unitize() {
		x *= (1 / size());
		y *= (1 / size());
		z *= (1 / size());
		return *this;
	}
};

//计算两个向量之间的夹角，返回角度值
float calang(vector3 a, vector3 b) {
	float t;
	t=(a* b)/ a.size() / b.size();
	t = acos(t) * 180 / PI;
	return t;
}
//一些常数
float eye[] = { 5, 0, 0 };
float center[] = { 0, 0, 0};
float up[] = { 0, 0, 1 };

GLfloat white[] = { 1,1,1, 1 };
GLfloat light_pos[] = { 0, 20, 10, 0 };

static const float HEIGHT_MAX = 0.0550f;//地形起伏的范围
static const float HEIGHT_MIN = -0.0575f;

bool isrotate = false;
float frotate = 0.5;
float anrg = 0;

//由于glut自带的球不能修改坐标，所以自己用立方体映射实现了一个球
class sphere {
public:
	int cut;
	float rad;
	vector3* vexy1;//立方体的六个面的坐标，每个面一个数组
	vector3* vexy2;
	vector3* veyz1;
	vector3* veyz2;
	vector3* vexz1;
	vector3* vexz2;
	vector3* cxy1;//立方体的六个面的颜色值
	vector3* cxy2;
	vector3* cyz1;
	vector3* cyz2;
	vector3* cxz1;
	vector3* cxz2;

	sphere(int n, float r) {
		rad = r;
		cut = n;
		vexy1 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		vexy2 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		veyz1 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		veyz2 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		vexz1 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		vexz2 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		cxy1 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		cxy2 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		cyz1 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		cyz2 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		cxz1 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		cxz2 = (vector3*)malloc(sizeof(vector3) * cut * cut);
		//set_seed(1);
	}
	void cubecal();
	void cubedraw();
	void facedraw(vector3* a, vector3* c);
};

sphere s = sphere(300, 1);//初始化球

void map(vector3& position) {//将立方体表面坐标映射成球体表面坐标
	float x2 = position.x * position.x;
	float y2 = position.y * position.y;
	float z2 = position.z * position.z;
	position.x = position.x * sqrt(1.0f - (y2 * 0.5f) - (z2 * 0.5f) + ((y2 * z2) / 3.0f));
	position.y = position.y * sqrt(1.0f - (z2 * 0.5f) - (x2 * 0.5f) + ((z2 * x2) / 3.0f));
	position.z = position.z * sqrt(1.0f - (x2 * 0.5f) - (y2 * 0.5f) + ((x2 * y2) / 3.0f));
}

vector3 calcolor(float a) {//计算该高度变化下应该表示的颜色
	vector3 color;
	if (a < 0.0) {//小于0为海，绝对值越大颜色越深
		a = fabs(a);
		color.x = color.y = 0.1;
		float i = (1 - a / fabs(HEIGHT_MIN));
		if (i < 0.05) {//防止颜色过深
			i = 0.05;
		}
		color.z = i * 0.5;//缩小颜色范围，改善效果
	}
	else {//大于0为陆地，绝对值越大颜色越深
		a = fabs(a);
		color.x = color.z = 0.1;
		float i = (1 - a / fabs(HEIGHT_MAX));
		if (i < 0.05) {
			i = 0.05;
		}
		color.y = i * 0.3;
	}
	return color;
}

float calheight(vector3& position) {//通过噪音函数生成高度扰动

	vector3 vNormalFromCenter = position;

	static const float NOISE_PERSISTENCE = 0.6f;
	static const float NOISE_OCTAVES = 8.0f;
	static const float NOISE_SCALE = 1.0f;

	float fNoise = scaled_octave_noise_3d(NOISE_OCTAVES, NOISE_PERSISTENCE, NOISE_SCALE, HEIGHT_MIN, HEIGHT_MAX, position.x, position.y, position.z);
	if (fNoise >= 0) {
		position = position + vNormalFromCenter * fNoise;
	}
	return fNoise;

}

void sphere::cubecal() {//计算球体各坐标和颜色值
	vector3 minv(-1.0, -1.0, -1.0);
	for (int i = 0; i < cut; i++) {
		for (int j = 0; j < cut; j++) {
			vector3 position = minv;
			position.x += (float)j / (float)(cut - 1) * 2.0f;
			position.y += (float)i / (float)(cut - 1) * 2.0f;
			map(position);
			position = position * rad;
			cxy1[i * cut + j] = calcolor(calheight(position));
			vexy1[i * cut + j] = position;
		}
	}
	minv = vector3(-1.0, -1.0, 1.0);
	for (int y = 0; y < cut; y++) {
		for (int x = 0; x < cut; x++) {
			vector3 position = minv;
			position.x += (float)x / (float)(cut - 1) * 2.0f;
			position.y += (float)y / (float)(cut - 1) * 2.0f;
			map(position);
			position = position * rad;
			cxy2[y * cut + x] = calcolor(calheight(position));
			vexy2[y * cut + x] = position;
		}
	}
	minv = vector3(-1.0, -1.0, -1.0);
	for (int i = 0; i < cut; i++) {
		for (int j = 0; j < cut; j++) {
			vector3 position = minv;
			position.y += (float)j / (float)(cut - 1) * 2.0f;
			position.z += (float)i / (float)(cut - 1) * 2.0f;
			map(position);
			position = position * rad;
			cyz1[i * cut + j] = calcolor(calheight(position));
			veyz1[i * cut + j] = position;
		}
	}
	minv = vector3(1.0, -1.0, -1.0);
	for (int i = 0; i < cut; i++) {
		for (int j = 0; j < cut; j++) {
			vector3 position = minv;
			position.y += (float)j / (float)(cut - 1) * 2.0f;
			position.z += (float)i / (float)(cut - 1) * 2.0f;
			map(position);
			position = position * rad;
			cyz2[i * cut + j] = calcolor(calheight(position));
			veyz2[i * cut + j] = position;
		}
	}
	minv = vector3(-1.0, -1.0, -1.0);
	for (int i = 0; i < cut; i++) {
		for (int j = 0; j < cut; j++) {
			vector3 position = minv;
			position.x += (float)j / (float)(cut - 1) * 2.0f;
			position.z += (float)i / (float)(cut - 1) * 2.0f;
			map(position);
			position = position * rad;
			cxz1[i * cut + j] = calcolor(calheight(position));
			vexz1[i * cut + j] = position;
		}
	}
	minv = vector3(-1.0, 1.0, -1.0);
	for (int i = 0; i < cut; i++) {
		for (int j = 0; j < cut; j++) {
			vector3 position = minv;
			position.x += (float)j / (float)(cut - 1) * 2.0f;
			position.z += (float)i / (float)(cut - 1) * 2.0f;
			map(position);
			position = position * rad;
			cxz2[i * cut + j] = calcolor(calheight(position));
			vexz2[i * cut + j] = position;
		}
	}
}

void sphere::cubedraw() {//画球体
	facedraw(vexy1, cxy1);
	facedraw(vexy2, cxy2);
	facedraw(veyz1, cyz1);
	facedraw(veyz2, cyz2);
	facedraw(vexz1, cxz1);
	facedraw(vexz2, cxz2);
}

void sphere::facedraw(vector3* a, vector3* c) {//画每个面
	vector3* vex = a;
	vector3 view = vector3(eye[0] - center[0], eye[1] - center[1], eye[2] - center[2]);
	float color[4] = { 0,0,0,1 };
	float ang;
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	for (int y = 0; y < cut - 1; y++) {
		for (int x = 0; x < cut - 1; x++) {
			color[0] = c[y * cut + x].x;//画球体表面
			color[1] = c[y * cut + x].y;
			color[2] = c[y * cut + x].z;
			vector3 v1 = vex[y * cut + x];
			vector3 v2 = vex[y * cut + (x + 1) % cut];
			vector3 v3 = vex[(y + 1) % cut * cut + (x + 1) % cut];
			vector3 v4 = vex[(y + 1) % cut * cut + x % cut];
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
			glMaterialf(GL_FRONT, GL_SHININESS, 10.0);
			glBegin(GL_QUADS);
			glNormal3f(v1.x, v1.y, v1.z);
			glVertex3f(v1.x, v1.y, v1.z);
			glNormal3f(v2.x, v2.y, v2.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glNormal3f(v3.x, v3.y, v3.z);
			glVertex3f(v3.x, v3.y, v3.z);
			glNormal3f(v4.x, v4.y, v4.z);
			glVertex3f(v4.x, v4.y, v4.z);
			glEnd();

			v1 = v1.unitize()*rad* 1.025;//画遮罩，改善颜色表现效果，但没有大气的光晕效果
			v2 = v2.unitize() * rad * 1.025;
			v3 = v3.unitize() * rad * 1.025;
			v4 = v4.unitize() * rad * 1.025;
			color[0] = c[y * cut + x].x + 0.2;
			color[1] = c[y * cut + x].y + 0.2;
			color[2] = c[y * cut + x].z + 0.2;
			color[3] = 0.1;
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
			//glMaterialf(GL_FRONT, GL_SHININESS, 10.0);
			glBegin(GL_QUADS);
			glNormal3f(v1.x, v1.y, v1.z);
			glVertex3f(v1.x, v1.y, v1.z);
			glNormal3f(v2.x, v2.y, v2.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glNormal3f(v3.x, v3.y, v3.z);
			glVertex3f(v3.x, v3.y, v3.z);
			glNormal3f(v4.x, v4.y, v4.z);
			glVertex3f(v4.x, v4.y, v4.z);
			glEnd();
	
		}
	}
}

void draw() {


	GLUquadric* qobj = gluNewQuadric();
	gluQuadricTexture(qobj, GL_TRUE);
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glPushMatrix();
	glRotatef(.0, 0, 0, 0);
	gluSphere(qobj, 1, 500, 1000);
	glPopMatrix();
	gluDeleteQuadric(qobj);

}

void reshape(int width, int height)
{
	if (height == 0)										// Prevent A Divide By Zero By
	{
		height = 1;										// Making Height Equal One
	}

	glViewport(0, 0, width, height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	float whRatio = (GLfloat)width / (GLfloat)height;
	gluPerspective(45, whRatio, 1, 1000);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
}

void idle()
{
	glutPostRedisplay();
}

void key(unsigned char k, int x, int y)
{
	switch (k)
	{
	case 27:
	case ' ':
		isrotate = !isrotate;
		break;
	default: break;
	}
}


void redraw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 1);
	glLoadIdentity();									// Reset The Current Modelview Matrix

	gluLookAt(eye[0], eye[1], eye[2],
		center[0], center[1], center[2],
		up[0], up[1], up[2]);

	//glEnable(GLUT_MULTISAMPLE);//开启抗锯齿，发现没用
	/*
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	*/
	/*
	glEnable(GL_FOG);//开启雾，也能达到相似的遮罩效果
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR,white);
	glFogf(GL_FOG_DENSITY, 0.5);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_START, eye[0]-1);
	glFogf(GL_FOG_END, eye[0]+1);
	*/

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	float ambient[] = { 0.05,0.05,0.05,1 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);
	glEnable(GL_LIGHT0);

	glRotatef(anrg, 0, 0, 1);
	if (isrotate) {
		anrg += frotate;
		if (anrg > 360) anrg -= 360;
	}
	s.cubedraw();

	glutSwapBuffers();
}

void init() {
	s.cubecal();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(720, 720);
	int windowHandle = glutCreateWindow("test");
	init();
	glutDisplayFunc(redraw);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutIdleFunc(idle);
	glutMainLoop();
	return 0;
}


