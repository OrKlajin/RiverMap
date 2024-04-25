#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <vector>

using namespace std;

const double PI = 3.14;
const int GSZ = 50;
const int H = 600;
const int W = 600;

typedef struct {
	int x;
	int z;
} POINT2D;

// Texture definitions

const int TW = 512; // must be a power of 2
const int TH = 512;

double angle = 0;
double modifier = 0;

unsigned char tx0[TH][TW][3]; // RGB


typedef struct {
	double x, y, z;
} POINT3;

POINT3 eye = {2,15,20 };

double sight_angle = PI;
POINT3 sight_dir = {sin(sight_angle),0,cos(sight_angle)}; // in plane X-Z
double speed = 0;
double angular_speed = 0;

// aircraft defs
double air_sight_angle = PI;
POINT3 air_sight_dir = { sin(air_sight_angle),0,cos(air_sight_angle) }; // in plane X-Z
double air_speed = 0;
double air_angular_speed = 0;
POINT3 aircraft = { 0,15,0 };
double pitch = 0;



double ground[GSZ][GSZ] = { 0 };
double waterlevel[GSZ][GSZ] = { 0 };

double tmp[GSZ][GSZ];
const int num_rivers = 100;
int num_drops = 0;
bool isRiver[num_rivers];
int xvalues[num_rivers];
int zvalues[num_rivers];

void UpdateGround();
void Smooth();
bool isAboveSand(double h);
void createRiver(int i, int j, int numRiver);



void setRiverStartingPoints(int i, int j, int numRiver) {
	

	if (i < 5 || j < 5 || i > GSZ - 5 || j > GSZ - 5) {
		isRiver[numRiver] = false;
		return;
	}
		

	if (isAboveSand(ground[i][j]) && isAboveSand(ground[i-1][j]) && isAboveSand(ground[i-1][j-1]) && isAboveSand(ground[i][j-1])) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4d(0, 0.2, 0.5, 0.8);
		glBegin(GL_POLYGON);
		
		glVertex3d(j - GSZ / 2, ground[i][j] + 0.001, i - GSZ / 2);	
		glVertex3d(j - GSZ / 2, ground[i - 1][j] + 0.001, i - 1 - GSZ / 2);
		glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1] + 0.001, i - 1 - GSZ / 2);
		glVertex3d(j - 1 - GSZ / 2, ground[i][j - 1] + 0.001, i - GSZ / 2);

		glEnd();
		glDisable(GL_BLEND);

		isRiver[numRiver] = true;
		return;
	}

	isRiver[numRiver] = false;

}

void keepWaterLevel(int i, int j, int numRiver) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.2, 0.5, 0.8);
	glBegin(GL_POLYGON);

	glVertex3d(j - GSZ / 2, waterlevel[i][j] + 0.001, i - GSZ / 2);
	glVertex3d(j - GSZ / 2, waterlevel[i - 1][j] + 0.001, i - 1 - GSZ / 2);
	glVertex3d(j - 1 - GSZ / 2, waterlevel[i - 1][j - 1] + 0.001, i - 1 - GSZ / 2);
	glVertex3d(j - 1 - GSZ / 2, waterlevel[i][j - 1] + 0.001, i - GSZ / 2);

	glEnd();
	glDisable(GL_BLEND);
}

void modifyErrosion(int i, int j, int numRiver, double modifier) {
	

	if (isRiver[numRiver] == true) {
		ground[i][j] -= modifier + 0.001;
	}
}

// uses stack to uvercome the recursion
void FloodFillIterative(int row, int col, int numRiver, double y)
{
	vector <POINT2D> myStack;
	bool moved;

	POINT2D current = { row,col };
	myStack.push_back(current);

	while (!myStack.empty())
	{
		moved = false;
		current = myStack.back();
		myStack.pop_back();
		
		row = current.x;  // save current point coordinates
		col = current.z;
		
		setRiverStartingPoints(row, col, numRiver);
		modifyErrosion(row, col, numRiver, modifier);
		
		
		// try going up
		if (ground[row + 1][col] < y)
		{
			current.x = row + 1;
			current.z = col;
			y = ground[current.x][current.z];
			moved = true;
		}
		// try going down
		if (ground[row - 1][col] < y)
		{
			current.x = row - 1;
			current.z = col;
			y = ground[current.x][current.z];
			moved = true;
		}
		// try going right
		if (ground[row][col + 1] < y)
		{
			current.x = row;
			current.z = col + 1;
			y = ground[current.x][current.z];
			moved = true;
		}
		// try going left
		if (ground[row][col - 1] < y)
		{
			current.x = row;
			current.z = col - 1;
			y = ground[current.x][current.z];
			moved = true;
		}
		// if we moved, push the current point on the stack
		if (moved)
			myStack.push_back(current);
		

	}
}

void createRiver(int i, int j, int numRiver) {
	double y = ground[i][j];

	if (isRiver[numRiver] == true) {
		FloodFillIterative(i, j, numRiver, y);	
	}
}

void init()
{
	//           R     G    B
	glClearColor(0.5,0.7,1,0);// color of window background

	glEnable(GL_DEPTH_TEST);

	int i, j, randx, randz;

	srand(time(0));

	for (i = 0;i < 3000;i++)
		UpdateGround();

	Smooth();
	for (i = 0;i < 1000;i++)
		UpdateGround();

	
	for (i = 0; i < num_rivers; i++) {
		randx = rand() % GSZ;
		randz = rand() % GSZ;

		xvalues[i] = randx;
		zvalues[i] = randz;
	}

	for (i = 0; i < GSZ; i++) {
		for (j = 0; j < GSZ; j++) {
			waterlevel[i][j] = ground[i][j];
		}
	}
	
}


