#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"

double pitch1 = 0; // slider position
double pitch2 = 0; // slider position
double pitch3 = 0; // slider position
bool isCaptured1 = false; // is slider 1 captured by mouse
bool isCaptured2 = false; // is slider 2 captured by mouse
bool isCaptured3 = false; // is slider 3 captured by mouse
int H = 600;
int W = 600;

int slider1H;
int slider2H;
int slider3H;

double air_speed = 0;
double air_angular_speed = 0;

const double PI = 3.14;
const int GSZ = 100;

double angle = 0;

typedef struct {
	double x, y, z;
} POINT3;

POINT3 eye = { 0,25,25 };

double sight_angle = PI;
POINT3 sight_dir = { sin(sight_angle),0,cos(sight_angle) }; // in plane X-Z
double speed = 0;
double angular_speed = 0;

double ground[GSZ][GSZ] = { 0 };
double tmp[GSZ][GSZ];

int num_windows = 3;
int num_floors = 2;

unsigned int letters;

char floorText = '0' + num_floors;
char windowText = '0' + num_windows;

// lighting
float lt1amb[4] = { 0.3 ,0.3,0.3, 0 };
float lt1diff[4] = { 0.6 ,0.6,0.6, 0 };
float lt1spec[4] = { 0.8 ,0.8,0.8, 0 };
float lt1pos[4] = { 0.3, 0.5, 0.5, 0}; // if the last parameter is 0 the lighting is directional 
// if the last parameter is 1 the lighting is positional

float lt2amb[4] = { 0.3 ,0.3,0.3, 0 };
float lt2diff[4] = { 0.6 ,0.1,0.6, 0 };
float lt2spec[4] = { 0.8 ,0.8,0.8, 0 };
float lt2pos[4] = { -1 ,1,1, 0 }; // if the last parameter is 0 the lighting is directional 
// if the last parameter is 1 thelighting is positional

// yellow material
float mt1amb[4] = { 0.7 ,0.7,0.5, 0 };
float mt1diff[4] = { 0.7 ,0.6,0.2, 0 };
float mt1spec[4] = { 1 ,1,1, 0 };

// blue material
float mt2amb[4] = { 0,0,0.3,0 };
float mt2diff[4] = { 0.3 ,0.6,0.4, 0 };
float mt2spec[4] = { 1 ,1,1, 0 };

// black material
float mt3amb[4] = { 0,0,0.1,0 };
float mt3diff[4] = { 0, 0 ,0 , 0 };
float mt3spec[4] = { 0 ,0,0.1, 0 };

// green material
float mt4amb[4] = { 0,0.4,0.1,0 };
float mt4diff[4] = { 0, 0 ,0 , 0 };
float mt4spec[4] = { 0 ,0.2,0.1, 0 };


void UpdateGround3();
void Smooth();

void initFont()
{
	HDC hdc = wglGetCurrentDC();

	HFONT hf;
	GLYPHMETRICSFLOAT gm[128];
	LOGFONT lf;

	lf.lfHeight = 25;
	lf.lfWidth = 0;
	lf.lfWeight = FW_BOLD;
	lf.lfEscapement = 0;
	lf.lfItalic = false;
	lf.lfUnderline = false;
	lf.lfCharSet = DEFAULT_CHARSET; // can be hebrew
	strcpy((char*)lf.lfFaceName, "Arial");

	hf = CreateFontIndirect(&lf);
	SelectObject(hdc, hf);

	letters = glGenLists(128);
	wglUseFontOutlines(hdc, 0, 128, letters, 0, 0.2, WGL_FONT_POLYGONS, gm);
}

void init()
{
	glClearColor(0,0.5,0.8,0);// color of window background

	glEnable(GL_DEPTH_TEST);

	initFont();
}

// rows are along Z-axis, cols are along x-axis
void SetNormal(int row, int col)
{
	double nx, ny, nz;

	nx = ground[row][col] - ground[row][col + 1];
	ny = 1;
	nz = ground[row][col] - ground[row + 1][col];

	glNormal3d(nx, ny, nz);
}

