
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <vector>

using namespace std;

const double PI = 3.14;
const int GSZ = 30;
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

typedef struct {
	double x, y, z;
} POINT3;

POINT3 eye = { 2,15,20 };

double sight_angle = PI;
POINT3 sight_dir = { sin(sight_angle),0,cos(sight_angle) }; // in plane X-Z
double speed = 0;
double angular_speed = 0;
double air_speed = 0;
double air_angular_speed = 0;



// lighting
float lt1amb[4] = { 0.3 ,0.3,0.3, 0 };
float lt1diff[4] = { 0.6 ,0.6,0.6, 0 };
float lt1spec[4] = { 0.8 ,0.8,0.8, 0 };
float lt1pos[4] = { 0.3, 0.5, 0.5, 0 }; 

float lt2amb[4] = { 0.3 ,0.3,0.3, 0 };
float lt2diff[4] = { 0.6 ,0.1,0.6, 0 };
float lt2spec[4] = { 0.8 ,0.8,0.8, 0 };
float lt2pos[4] = { -1 ,1,1, 0 }; 

// yellow material
float mt1amb[4] = { 0.7 ,0.7,0.5, 0 };
float mt1diff[4] = { 0.7 ,0.6,0.2, 0 };
float mt1spec[4] = { 1 ,1,1, 0 };

// blue material
float mt2amb[4] = { 0,0,0.3,0 };
float mt2diff[4] = { 0.3 ,0.6,0.4, 0 };
float mt2spec[4] = { 1 ,1,1, 0 };


double ground[GSZ][GSZ] = { 0 }; // ground tile
double waterlevel[GSZ][GSZ] = { 0 }; // start river tile
double riverWater[GSZ][GSZ] = { 0 }; // river water height
bool isErosionActive = true;
double setHeight = 0.6;

double tmp[GSZ][GSZ];
const int num_rivers = 30; // set number of rivers
bool isRiver[num_rivers];
int xvalues[num_rivers];
int zvalues[num_rivers];
int citycoords[2] = { 0 };
bool cityPlaced = false;

void UpdateGround();
void Smooth();
bool isAboveSea(double h);
void createRiver(int i, int j, int numRiver);
void createRiverFill(int i, int j);
void getCoords(int* coords);
double findMinOfNeighbors(int i, int j);



void setRiverStartingPoints(int i, int j, int numRiver) {


	if (i < 5 || j < 5 || i > GSZ - 5 || j > GSZ - 5) {
		isRiver[numRiver] = false;
		return;
	}


	if (isAboveSea(ground[i][j]) && isAboveSea(ground[i - 1][j]) && isAboveSea(ground[i - 1][j - 1]) && isAboveSea(ground[i][j - 1])) {
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

		isRiver[numRiver] = true;
		return;
	}

	isRiver[numRiver] = false;

}


void modifyErrosion(int i, int j, int numRiver, double modifier) {


	if (isRiver[numRiver] == true && isErosionActive) {
		ground[i][j] -= modifier + 0.001;
		if (waterlevel[i][j] > 0)
			waterlevel[i][j] -= modifier + 0.00001;
		if (findMinOfNeighbors(i, j) != riverWater[i][j])
				riverWater[i][j] += modifier + 0.000001;
	}
}

// uses stack to overcome the recursion
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

double findMaxOfNeighbors(int i, int j) {
	double h = ground[i][j];
	double h1 = fmax(ground[i][j], ground[i][j + 1]);
	double h2 = fmax(ground[i][j], ground[i][j - 1]);
	double h3 = fmax(ground[i][j], ground[i + 1][j]);
	double h4 = fmax(ground[i][j], ground[i + 1][j]);

	if (h1 > h)
		h = h1;
	if (h2 > h)
		h = h2;
	if (h3 > h)
		h = h3;
	if (h4 > h)
		h = h4;

	return h;
}

