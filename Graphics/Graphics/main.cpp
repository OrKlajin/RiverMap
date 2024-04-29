#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <vector>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

POINT3 eye = { 2,15,20 };

double sight_angle = PI;
POINT3 sight_dir = { sin(sight_angle),0,cos(sight_angle) }; // in plane X-Z
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
// Global variable to manage update frequency
auto lastUpdate = std::chrono::steady_clock::now();
// Define constants at the top of your file or in a configuration section
const double MIN_TERRAIN_HEIGHT = 0.0; // Example: no terrain should go below sea level, if 0 represents sea level.
const double SEDIMENT_FACTOR = 0.1;    // Example: 10% of the eroded material is deposited per cell downstream.
const double MAX_WATER_HEIGHT = 5.0;   // Example: maximum water height in meters or appropriate units above the ground level.
// Constants for water rendering
const double SOME_THRESHOLD = 0.2;  // Change in water depth at which color changes
const double DEPRESSION_RADIUS = 1.0;  // Visual radius of water puddles
// Color definitions
const GLfloat lightBlue[4] = { 0.4f, 0.7f, 1.0f, 0.8f }; // RGBA for light blue
const GLfloat strongBlue[4] = { 0.0f, 0.2f, 0.8f, 0.8f }; // RGBA for strong blue
const int REDUCTION_FACTOR = 10; // Lower number means more puddles
const double MAX_WATER_LEVEL = 80.0;  // Maximum water level set to 80 meters.
bool isErosionActive = true; // This flag controls whether erosion is happening
double originalHeight[GSZ][GSZ];  // Assuming GSZ is the size of your grid
double someThreshold = 0.05;  // Example value, adjust based on your simulation needs




void UpdateGround();
void Smooth();
bool isAboveSand(double h);
void createRiver(int i, int j, int numRiver);
bool isRiverArea(int i, int j);
bool isDepression(int i, int j);
bool nearDepression(int i, int j);
void findLowestAdjacent(int i, int j, int& di, int& dj);






void setRiverStartingPoints(int i, int j, int numRiver) {
	// Assuming isDepression is adjusted to be less restrictive
	if (isDepression(i, j) || nearDepression(i, j)) { // `nearDepression` is a new hypothetical function
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4d(0, 0.2, 0.5, 0.8);  // Water color
		glBegin(GL_POLYGON);
		glVertex3d(j - GSZ / 2, ground[i][j] + 0.001, i - GSZ / 2);
		glVertex3d(j - GSZ / 2, ground[i - 1][j] + 0.001, i - 1 - GSZ / 2);
		glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1] + 0.001, i - 1 - GSZ / 2);
		glVertex3d(j - 1 - GSZ / 2, ground[i][j - 1] + 0.001, i - GSZ / 2);
		glEnd();
		glDisable(GL_BLEND);
		isRiver[numRiver] = true;
	}
	else {
		isRiver[numRiver] = false;
	}
}

void keepWaterLevel(int i, int j, int numRiver) {
	// Check for valid indices to prevent accessing out of bounds
	if (i <= 0 || j <= 0 || i >= GSZ - 1 || j >= GSZ - 1) {
		return;
	}

	// Only draw water at this spot if it's a depression and part of the river
	if (isRiver[numRiver] && isDepression(i, j)) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4d(0, 0.2, 0.5, 0.8); // Water color
		glBegin(GL_POLYGON);
		glVertex3d(j - GSZ / 2, waterlevel[i][j] + 0.001, i - GSZ / 2);
		glVertex3d(j - GSZ / 2, waterlevel[i - 1][j] + 0.001, i - 1 - GSZ / 2);
		glVertex3d(j - 1 - GSZ / 2, waterlevel[i - 1][j - 1] + 0.001, i - 1 - GSZ / 2);
		glVertex3d(j - 1 - GSZ / 2, waterlevel[i][j - 1] + 0.001, i - GSZ / 2);
		glEnd();
		glDisable(GL_BLEND);
	}
}