void drawWindows() {
	double zlocation = 4.6;
	int middleWindow, middleFloor;
	double xlocation, ylocation;
	// material
	glMaterialfv(GL_FRONT, GL_AMBIENT, mt3amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mt3diff);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mt3spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 0);

	for (int j = 0; j < num_floors; j++) {
		if (num_floors % 2 == 0) {
			if (j % 2 == 0)
				ylocation = -(j + 1);
			else
				ylocation = (j + 1);
		}
		else {
			middleFloor = (num_floors / 2);
			if (j == middleFloor)
				ylocation = 0;
			else if (j < middleFloor)
				ylocation = -(j * middleFloor + 3);
			else
				ylocation = (j * 2 - middleFloor * 2 + 1);
		}

		glPushMatrix();
		glTranslated(0, ylocation, zlocation);
		for (int i = 0; i < num_windows; i++) {
			if (num_windows % 2 == 0) {
				if (i % 2 == 0)
					xlocation = -(i + 0.5);
				else
					xlocation = (i + 0.5);
			}
			else {
				middleWindow = (num_windows / 2);
				if (i == middleWindow)
					xlocation = 0;
				else if (i < middleWindow)
					xlocation = -(i * middleWindow + 2);
				else
					xlocation = (i * 2 - middleWindow * 2);
			}

			// right
			glPushMatrix();
			glTranslated(xlocation, 0, 0);
			glutSolidCube(1);
			glTranslated(0, -1, 0);
			glutSolidCube(1);
			glTranslated(0, 1, 0);
			glPopMatrix();
		}
		glTranslated(0, 0, -2 * zlocation);

		for (int i = 0; i < num_windows; i++) {
			if (num_windows % 2 == 0) {
				if (i % 2 == 0)
					xlocation = -(i + 0.5);
				else
					xlocation = (i + 0.5);
			}
			else {
				middleWindow = (num_windows / 2);
				if (i == middleWindow)
					xlocation = 0;
				else if (i < middleWindow)
					xlocation = -(i * middleWindow + 2);
				else
					xlocation = (i * 2 - middleWindow * 2);
			}
			// left
			glPushMatrix();
			glTranslated(xlocation, 0, 0);
			glutSolidCube(1);
			glTranslated(0, -1, 0);
			glutSolidCube(1);
			glTranslated(0, 1, 0);
			glPopMatrix();
		}
		glTranslated(-zlocation, 0, zlocation);

		for (int i = 0; i < num_windows; i++) {
			if (num_windows % 2 == 0) {
				if (i % 2 == 0)
					xlocation = -(i + 0.5);
				else
					xlocation = (i + 0.5);
			}
			else {
				middleWindow = (num_windows / 2);
				if (i == middleWindow)
					xlocation = 0;
				else if (i < middleWindow)
					xlocation = -(i * middleWindow + 2);
				else
					xlocation = (i * 2 - middleWindow * 2);
			}
			// front
			glPushMatrix();
			glTranslated(0, 0, xlocation);
			glutSolidCube(1);
			glTranslated(0, -1, 0);
			glutSolidCube(1);
			glTranslated(0, 1, 0);
			glPopMatrix();
		}
		glTranslated(2 * zlocation, 0, 0);

		for (int i = 0; i < num_windows; i++) {
			if (num_windows % 2 == 0) {
				if (i % 2 == 0)
					xlocation = -(i + 0.5);
				else
					xlocation = (i + 0.5);
			}
			else {
				middleWindow = (num_windows / 2);
				if (i == middleWindow)
					xlocation = 0;
				else if (i < middleWindow)
					xlocation = -(i * middleWindow + 2);
				else
					xlocation = (i * 2 - middleWindow * 2);
			}
			// back
			glPushMatrix();
			glTranslated(0, 0, xlocation);
			glutSolidCube(1);
			glTranslated(0, -1, 0);
			glutSolidCube(1);
			glTranslated(0, 1, 0);
			glPopMatrix();
		}

		glPopMatrix();


	}
	
}