double findMinOfNeighbors(int i, int j) {
	double h = ground[i][j];
	double h1 = fmin(ground[i][j], ground[i][j + 1]);
	double h2 = fmin(ground[i][j], ground[i][j - 1]);
	double h3 = fmin(ground[i][j], ground[i + 1][j]);
	double h4 = fmin(ground[i][j], ground[i + 1][j]);

	if (h1 < h)
		h = h1;
	if (h2 < h)
		h = h2;
	if (h3 < h)
		h = h3;
	if (h4 < h)
		h = h4;

	return h;
}

void createRiverFill(int i, int j) {
	double h = ground[i][j];

	if (riverWater[i][j] > 0) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4d(0, 0.2, 0.5, 0.8);
		glBegin(GL_POLYGON);
		
			glVertex3d(j - 1 - GSZ / 2, riverWater[i][j - 1], i - GSZ / 2);		
			glVertex3d(j - 1 - GSZ / 2, riverWater[i - 1][j - 1], i - 1 - GSZ / 2);	
			glVertex3d(j - GSZ / 2, riverWater[i - 1][j], i - 1 - GSZ / 2);	
			glVertex3d(j - GSZ / 2, riverWater[i][j], i - GSZ / 2);
		
		glEnd();
		glDisable(GL_BLEND);
	}
}

void giveRiverValues(int i, int j) {
	double h = ground[i][j];

	if (setHeight < h)
		riverWater[i][j] = fmin(setHeight, waterlevel[i][j]) + 0.2;
	else
		riverWater[i][j] = 0;
}