void modifyErosion(int i, int j, double erosionRate) {
	if (!isRiverArea(i, j)) return;

	// Increase erosion impact visually by making it lower the terrain more noticeably
	double erosionAmount = erosionRate * (waterlevel[i][j] / MAX_WATER_LEVEL) * 10; // Increase factor for visibility
	ground[i][j] -= erosionAmount; // Erode the current cell more significantly

	// Find the lowest neighboring cell to deposit sediment
	double lowestHeight = ground[i][j];
	int lowestI = i, lowestJ = j;

	for (int di = -1; di <= 1; di++) {
		for (int dj = -1; dj <= 1; dj++) {
			int ni = i + di, nj = j + dj;
			if (ni < 0 || nj < 0 || ni >= GSZ || nj >= GSZ) continue; // Boundary check
			if (ground[ni][nj] < lowestHeight) {
				lowestHeight = ground[ni][nj];
				lowestI = ni;
				lowestJ = nj;
			}
		}
	}

	// Deposit sediment at the lowest adjacent point
	if (lowestI != i || lowestJ != j) {
		ground[lowestI][lowestJ] += erosionAmount * SEDIMENT_FACTOR; // Deposit a portion of the eroded material
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
		//modifyErosion(row, col, numRiver, modifier);
		modifyErosion(row, col, modifier); // Assuming numRiver isn't needed directly in modifyErosion


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

// The FloodFillIterative function should be fine as long as it correctly identifies lower ground.
// No changes needed here unless it's incorrectly identifying lower ground.

void createRiver(int i, int j, int numRiver) {
	// This should only be called if the spot is a valid river location.
	if (isRiver[numRiver] && isDepression(i, j)) {
		FloodFillIterative(i, j, numRiver, ground[i][j]);
	}
}

void init()
{
	//           R     G    B
	glClearColor(0.5, 0.7, 1, 0);// color of window background

	glEnable(GL_DEPTH_TEST);

	int i, j, randx, randz;

	srand(time(0));

	for (i = 0; i < GSZ; i++) {
		for (int j = 0; j < GSZ; j++) {
			originalHeight[i][j] = ground[i][j];  // Store the original height for each cell
		}
	}

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

	for (int i = 0; i < GSZ; i++) {
		for (int j = 0; j < GSZ; j++) {
			if (isRiverArea(i, j)) {  // Assume isRiverArea() is a function that determines if a cell is part of a river
				waterlevel[i][j] = ground[i][j] - 0.1;  // Set water level lower than the ground for river areas
			}
			else {
				waterlevel[i][j] = ground[i][j];  // Elsewhere, water level matches ground level
			}
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
		for (int i = 0;i < GSZ;i++)
			for (int j = 0;j < GSZ;j++)
			{
				if (i < a * j + b) ground[i][j] += delta;
				else ground[i][j] -= delta;
			}
	}


}

void Smooth()
{

	for (int i = 1;i < GSZ - 1;i++)
		for (int j = 1;j < GSZ - 1;j++)
		{
			tmp[i][j] = (ground[i - 1][j - 1] + ground[i - 1][j] + ground[i - 1][j + 1] +
				ground[i][j - 1] + ground[i][j] + ground[i][j + 1] +
				ground[i + 1][j - 1] + ground[i + 1][j] + ground[i + 1][j + 1]) / 9.0;
		}

	for (int i = 1;i < GSZ - 1;i++)
		for (int j = 1;j < GSZ - 1;j++)
			ground[i][j] = tmp[i][j];

}



void SetColor(double h)
{
	 h = fabs(h) / 6; 
		h /= 2;
	// sand
	if (h < 0.03)
		glColor3d(0.8, 0.7, 0.5);
	else	if (h < 0.3)// grass
		glColor3d(0.3 + 0.8 * h, 0.6 - 0.6 * h, 0.2 + 0.2 * h);
	else if (h < 0.9) // stones
		glColor3d(0.4 + 0.1 * h, 0.4 + 0.1 * h, 0.2 + 0.1 * h);
	else // snow
		glColor3d(h, h, 1.1 * h);

}



bool isAboveSand(double h) {
	 h = fabs(h) / 6; 
		if (h >= 0.03)
			return true;
	return false;
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

void DrawFloor() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor3d(0, 0, 0.3);  // Base terrain color

	for (int i = 1; i < GSZ - 1; i++) {
		for (int j = 1; j < GSZ - 1; j++) {
			glBegin(GL_POLYGON);
			SetColor(ground[i][j]);  // Set terrain color based on height
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

	glDisable(GL_BLEND);
}



void DrawTerrainAndWater() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const double SEA_LEVEL = 0.5; // Adjust this to match the actual desired sea level in your terrain
	const GLfloat seaColor[4] = { 0.0f, 0.1f, 0.6f, 0.8f }; // Color for the sea
	const GLfloat lightBlue[4] = { 0.4f, 0.7f, 1.0f, 0.8f }; // Light blue color for shallow water
	const GLfloat strongBlue[4] = { 0.0f, 0.2f, 0.8f, 0.8f }; // Strong blue for deeper water

	// First draw the sea
	for (int i = 0; i < GSZ; i++) {
		for (int j = 0; j < GSZ; j++) {
			if (ground[i][j] <= SEA_LEVEL) {
				glColor4fv(seaColor);
				glBegin(GL_QUADS);
				glVertex3d(j - GSZ / 2, SEA_LEVEL, i - GSZ / 2);
				glVertex3d(j - GSZ / 2, SEA_LEVEL, i + 1 - GSZ / 2);
				glVertex3d(j + 1 - GSZ / 2, SEA_LEVEL, i + 1 - GSZ / 2);
				glVertex3d(j + 1 - GSZ / 2, SEA_LEVEL, i - GSZ / 2);
				glEnd();
			}
		}
	}

	// Then draw puddles on the terrain based on depression and actual water level
	for (int i = 1; i < GSZ - 1; i++) {
		for (int j = 1; j < GSZ - 1; j++) {
			if (isDepression(i, j) && waterlevel[i][j] > ground[i][j]) {
				const GLfloat* color = (waterlevel[i][j] - ground[i][j]) > 0.1 ? strongBlue : lightBlue;
				//drawCircle(j - GSZ / 2, ground[i][j], i - GSZ / 2, 0.1, 60, color);
			}
		}
	}

	glDisable(GL_BLEND);
}





bool isRiverArea(int i, int j) {
	// Check if a specific cell is considered part of a river area
	// This could be a direct check on a grid that marks river cells
	// For now, using a placeholder condition that checks if the water level is explicitly set below ground level
	return waterlevel[i][j] < ground[i][j] - 0.05;
}


bool isDepression(int i, int j) {
	if (i <= 0 || i >= GSZ - 1 || j <= 0 || j >= GSZ - 1) return false; // Bounds check

	double currentHeight = ground[i][j];
	int lowerCount = 0;

	// Check all eight surrounding cells to determine if the current cell is a depression
	for (int di = -1; di <= 1; di++) {
		for (int dj = -1; dj <= 1; dj++) {
			if (di == 0 && dj == 0) continue; // Skip the center cell itself
			int ni = i + di;
			int nj = j + dj;
			if (ni >= 0 && ni < GSZ && nj >= 0 && nj < GSZ) {
				if (ground[ni][nj] > currentHeight) lowerCount++;
			}
		}
	}

	return lowerCount >= 5; // Adjust this threshold based on desired sensitivity
}

bool nearDepression(int i, int j) {
	// Threshold for considering a cell as being near a depression
	const double depressionThreshold = 0.1; // Adjust this value based on your terrain scale and details

	// Check bounds to ensure we do not access out of range elements
	if (i <= 0 || j <= 0 || i >= GSZ - 1 || j >= GSZ - 1) {
		return false;
	}

	// Calculate the average height of surrounding cells
	double avgHeight = 0;
	int count = 0;
	for (int di = -1; di <= 1; di++) {
		for (int dj = -1; dj <= 1; dj++) {
			if (di == 0 && dj == 0) continue; // skip the center cell itself
			int ni = i + di;
			int nj = j + dj;
			if (ni >= 0 && ni < GSZ && nj >= 0 && nj < GSZ) {
				avgHeight += ground[ni][nj];
				count++;
			}
		}
	}

	if (count > 0) {
		avgHeight /= count; // Compute the average height of the surrounding cells
	}

	// Determine if the central cell is lower than the average but within a small threshold
	if (ground[i][j] < avgHeight && (avgHeight - ground[i][j]) <= depressionThreshold) {
		return true; // It's near a depression if it's slightly lower than the average of its neighbors
	}

	return false;
}


void findLowestAdjacent(int i, int j, int& di, int& dj) {
	double min_height = std::numeric_limits<double>::max();
	di = i;
	dj = j;
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			if (dx == 0 && dy == 0) continue;
			int nx = i + dx, ny = j + dy;
			if (nx >= 0 && nx < GSZ && ny >= 0 && ny < GSZ) {
				if (ground[nx][ny] < min_height) {
					min_height = ground[nx][ny];
					di = nx;
					dj = ny;
				}
			}
		}
	}
}


void applyRainDropErosion(int i, int j, double baseErosionRate) {
	if (i <= 0 || i >= GSZ - 1 || j <= 0 || j >= GSZ - 1)
		return; // Boundary condition check
	
	double erosionRate = baseErosionRate * 0.1; // Adjust erosion impact
	double lowestHeight = ground[i][j];
	int lowestI = i, lowestJ = j;

	// Find the lowest neighboring cell
	for (int di = -1; di <= 1; di++) {
		for (int dj = -1; dj <= 1; dj++) {
			int ni = i + di, nj = j + dj;
			if (ni <= 0 || nj <= 0 || ni >= GSZ - 1 || nj >= GSZ - 1)
				continue; // Boundary condition check

			if (ground[ni][nj] < lowestHeight) {
				lowestHeight = ground[ni][nj];
				lowestI = ni;
				lowestJ = nj;
			}
		}
	}

	// Erode the current cell slightly
	ground[i][j] -= erosionRate;
	waterlevel[i][j] += erosionRate;

	// If there's a lower adjacent cell, move water and eroded material there
	if (lowestI != i || lowestJ != j) {
		ground[lowestI][lowestJ] += erosionRate * 0.5; // Deposit half the eroded material
		waterlevel[lowestI][lowestJ] += erosionRate;
	}
	else {
		// Water evaporates and leaves sediment
		ground[i][j] += erosionRate * 0.5; // Sediment deposit
	}
}


void simulateRainfall() {
	for (int i = 0; i < GSZ; i++) {
		for (int j = 0; j < GSZ; j++) {
			if (rand() % 100 < 10) { // 10% chance of rain affecting the cell
				applyRainDropErosion(i, j, 0.001); // Apply erosion with a small rate
			}
		}
	}
}

void updateWaterLevels() {
	// Temporary array to store updated water levels
	double newWaterLevels[GSZ][GSZ] = { 0 };

	for (int i = 1; i < GSZ - 1; i++) {
		for (int j = 1; j < GSZ - 1; j++) {
			double runoff = 0.05 * waterlevel[i][j]; // Calculate runoff as a percentage of current water level
			double totalRunoff = 0; // Total water distributed to lower cells

			// Distribute water to adjacent lower cells
			for (int di = -1; di <= 1; di++) {
				for (int dj = -1; dj <= 1; dj++) {
					int ni = i + di, nj = j + dj;
					if (ni > 0 && ni < GSZ - 1 && nj > 0 && nj < GSZ - 1) {
						if (ground[ni][nj] + waterlevel[ni][nj] < ground[i][j] + waterlevel[i][j]) {
							double flowAmount = runoff / 8; // Equally divide runoff to all lower cells
							newWaterLevels[ni][nj] += flowAmount;
							totalRunoff += flowAmount;
						}
					}
				}
			}

			// Update current cell's water level based on total distributed runoff
			newWaterLevels[i][j] += waterlevel[i][j] - totalRunoff;
		}
	}

	// Copy the new water levels back to the main array
	for (int i = 0; i < GSZ; i++) {
		for (int j = 0; j < GSZ; j++) {
			waterlevel[i][j] = newWaterLevels[i][j];
		}
	}
}


void drawDepressions() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	const double someThreshold = 2; // Define how much lower the ground must be to be considered a depression
	const double depthOffset = 2; // Lower the depression by an additional offset for visual effect

	for (int i = 1; i < GSZ - 1; i++) {
		for (int j = 1; j < GSZ - 1; j++) {
			if (ground[i][j] < originalHeight[i][j] - someThreshold) { // Check if the current ground is significantly lower than the original
				glColor4f(0.05f, 0.15f, 0.35f, 0.9f); // Use a distinct color for depressions
				glBegin(GL_POLYGON);
				glVertex3d(j - GSZ / 2, ground[i][j] - depthOffset, i - GSZ / 2); // Vertex at current position lowered by depthOffset
				glVertex3d(j - GSZ / 2, ground[i - 1][j] - depthOffset, i - 1 - GSZ / 2); // Vertex to the north
				glVertex3d(j - 1 - GSZ / 2, ground[i - 1][j - 1] - depthOffset, i - 1 - GSZ / 2); // Vertex to the northwest
				glVertex3d(j - 1 - GSZ / 2, ground[i][j - 1] - depthOffset, i - GSZ / 2); // Vertex to the west
				glEnd();
			}
		}
	}

	glDisable(GL_BLEND);
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
	
	//updateSimulation(); // Update simulation state
	DrawFloor();    // Draw the terrain
	DrawTerrainAndWater();  // Draw all water features


	for (int i = 0; i < num_rivers; i++) {
		setRiverStartingPoints(xvalues[i], zvalues[i], i);
	}
	for (int i = 0; i < num_rivers; i++) {
		createRiver(xvalues[i], zvalues[i], i);
	}

	glutSwapBuffers(); // show all
}




void idle() {
	// Update the angle for continuous rotation
	angle += 0.1;

	// Update aircraft motion
	air_sight_angle += air_angular_speed;
	air_sight_dir.x = sin(air_sight_angle);
	air_sight_dir.y = sin(-pitch);
	air_sight_dir.z = cos(air_sight_angle);

	aircraft.x += air_speed * air_sight_dir.x;
	aircraft.y += air_speed * air_sight_dir.y;
	aircraft.z += air_speed * air_sight_dir.z;

	// Update camera motion
	sight_angle += angular_speed;
	sight_dir.x = sin(sight_angle);
	sight_dir.z = cos(sight_angle);

	eye.x += speed * sight_dir.x;
	eye.y += speed * sight_dir.y;
	eye.z += speed * sight_dir.z;

	// Check if erosion should be active
	if (isErosionActive) {
		simulateRainfall(); // Simulate rainfall only when erosion is active

		// Increment drop count and adjust erosion if conditions meet
		if (num_drops < 10000)
			num_drops++;
		if (num_drops % 1000 == 0 && modifier < 0.000001) {
			modifier += 0.0000001;
			for (int i = 0; i < num_rivers; i++) {
				modifyErosion(xvalues[i], zvalues[i], modifier);
			}
		}
	}

	// Control the frequency of water level updates
	auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() > 100) { // Update every 100 ms
		updateWaterLevels();
		lastUpdate = now;
	}

	// Always request a redraw
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

void onMouseClick(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		isErosionActive = !isErosionActive; // Toggle erosion activity
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
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("First Example");
	

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutMouseFunc(onMouseClick); 
	glutSpecialFunc(SpecialKeys);
	glutKeyboardFunc(keyboard);

	init();

	glutMainLoop();
}