void SetColor(double h)
{
	h = fabs(h) / 6;
	// sand
	if (h < 0.02)
		glColor3d(0.8, 0.7, 0.5);
	else	if (h < 0.3)// grass
		glColor3d(0.4 + 0.8 * h, 0.6 - 0.6 * h, 0.2 + 0.2 * h);
	else if (h < 0.9) // stones
		glColor3d(0.4 + 0.1 * h, 0.4 + 0.1 * h, 0.2 + 0.1 * h);
	else // snow
		glColor3d(h, h, 1.1 * h);

}

void UpdateGround3()
{
	double delta = 0.04;
	if (rand() % 2 == 0)
		delta = -delta;
	int x1, y1, x2, y2;
	x1 = rand() % GSZ;
	y1 = rand() % GSZ;
	x2 = rand() % GSZ;
	y2 = rand() % GSZ;
	double a, b;
	if (x1 != x2)
	{
		a = (y2 - y1) / ((double)x2 - x1);
		b = y1 - a * x1;
		for (int i = 0; i < GSZ; i++)
			for (int j = 0; j < GSZ; j++)
			{
				if (i < a * j + b) ground[i][j] += delta;
				else ground[i][j] -= delta;
			}
	}
}

void Smooth()
{

	for (int i = 1; i < GSZ - 1; i++)
		for (int j = 1; j < GSZ - 1; j++)
		{
			tmp[i][j] = (ground[i - 1][j - 1] + ground[i - 1][j] + ground[i - 1][j + 1] +
				ground[i][j - 1] + ground[i][j] + ground[i][j + 1] +
				ground[i + 1][j - 1] + ground[i + 1][j] + ground[i + 1][j + 1]) / 9.0;
		}

	for (int i = 1; i < GSZ - 1; i++)
		for (int j = 1; j < GSZ - 1; j++)
			ground[i][j] = tmp[i][j];

}

void DrawFloor()
{
	int i, j;

	/*glColor3d(0, 0, 0.3);*/
	// material
	glMaterialfv(GL_FRONT, GL_AMBIENT, mt4amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mt4diff);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mt4spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 0);

	for (i = 1; i < GSZ; i++)
		for (j = 1; j < GSZ; j++)
		{
			//			glBegin(GL_LINE_LOOP);
			glBegin(GL_POLYGON);
			/*SetColor(ground[i][j]);*/
			glVertex3d(j - GSZ / 2, ground[i][j], i - GSZ / 2);
			/*SetColor(ground[i - 1][j]);*/
			glVertex3d(j - GSZ / 2, ground[i - 1][j], i - 1 - GSZ / 2);
			/*SetColor(ground[i - 1][j - 1]);*/
			glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1], i - 1 - GSZ / 2);
			/*SetColor(ground[i][j - 1]);*/
			glVertex3d(j - 1 - GSZ / 2, ground[i][j - 1], i - GSZ / 2);
			glEnd();
		}
}

void DrawBackGround() {

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4d(0, 0, 0, 0.4); // transparent black

	glBegin(GL_POLYGON);
	glVertex2d(0, 0);
	glVertex2d(100, 0);
	glVertex2d(100, 600);
	glVertex2d(0, 600);
	glEnd();

	glDisable(GL_BLEND);

}

int DrawSlider(int x, int y, double pitch)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4d(0, 0, 0, 0.6); // transparent black
	glBegin(GL_POLYGON);
	glVertex2d(x, y);
	glVertex2d(x, 100 + y);
	glVertex2d(100 + x, 100 + y);
	glVertex2d(100 + x, y);
	glEnd();
	glDisable(GL_BLEND);

	glBegin(GL_LINES);
	glVertex2d(5 + x, 50 + y);
	glVertex2d(95 + x, 50 + y);
	glEnd();

	// button
	glColor3d(0, 0.5, 0);
	double w = 100 * (1 + pitch) / 2;
	glBegin(GL_POLYGON);
	glVertex2d(w - 8 + x, y + 42);
	glVertex2d(w - 8 + x, y + 58);
	glVertex2d(w + 8 + x, y + 58);
	glVertex2d(w + 8 + x, y + 42);
	glEnd();

	return y;


}