void init()
{
	//           R     G    B
	glClearColor(0.5, 0.7, 1, 0);// color of window background

	glEnable(GL_DEPTH_TEST);

	int i, j, randx, randz;

	srand(time(0));

	for (i = 0; i < 3000; i++)
		UpdateGround();

	Smooth();
	for (i = 0; i < 1000; i++)
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

	for (i = 0; i < GSZ; i++) {
		for (j = 0; j < GSZ; j++) {
			giveRiverValues(i, j);
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



void SetColor(double h)
{
	h /= 2;
	// sand
	if (h < 0.03)
		glColor3d(0.8, 0.7, 0.5);
	else	if (h < 0.3)// grass
		glColor3d(0.3 + 0.8 * h, 0.6 - 0.6 * h, 0.2 + 0.2 * h);
	else if (h < 2.5) // stones
		glColor3d(0.4 + 0.1 * h, 0.4 + 0.1 * h, 0.2 + 0.1 * h);
	else // snow
		glColor3d(h, h, 1.1 * h);

}

bool isAboveSea(double h) {
	if (h > 0)
		return true;
	return false;
}


#pragma warning(push)
#pragma warning(disable:6385)
void DrawFloor()
{
	int i, j;

	glColor3d(0, 0, 0.3);

	for (i = 1; i < GSZ - 1; i++)
		for (j = 1; j < GSZ - 1; j++)
		{
			if (i == citycoords[i] && j == citycoords[j]) {
				// do nothing
			}
			else {
				glBegin(GL_POLYGON);
				SetColor(ground[i][j]);
				glVertex3d(j - GSZ / 2, ground[i][j], i - GSZ / 2);
				SetColor(ground[i - 1][j]);
				glVertex3d(j - GSZ / 2, ground[i - 1][j], i - 1 - GSZ / 2);
				SetColor(ground[i - 1][j - 1]);
				glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1], i - 1 - GSZ / 2);
				SetColor(ground[i][j - 1]);
				glVertex3d(j - 1 - GSZ / 2, ground[i][j - 1], i - GSZ / 2);
				glEnd();
			}		
		}
	//	glDisable(GL_LIGHTING);
		// water + transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.4, 0.7, 0.7);
	glBegin(GL_POLYGON);
	glVertex3d(-GSZ / 2, 0, -GSZ / 2);
	glVertex3d(-GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, -GSZ / 2);
	glEnd();

	glDisable(GL_BLEND);
	//	glEnable(GL_LIGHTING);

}
#pragma warning(pop)

bool checked(int x, int z) {
	if (riverWater[x][z] > 0)
		return true;
	return false;
}

int checkNearestRiver(int x, int z) {

	if (riverWater[x][z] > 0)
		return 0;

	vector <POINT2D> myStack;
	int index = x;
	int oldx = x, oldz = z;
	POINT2D current = { x, z };
	myStack.push_back(current);

	bool checkedUp = false;
	bool checkedDown = false;
	bool checkedRight = false;
	bool checkedLeft = false;

	while (!myStack.empty())
	{
		current = myStack.back();
		myStack.pop_back();

		x = current.x;  // save current point coordinates
		z = current.z;

		if (x + 1 == GSZ)
			checkedUp = true;
		if (x - 1 < 0)
			checkedDown = true;
		if (z + 1 == GSZ)
			checkedRight = true;
		if (z - 1 < 0)
			checkedLeft = true;

		

		// try going up
		if (!checkedUp) {
			if (riverWater[x + 1][z] > 0 && ground[x + 1][z] < setHeight)
			{
				return fabs((oldx + 1 - index) * GSZ);
			}
			else {
				index++;
				current.x = x + 1;
				current.z = z;
				myStack.push_back(current);
				goto RiverLoop;
			}
		}
		// try going down
		if (!checkedDown) {
			if (riverWater[x - 1][z] > 0 && ground[x - 1][z] < setHeight)
			{
				return fabs((oldx - 1 - index) * GSZ);
			}
			else {
				index++;
				current.x = x - 1;
				current.z = z;
				myStack.push_back(current);
				goto RiverLoop;
			}
		}	
		// try going right
		if (!checkedRight) {
			if (riverWater[x][z + 1] > 0 && ground[x][z + 1] < setHeight)
			{
				return fabs((oldx - index) * GSZ + oldz);
			}
			else {
				index++;
				current.x = x;
				current.z = z + 1;
				myStack.push_back(current);
				goto RiverLoop;
			}
		}
		// try going left
		if (!checkedLeft) {
			if (riverWater[x][z - 1] > 0 && ground[x][z - 1] < setHeight)
			{
				return fabs((oldx - index) * GSZ - oldz);
			}
			else {
				index++;
				current.x = x;
				current.z = z + 1;
				myStack.push_back(current);
				goto RiverLoop;
			}
		}	
	RiverLoop:;
	}

	return 0;
}

int checkNearestSea(int x, int z) {

	if (waterlevel[x][z] == 0)
		return 0;

	vector <POINT2D> myStack;
	int index = x;
	int oldx = x, oldz = z;
	POINT2D current = { x, z };
	myStack.push_back(current);

	bool checkedUp = false;
	bool checkedDown = false;
	bool checkedRight = false;
	bool checkedLeft = false;

	while (!myStack.empty())
	{
		current = myStack.back();
		myStack.pop_back();

		x = current.x;  // save current point coordinates
		z = current.z;

		if (x + 1 == GSZ)
			checkedUp = true;
		if (x - 1 < 0)
			checkedDown = true;
		if (z + 1 == GSZ)
			checkedRight = true;
		if (z - 1 < 0)
			checkedLeft = true;

		// try going up
		if (!checkedUp) {
			if (waterlevel[x + 1][z] < 0.3)
			{
				return fabs((oldx + 1 - index) * GSZ);
			}
			else {
				index++;
				current.x = x + 1;
				current.z = z;
				myStack.push_back(current);
				goto SeaLoop;
			}
		}
		// try going down
		if (!checkedDown) {
			if (waterlevel[x - 1][z] < 0.3)
			{
				return fabs((oldx - 1 - index) * GSZ);
			}
			else {
				index++;
				current.x = x - 1;
				current.z = z;
				myStack.push_back(current);
				goto SeaLoop;
			}
		}
		// try going right
		if (!checkedRight) {
			if (waterlevel[x][z + 1] < 0.3)
			{
				return fabs((oldx - index) * GSZ + oldz);
			}
			else {
				index++;
				current.x = x;
				current.z = z + 1;
				myStack.push_back(current);
				goto SeaLoop;
			}
		}
		// try going left
		if (!checkedLeft) {
			if (waterlevel[x][z - 1] < 0.3)
			{
				return fabs((oldx - index) * GSZ - oldz);
			}
			else {
				index++;
				current.x = x;
				current.z = z + 1;
				myStack.push_back(current);
				goto SeaLoop;
			}
		}
		SeaLoop:;
	}
	return 0;
}


bool isValidCitySpot(int row, int col) {

	if (ground[row][col] <= 0 && ground[row][col] <= setHeight) {
		return false;
	}

	if (setHeight > ground[row][col] && checkNearestRiver(row, col) > (GSZ * 3) && checkNearestSea(row, col) >= (GSZ * 2))
	{
		return true;
	}

	return false;
}



void drawCity(int i, int j) {

	if (i < 5 || j < 5 || i > GSZ - 5 || j > GSZ - 5)
		return;

	// flatten ground first
	double h = findMinOfNeighbors(i, j);
	glBegin(GL_POLYGON);
	SetColor(h);

	glVertex3d(j - GSZ / 2, h, i - GSZ / 2);
	glVertex3d(j - GSZ / 2, h, i - 2 - GSZ / 2);
	glVertex3d(j - 2 - GSZ / 2, h, i - 2 - GSZ / 2);
	glVertex3d(j - 2 - GSZ / 2, h, i - GSZ / 2);
	

	glEnd();


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

	int dist = 2;

	
	// draw building cube with windows
		glPushMatrix();
		glTranslated(j - GSZ / 2, ground[i][j], i - GSZ / 2);
		glRotated(45, 0, 1, 0);
		glutSolidCube(0.8);
		glTranslated(0, 0.6, 0);
		glutSolidCube(0.8);
		glPopMatrix();

		// material
		glMaterialfv(GL_FRONT, GL_AMBIENT, mt2amb);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mt2diff);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mt2spec);
		glMaterialf(GL_FRONT, GL_SHININESS, 50);
		// draw cone for roof
		glPushMatrix();
		glTranslated(j - GSZ / 2, ground[i][j] + 0.9, i - GSZ / 2);
		glRotated(-90, 1, 0, 0);
		glutSolidCone(0.8, 0.5, 4, 20);
		glPopMatrix();

		if (ground[i][j + dist] > 0 && checkNearestRiver(i, j + dist) > 0) {
			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt1amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt1diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt1spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 100);

			glPushMatrix();
			glTranslated(j - GSZ / 2 + dist, findMaxOfNeighbors(i, j + dist), i - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glutSolidCube(0.8);
			glTranslated(0, 0.6, 0);
			glutSolidCube(0.8);
			glPopMatrix();

			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt2amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt2diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt2spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 50);
			// draw cone for roof
			glPushMatrix();
			glTranslated(j - GSZ / 2 + dist, findMaxOfNeighbors(i, j + dist) + 0.9, i - GSZ / 2);
			glRotated(-90, 1, 0, 0);
			glutSolidCone(0.8, 0.5, 4, 20);
			glPopMatrix();
		}
		
		if (ground[i][j - dist] > 0 && checkNearestRiver(i, j - dist) > 0) {
			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt1amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt1diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt1spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 100);

			glPushMatrix();
			glTranslated(j - GSZ / 2 - dist, findMaxOfNeighbors(i, j - dist), i - GSZ / 2);
			glRotated(45, 0, 1, 0);
			glutSolidCube(0.8);
			glTranslated(0, 0.6, 0);
			glutSolidCube(0.8);
			glPopMatrix();

			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt2amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt2diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt2spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 50);
			// draw cone for roof
			glPushMatrix();
			glTranslated(j - GSZ / 2 - dist, findMaxOfNeighbors(i, j - dist) + 0.9, i - GSZ / 2);
			glRotated(-90, 1, 0, 0);
			glutSolidCone(0.8, 0.5, 4, 20);
			glPopMatrix();
		}
		
		if (ground[i + dist][j] > 0 && checkNearestRiver(i + dist, j) > 0) {
			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt1amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt1diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt1spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 100);

			glPushMatrix();
			glTranslated(j - GSZ / 2, findMaxOfNeighbors(i + dist, j), i - GSZ / 2 + dist);
			glRotated(45, 0, 1, 0);
			glutSolidCube(0.8);
			glTranslated(0, 0.6, 0);
			glutSolidCube(0.8);
			/*drawWindows(i , j); *///windows needs changing
			glPopMatrix();

			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt2amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt2diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt2spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 50);
			// draw cone for roof
			glPushMatrix();
			glTranslated(j - GSZ / 2, findMaxOfNeighbors(i + dist, j) + 0.9, i - GSZ / 2 + dist);
			glRotated(-90, 1, 0, 0);
			glutSolidCone(0.8, 0.5, 4, 20);
			glPopMatrix();
		}

		if (ground[i - dist][j] > 0 && checkNearestRiver(i - dist, j) > 0) {
			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt1amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt1diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt1spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 100);

			glPushMatrix();
			glTranslated(j - GSZ / 2, findMaxOfNeighbors(i - dist, j), i - GSZ / 2 - dist);
			glRotated(45, 0, 1, 0);
			glutSolidCube(0.8);
			glTranslated(0, 0.6, 0);
			glutSolidCube(0.8);
			glPopMatrix();

			// material
			glMaterialfv(GL_FRONT, GL_AMBIENT, mt2amb);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mt2diff);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mt2spec);
			glMaterialf(GL_FRONT, GL_SHININESS, 50);
			// draw cone for roof
			glPushMatrix();
			glTranslated(j - GSZ / 2, findMaxOfNeighbors(i - dist, j) + 0.9, i - GSZ / 2 - dist);
			glRotated(-90, 1, 0, 0);
			glutSolidCone(0.8, 0.5, 4, 20);
			glPopMatrix();
		}

	glDisable(GL_LIGHTING);
}

