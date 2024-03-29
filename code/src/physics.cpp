#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include "glm\glm.hpp"
#include <iostream>
#include <time.h>
#include <glm/gtx/intersect.hpp>
#include <math.h>

#define PI (3.141592653589793)

bool show_test_window = false;

const int clothMeshLines(18);
const int clothMeshColumns(14);
float particleInitRest;

bool pause = false;
bool reset_ = false;

float t = 0.f;					//time
float tRestart = 15.f;			//time to restart

//Gerstner Waves variables

const int nWaves = 3;			//number of waves

glm::vec3 origin = { -4.5f, 9.f, -3.f };	//x0
glm::vec3* position;						//x, y, z
glm::vec3* lastPosition;					//
glm::vec3* firstPosition;
float sumX;
float sumY;

float A[3];					//amplitud
float O[3];					//frequency
float L[3];					//lambda, distance between wave crests
glm::vec3 K = { 1, 0, 0 };	//wave vector
float k[3];					//wave lenght, k = 2*pi/L
float p[3];					//phase of the wave

//Bouyancy Forces

float d = 0.997f;					//p, density of the fluid
glm::vec3 g;						//g, gravity
float g_[] = { 0.f, -9.81f, 0.f };
float V;							//Vsub
float m = 1.f;						//mass

glm::vec3 a;	//sphere acceleration
glm::vec3 v;

glm::vec3 F;	//sphere total forces
glm::vec3 Fb;	//F buoyancy
glm::vec3 Fa;	//accelaration force

glm::vec3 sphPos;	//sphere position
float radius = 1.f;	//sphere radius

float dist = 0;		//dist between wave and ball
float h;		

float auxX, distX = 0;
int lineCloser;

//Drag Force
glm::vec3 Fd;					//drag force
glm::vec3 U;					//fluid material
float Area;						//relative area
float AreaR;					//relative area radius
const float Cd = 0.47f;			//drag coeficient sphere

//Namespaces
namespace Sphere {
	extern void setupSphere(glm::vec3 pos = glm::vec3(0.f, 1.f, 0.f), float radius = 1.f);
	extern void cleanupSphere();
	extern void updateSphere(glm::vec3 pos, float radius = 1.f);
	extern void drawSphere();
}

namespace ClothMesh {
	extern void setupClothMesh();
	extern void cleanupClothMesh();
	extern void updateClothMesh(float* array_data);
	extern void drawClothMesh();
}

int getPosInArray(int fila, int columna) {
	return columna + clothMeshColumns * fila;
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
	}

	ImGui::Checkbox("Pause", &pause);
	if (ImGui::Button("Reset simulation")) { reset_ = true; }
	ImGui::DragFloat("Reset Time", &tRestart, 0.1f);
	ImGui::DragFloat3("Gravity", g_, 0.1f);
	ImGui::DragFloat("Sphere Mass", &m, 0.1f);
	
	ImGui::End();

	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

void reset()
{
	A[0] = 0.25f;					A[1] = 0.5f;					A[2] = 0.75f;
	O[0] = 0.75;					O[1] = 1.;						O[2] = 1.25;
	L[0] = 0.2f;					L[1] = 0.5f;					L[2] = 0.8f;
	k[0] = 2.f * PI / L[0];			k[1] = 2.f * PI / L[1];			k[2] = 2.f * PI / L[2];
	p[0] = 0.5f;					p[1] = 1.2f;					p[2] = 0.8f;

	Fb = a = v = F = Fa = { 0.f, 0.f, 0.f };

	sphPos = glm::vec3{
		-4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (8))),
		2 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (2))),
		-4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (8))),
	};

	particleInitRest = 9.f / clothMeshLines;

	for (int i = 0; i < clothMeshLines; i++) {

		for (int j = 0; j < clothMeshColumns; j++) {

			position[getPosInArray(i, j)]	   = glm::vec3(i * particleInitRest, 0.f, j * particleInitRest) + origin;
			firstPosition[getPosInArray(i, j)] = glm::vec3(i * particleInitRest, 0.f, j * particleInitRest) + origin;

		}
	}

	t = 0.f;
	reset_ = false;
}