void UpdateGround()
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
		for(int i=0;i<GSZ;i++)
			for (int j = 0;j < GSZ;j++)
			{
				if (i < a * j + b) ground[i][j] += delta;
				else ground[i][j] -= delta;
			}
	}


}

void Smooth()
{

	for(int i=1;i<GSZ-1;i++)
		for (int j = 1;j < GSZ - 1;j++)
		{
			tmp[i][j] = (ground[i-1][j-1]+ ground[i-1 ][j]+ ground[i - 1][j + 1]+
				ground[i][j - 1] + ground[i ][j] + ground[i ][j + 1]+
				ground[i + 1][j - 1] + ground[i + 1][j] + ground[i + 1][j + 1]) / 9.0;
		}

	for (int i = 1;i < GSZ - 1;i++)
		for (int j = 1;j < GSZ - 1;j++)
			ground[i][j] = tmp[i][j];

}



void SetColor(double h)
{
	/*h = fabs(h)/6;*/
	h /= 2;
	// sand
	if (h < 0.03)
		glColor3d(0.8, 0.7, 0.5);
	else	if(h<0.3)// grass
	glColor3d(0.3+0.8*h,0.6-0.6*h,0.2+ 0.2 * h);
	else if(h<0.9) // stones
		glColor3d(0.4 + 0.1 * h,  0.4+0.1*h, 0.2 + 0.1 * h);
	else // snow
		glColor3d(  h,  h, 1.1 * h);

}

bool isAboveSand(double h) {
	/*h = fabs(h) / 6;*/
	if (h >= 0.03)
		return true;
	return false;
}

// rows are along Z-axis, cols are along x-axis
void SetNormal(int row,int col)
{
	double nx, ny, nz;

	nx = ground[row][col] - ground[row][col + 1];
	ny = 1;
	nz = ground[row][col] - ground[row + 1][col];

	glNormal3d(nx, ny, nz);
}

void DrawFloor()
{
	int i,j;

	glColor3d(0, 0, 0.3);

	for(i=1;i<GSZ-1;i++)
		for (j = 1;j < GSZ-1;j++)
		{
			glBegin(GL_POLYGON);
			SetColor(ground[i][j]);
//			SetNormal(i, j);
			glVertex3d(j-GSZ/2, ground[i][j], i-GSZ/2);
			SetColor(ground[i-1][j]);
//			SetNormal(i-1, j);
			glVertex3d(j - GSZ / 2, ground[i - 1][j], i - 1 - GSZ / 2);
			SetColor(ground[i-1][j-1]);
//			SetNormal(i-1, j-1);
			glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1], i - 1 - GSZ / 2);
			SetColor(ground[i][j-1]);
//			SetNormal(i, j-1);
			glVertex3d(j - 1 - GSZ / 2, ground[i ][j - 1], i - GSZ / 2);
			glEnd();
		}
//	glDisable(GL_LIGHTING);
	// water + transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.4, 0.7,0.7);
	glBegin(GL_POLYGON);
	glVertex3d(-GSZ / 2, 0, -GSZ / 2);
	glVertex3d(-GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, -GSZ / 2);
	glEnd();

	glDisable(GL_BLEND);
//	glEnable(GL_LIGHTING);

}

// put all the drawings here
void display()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // clean frame buffer and Z-buffer
	glViewport(0, 0, W, H);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // unity matrix of projection

	glFrustum(-1, 1, -1, 1, 0.75, 300);
	gluLookAt(eye.x, eye.y, eye.z,  // eye position
		eye.x+ sight_dir.x, eye.y-0.3, eye.z+sight_dir.z,  // sight dir
		0, 1, 0);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	DrawFloor();
	for (int i = 0; i < num_rivers; i++) {
		setRiverStartingPoints(xvalues[i], zvalues[i], i);
	}
	for (int i = 0; i < num_rivers; i++) {
		createRiver(xvalues[i], zvalues[i], i);
	}

	glutSwapBuffers(); // show all
}


void idle() 
{
	int i, j;
	double dist;
	angle += 0.1;

	// aircraft motion
	air_sight_angle += air_angular_speed;

	air_sight_dir.x = sin(air_sight_angle);
	air_sight_dir.y = sin(-pitch);
	air_sight_dir.z = cos(air_sight_angle);

	aircraft.x += air_speed * air_sight_dir.x;
	aircraft.y += air_speed * air_sight_dir.y;
	aircraft.z += air_speed * air_sight_dir.z;

	// ego-motion  or locomotion
	sight_angle += angular_speed;
	// the direction of our sight (forward)
	sight_dir.x = sin(sight_angle);

	sight_dir.z = cos(sight_angle);
	// motion
	eye.x += speed * sight_dir.x;
	eye.y += speed * sight_dir.y;
	eye.z += speed * sight_dir.z;

	if (num_drops < 10000)
		num_drops++;
	if (num_drops % 1000 == 0) {
		if (modifier < 0.000001)
			modifier += 0.0000001;
		for (int i = 0; i < num_rivers; i++) {
			/*modifyErrosion(xvalues[i], zvalues[i], i, modifier);*/
		}
	}
	

	glutPostRedisplay();
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
		speed+= 0.005;
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

void main(int argc, char* argv[]) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("First Example");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutSpecialFunc(SpecialKeys);
	glutKeyboardFunc(keyboard);
	
	init();

	glutMainLoop();
}