int checkSurroundings(int i, int j) {
	int sum = 1;
	if (ground[i + 2][j] > 0 && checkNearestRiver(i + 2, j) > 0)
		sum++;
	if (ground[i - 2][j] > 0 && checkNearestRiver(i - 2, j) > 0)
		sum++;
	if (ground[i][j + 2] > 0 && checkNearestRiver(i, j + 2) > 0)
		sum++;
	if (ground[i][j - 2] > 0 && checkNearestRiver(i, j - 2) > 0)
		sum++;

	return sum;
}

void getCoords(int* coords) {

	int num = 0;

	for (int i = 5; i < GSZ - 5; i++) {
		for (int j = 5; j < GSZ - 5; j++) {
			if (isValidCitySpot(i, j) == true) {
				num = checkSurroundings(i, j);
				if (num > 2) { // min buildings is at least 3
					printf("City placed at %d %d\n", i, j);
					coords[0] = i;
					coords[1] = j;
					return;
				}

			}		
		}
	}
}

// put all the drawings here
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


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // unity matrix of model

	

	DrawFloor();
	for (int i = 0; i < num_rivers; i++) {
		setRiverStartingPoints(xvalues[i], zvalues[i], i);
	}
	for (int i = 0; i < num_rivers; i++) {
		createRiver(xvalues[i], zvalues[i], i);
	}
	for (int i = 5; i < GSZ - 5; i++) {
		for (int j = 5; j < GSZ - 5; j++) {
			createRiverFill(i, j);
		}
	}
	
	if (cityPlaced == false) {
		getCoords(citycoords);
		cityPlaced = true;
	}
	
	drawCity(citycoords[0], citycoords[1]);


	

	glutSwapBuffers(); // show all
}


void idle()
{
	int i, j;
	angle += 0.1;



	// ego-motion  or locomotion
	sight_angle += angular_speed;
	// the direction of our sight (forward)
	sight_dir.x = sin(sight_angle);

	sight_dir.z = cos(sight_angle);
	// motion
	eye.x += speed * sight_dir.x;
	eye.y += speed * sight_dir.y;
	eye.z += speed * sight_dir.z;

	if (modifier < 1)
		modifier += 0.00000000001;
	

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

void onMouseClick(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		isErosionActive = !isErosionActive;
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
	glutSpecialFunc(SpecialKeys);
	glutMouseFunc(onMouseClick);
	glutKeyboardFunc(keyboard);

	init();

	glutMainLoop();
}