void PhysicsInit() {

	position	  = new glm::vec3[clothMeshLines*clothMeshColumns];
	lastPosition  = new glm::vec3[clothMeshLines*clothMeshColumns];
	firstPosition = new glm::vec3[clothMeshLines*clothMeshColumns];
	
	A[0] = 0.25f;					A[1] = 0.5f;					A[2] = 0.75f;
	O[0] = 0.75;					O[1] = 1.;						O[2] = 1.25;
	L[0] = 0.2f;					L[1] = 0.5f;					L[2] = 0.8f;
	k[0] = 2.f * PI / L[0];			k[1] = 2.f * PI / L[1];			k[2] = 2.f * PI / L[2];
	p[0] = 0.5f;					p[1] = 1.2f;					p[2] = 0.8f;

	Fb = a = v = F = Fa = { 0.f, 0.f, 0.f };

	sphPos = glm::vec3{
		-4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (8))),
		2 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (2))),
		-4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (8))),
	};

	particleInitRest = 9.f / clothMeshLines;

	for (int i = 0; i < clothMeshLines; i++) {

		for (int j = 0; j < clothMeshColumns; j++) {

			position[getPosInArray(i, j)]      = glm::vec3(i * particleInitRest, 0.f, j * particleInitRest) + origin;
			firstPosition[getPosInArray(i, j)] = glm::vec3(i * particleInitRest, 0.f, j * particleInitRest) + origin;

		}
	}

	Sphere::setupSphere(sphPos, radius);
}

void PhysicsUpdate(float dt) {
	if (!pause)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////Wave

		for (int i = 0; i < clothMeshLines; i++) {

			for (int j = 0; j < clothMeshColumns; j++) {

				lastPosition[getPosInArray(i, j)] = position[getPosInArray(i, j)];

				for (int z = 0; z < nWaves; z++) {
					sumX += (K.x / k[z]) * A[z] * sin(k[z] * firstPosition[getPosInArray(i, j)].x - O[z] * t + p[z]);
					sumY += A[z] * cos(K.x * firstPosition[getPosInArray(i, j)].x - O[z] * t + p[z]);
				}

				position[getPosInArray(i, j)].x = firstPosition[getPosInArray(i, j)].x - sumX;
				position[getPosInArray(i, j)].y = sumY;

				sumX = sumY = 0;

			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////Sphere

		//get line vertical

		auxX = std::abs(glm::distance(position[getPosInArray(0, 0)].x, sphPos.x));
		distX = auxX;
		lineCloser = 0;

		for (int i = 1; i < clothMeshLines; i++) {

			auxX = std::abs(glm::distance(position[getPosInArray(i, 0)].x, sphPos.x));

			if (auxX < distX) {
				distX = auxX;
				lineCloser = i;
			}

		}

		//get distance to wave perpendicular

		dist = position[getPosInArray(lineCloser, 0)].y - sphPos.y;

		//get volume and area inside water

		if (std::abs(dist) < radius)
		{
			h = radius - std::abs(dist);

			if (dist < 0 || dist == 0)
			{
				V = ((PI * h * h) / 3.f) * (3.f * radius - h);
				AreaR = h;
			}
			else
			{
				V = ((4.f / 3.f) * PI * radius * radius * radius) - ((PI * h * h) / 3.f) * (3.f * radius - h);
				AreaR = radius;
			}
		}
		else if (dist < sphPos.y)
		{
			V = 0;
			AreaR = 0;
		}
		else
		{
			V = (4.f / 3.f) * PI * radius * radius * radius;
			AreaR = radius;
		}

		//calculate U and relative area

		U = (position[getPosInArray(lineCloser, 0)] - lastPosition[getPosInArray(lineCloser, 0)]) + v;	//fluid velocity + object velocity = relative velocity
		Area = PI * AreaR * AreaR;

		//calculate forces and velocity

		g.x = g_[0]; g.y = g_[1]; g.z = g_[2];

		Fb = d * glm::abs(g) * V * glm::vec3{ 0.f, 1.f, 0.f };
		Fa = g * m;
		Fd = -0.5f * d * Cd * Area * glm::length(U) * U * glm::vec3{ 0.f, 1.f, 0.f };

		F = Fa + Fb + Fd;

		a = F / m;

		v += F * dt;

		sphPos += v * dt;

		t += dt;

		if (t > tRestart) reset();
	}

	if (reset_) reset();

	//Draw
	ClothMesh::updateClothMesh((float*)position);
	Sphere::updateSphere(sphPos, radius);
	Sphere::drawSphere();
}

void PhysicsCleanup() {
	Sphere::cleanupSphere();
	ClothMesh::cleanupClothMesh();
}