void updateColorOfRoof(double pitch) {
	mt2amb[0] = (pitch + 1) * 2 / 5; // red
	mt2amb[1] = (pitch + 1) * 2 / 25; // green
	mt2amb[2] = 0.3 + (pitch + 1) * 2 / 5; // blue
}

bool MoveSliderPitch(int sliderH, double pitch, int x, int y)
{
	double w = 100 * (1 + pitch) / 2; // middle of the slider button

	if (H - y > 42 + sliderH && H - y < 58 + sliderH && x > w - 8 && x < w + 8)
		return true;

	return false;
}

void ShowText(char* text)
{
	glListBase(letters);
	glCallLists(1, GL_UNSIGNED_BYTE, text);
}


void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clean frame buffer and Z-buffer
	glViewport(0, 0, W, H);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, eye.y, eye.z,  // eye position
		eye.x + sight_dir.x, eye.y - 0.3, eye.z + sight_dir.z,  // sight dir
		0, 1, 0);


	// Building

	// cube
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	

	// add lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	glLightfv(GL_LIGHT0, GL_AMBIENT, lt1amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lt1diff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lt1spec);
	glLightfv(GL_LIGHT0, GL_POSITION, lt1pos);


	// material
	glMaterialfv(GL_FRONT, GL_AMBIENT, mt1amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mt1diff);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mt1spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 100);

	// draw building cube with windows
	glPushMatrix();
	glTranslated(0, 15, 0);
	glRotated(45, 0, 1, 0);
	glutSolidCube(10);
	drawWindows();
	glPopMatrix();

	// material
	glMaterialfv(GL_FRONT, GL_AMBIENT, mt2amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mt2diff);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mt2spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 50);

	// draw cone for roof
	glPushMatrix();
	glTranslated(0, 20, 0);
	glRotated(-90, 1, 0, 0);
	glutSolidCone(8, 4, 4, 20);
	glPopMatrix();

	// draw floor
	glPushMatrix();
	glTranslated(0, 10, 0);
	glScaled(2, 1, 2);
	DrawFloor();
	glPopMatrix();

	glDisable(GL_LIGHTING);

	// pitch sliders
	glViewport(0, 0, 200, 600);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection
	glOrtho(0, 200, 0, 600, -1, 1);  // 2d
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of projection
	glDisable(GL_DEPTH_TEST);
	DrawBackGround();

	// draw text

	char text1[100] = "Roof Color";
	char text2[100] = "Number of Floors";
	char text3[100] = "Number of Windows";


	glColor3d(1, 1, 1);
	
	for (int i = 0; i < strlen(text1); i++)
	{
		glPushMatrix();
		glScaled(16, 16, 0);
		glTranslated(i - 0.4*i + 0.3, 34, 0);
		if (i >= 8)
			glTranslated(-0.4,0,0);
		ShowText(text1 + i);
		glPopMatrix();
	}

	for (int i = 0; i < strlen(text2); i++)
	{
		glPushMatrix();
		glScaled(16, 16, 0);
		if (i < 10)
			glTranslated(i - 0.4 * i + 0.3, 23, 0);
		else
			glTranslated(0.4 * (i - 10) + 1.7, 22, 0);
		if (i == 11)
			glTranslated(0.1, 0, 0);
		ShowText(text2 + i);
		glPopMatrix();
	}
	glPushMatrix();
	glScaled(25, 25, 0);
	glTranslated(1.75, 13, 0);
	ShowText(&floorText);
	glPopMatrix();

	for (int i = 0; i < strlen(text3); i++)
	{
		glPushMatrix();
		glScaled(16, 16, 0);
		if (i < 10)
			glTranslated(i - 0.4 * i + 0.3, 10, 0);
		else
			glTranslated(0.5 * (i - 10) + 1.2, 9, 0);
		if (i == 11)
			glTranslated(0.23, 0, 0);	
		ShowText(text3 + i);
		glPopMatrix();
	}
	glPushMatrix();
	glScaled(25, 25, 0);
	glTranslated(1.75, 4.8, 0);
	ShowText(&windowText);
	glPopMatrix();

	// draw sliders
	slider1H = DrawSlider(0,400,pitch1);
	slider2H = DrawSlider(0,200,pitch2);
	slider3H = DrawSlider(0,0,pitch3);
	glEnable(GL_DEPTH_TEST);

	glutSwapBuffers(); // show all
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
		air_speed += 0.01;
		break;
	case 's':
		air_speed -= 0.01;
		break;
	case 'a':
		air_angular_speed += 0.001; // yaw
		break;
	case 'd':
		air_angular_speed -= 0.001; // yaw
		break;
	}
}

void SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT: // turns the user to the left
		angular_speed += 0.0004;
		break;
	case GLUT_KEY_RIGHT:
		angular_speed -= 0.0004;
		break;
	case GLUT_KEY_UP: // increases the speed
		speed += 0.005;
		break;
	case GLUT_KEY_DOWN:
		speed -= 0.005;
		break;
	case GLUT_KEY_PAGE_UP:
		eye.y += 0.1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		eye.y -= 0.1;
		break;

	}
}

void idle() 
{
	angle += 0.02;

	// ego-motion  or locomotion
	sight_angle += angular_speed;
	// the direction of our sight (forward)
	sight_dir.x = sin(sight_angle);
	sight_dir.z = cos(sight_angle);
	// motion
	eye.x += speed * sight_dir.x;
	eye.y += speed * sight_dir.y;
	eye.z += speed * sight_dir.z;

	glutPostRedisplay();
}


void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		isCaptured1 = MoveSliderPitch(slider1H, pitch1, x, y);
		isCaptured2 = MoveSliderPitch(slider2H, pitch2, x, y);
		isCaptured3 = MoveSliderPitch(slider3H, pitch3, x, y);

	}
	else 	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		isCaptured1 = false; // release mouse
		isCaptured2 = false; 
		isCaptured3 = false;
	}
		


}

void mouseMotion(int x, int y)
{
	// updates pitch, pitch is from -1 to 1
	if (isCaptured1) // slider 1 to change color of the roof
	{
		pitch1 = (2 * x / 100.0) - 1;
		if (pitch1 > 0.85)
			pitch1 = 0.85;
		else if (pitch1 < -0.85)
			pitch1 = -0.85;
		updateColorOfRoof(pitch1);
		
	}
	else if (isCaptured2)
	{
		pitch2 = (2 * x / 100.0) - 1;
		if (pitch2 > 0.85)
			pitch2 = 0.85;
		else if (pitch2 < -0.85)
			pitch2 = -0.85;

		if (pitch2 <= 1 && pitch2 >= 0.3)
			num_floors = 3;
		else if (pitch2 < 0.3 && pitch2 >= -0.3)
			num_floors = 2;
		else if (pitch2 < -0.3 && pitch2 >= -1)
			num_floors = 1;

		drawWindows();

		floorText = '0' + num_floors;
		glPushMatrix();
		glScaled(25, 25, 0);
		glTranslated(1.75, 13, 0);
		ShowText(&floorText);
		glPopMatrix();
	}
	else if (isCaptured3)
	{
		pitch3 = (2 * x / 100.0) - 1;
		if (pitch3 > 0.85)
			pitch3 = 0.85;
		else if (pitch3 < -0.85)
			pitch3 = -0.85;

		if (pitch3 <= 1 && pitch3 >= 0.6)
			num_windows = 5;
		else if (pitch3 < 0.6 && pitch3 >= 0.2)
			num_windows = 4;
		else if (pitch3 < 0.2 && pitch3 >= -0.2)
			num_windows = 3;
		else if (pitch3 < -0.2 && pitch3 >= -0.6)
			num_windows = 2;
		else if (pitch3 < -0.6 && pitch3 >= -1)
			num_windows = 1;

		drawWindows();

		windowText = '0' + num_windows;
		glPushMatrix();
		glScaled(25, 25, 0);
		glTranslated(1.75, 4.8, 0);
		ShowText(&windowText);
		glPopMatrix();
	}
}


void main(int argc, char* argv[]) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("First Example");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(SpecialKeys);

	init();

	glutMainLoop();
}