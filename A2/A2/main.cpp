/*
*	Includes
*/
#include <assimp/cimport.h>		// C importer
#include <assimp/scene.h>		// Collects data
#include <assimp/postprocess.h> // Various extra operations
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <math.h>
#include <mmsystem.h>
#include <stdio.h>
#include <vector>				// STL dynamic memory
#include <windows.h>

#include "Antons_maths_funcs.h" // Anton's maths functions
#include "Camera.h"
#include "Mesh.h"
#include "PlaneRotation.h"
#include "Shader_Functions.h"
#include "time.h"

using namespace std;

/*
*	Globally defined variables and constants
*/
#define BUFFER_OFFSET(i) ((char *)NULL + (i))  // Macro for indexing vertex buffer

#define NUM_MESHES   1
#define NUM_SHADERS	 3
#define NUM_TEXTURES 1

bool firstPerson = false;
bool useEuler = false;
bool keys[1024];
Camera camera(vec3(-1.5f, 2.0f, 7.5f));
enum Meshes { PLANE_MESH };
enum Shaders { SKYBOX, PARTICLE_SHADER, BASIC_TEXTURE_SHADER };
enum Textures { PLANE_TEXTURE };
GLfloat yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
GLuint shaderProgramID[NUM_SHADERS];
int screenWidth = 1000;
int screenHeight = 800;
Mesh skyboxMesh, planeMesh;
vec4 upV = vec4(0.0f, 0.0f, 1.0f, 0.0f); //Up and Forward are flipped because of the initial rotation of the model
vec4 fV = vec4(0.0f, 1.0f, 0.0f, 0.0f);
vec4 rightV = vec4(1.0f, 0.0f, 0.0f, 0.0f);
vec4 eulerUpV = vec4(0.0f, 0.0f, 1.0f, 0.0f); //Up and Forward are flipped because of the initial rotation of the model
vec4 eulerFV = vec4(0.0f, 1.0f, 0.0f, 0.0f);
vec4 eulerRightV = vec4(1.0f, 0.0f, 0.0f, 0.0f);
vec3 origin = vec3(0.0f, 0.0f, 0.0f);
versor orientation;
mat4 rotationMat;
//mat4 eulerRotationMat;

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/airplane3.dae" };
const char * skyboxTextureFiles[6] = { "../Textures/DSposx.png", "../Textures/DSnegx.png", "../Textures/DSposy.png", "../Textures/DSnegy.png", "../Textures/DSposz.png", "../Textures/DSnegz.png" };
const char * textureFiles[NUM_TEXTURES] = { "../Textures/plane.jpg" };

const char * vertexShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxVertexShader.txt", "../Shaders/ParticleVertexShader.txt", "../Shaders/BasicTextureVertexShader.txt" };
const char * fragmentShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxFragmentShader.txt", "../Shaders/ParticleFragmentShader.txt", "../Shaders/BasicTextureFragmentShader.txt" };

void display()
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(5.0f / 255.0f, 1.0f / 255.0f, 15.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set the view matrix to first or third person views
	mat4 view = identity_mat4();

	if (!firstPerson)
		view = look_at(camera.Position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	else
	{
		if (!useEuler)
			view = look_at(origin, origin - fV, upV);
		else
			view = look_at(origin, origin - eulerFV, eulerUpV);
		view = translate(view, vec3(0.0f, 1.0f, 0.0f));
	}	
	
	// Projection Matrix
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
	
	// Model Matrix
	mat4 model = identity_mat4();
	if (!useEuler)
		model = rotationMat;
	else
	{
		model = rotate_x_deg(model, -90.0f);
		model = model * getRotationMatrix(radians(yaw), radians(roll), radians(pitch), eulerFV, eulerRightV, eulerUpV);
	}
	//model = eulerRotationMat;

	// Draw skybox first
	skyboxMesh.drawSkybox(view, projection);
	
	// Draw the plane
	planeMesh.drawMesh(view, projection, model);

	glutSwapBuffers();
}

void processInput()
{
	if (keys['q'])
	{
		yaw -= 1.0f;
		applyYaw(radians(-1.0f), rotationMat, upV, fV, rightV, orientation);
		//applyEulerYaw(radians(-10.0f), eulerRotationMat, eulerUpV, eulerFV, eulerRightV);
	}
	if (keys['w'])
	{
		yaw += 1.0f;
		applyYaw(radians(1.0f), rotationMat, upV, fV, rightV, orientation);
		//applyEulerYaw(radians(10.0f), eulerRotationMat, eulerUpV, eulerFV, eulerRightV);
	}
	if (keys['a'])
	{
		roll -= 1.0f;
		applyRoll(radians(-1.0f), rotationMat, upV, fV, rightV, orientation);
		//applyEulerRoll(radians(-10.0f), eulerRotationMat, eulerUpV, eulerFV, eulerRightV);
	}
	if (keys['s'])
	{
		roll += 1.0f;
		applyRoll(radians(1.0f), rotationMat, upV, fV, rightV, orientation);
		//applyEulerRoll(radians(10.0f), eulerRotationMat, eulerUpV, eulerFV, eulerRightV);
	}
	if (keys['z'])
	{
		pitch -= 1.0f;
		applyPitch(radians(-1.0f), rotationMat, upV, fV, rightV, orientation);
		//applyEulerPitch(radians(-10.0f), eulerRotationMat, eulerUpV, eulerFV, eulerRightV);
	}
	if (keys['x'])
	{
		pitch += 1.0f;
		applyPitch(radians(1.0f), rotationMat, upV, fV, rightV, orientation);
		//applyEulerPitch(radians(10.0f), eulerRotationMat, eulerUpV, eulerFV, eulerRightV);
	}
	if (keys[(char)27])
		exit(0);
}

void updateScene()
{
	processInput();
	// Draw the next frame
	glutPostRedisplay();
}

void init()
{
	// Initialise the orientation and rotation matrix (the initial rotation is due to the alignment of the model)
	orientation = quat_from_axis_deg(-90.0f, rightV.v[0], rightV.v[1], rightV.v[2]);
	rotationMat = quat_to_mat4(orientation);
	applyYaw(0.0f, rotationMat, upV, fV, rightV, orientation);
	//eulerRotationMat = identity_mat4();
	//eulerRotationMat = rotate_x_deg(eulerRotationMat, -90.0f);
	//applyEulerYaw(0.0f, eulerRotationMat, eulerUpV, eulerFV, eulerRightV);

	// Compile the shaders
	for (int i = 0; i < NUM_SHADERS; i++)
	{
	shaderProgramID[i] = CompileShaders(vertexShaderNames[i], fragmentShaderNames[i]);
	}

	skyboxMesh = Mesh(&shaderProgramID[SKYBOX]);
	skyboxMesh.setupSkybox(skyboxTextureFiles);

	planeMesh = Mesh(&shaderProgramID[BASIC_TEXTURE_SHADER]);
	planeMesh.generateObjectBufferMesh(meshFiles[PLANE_MESH]);
	planeMesh.loadTexture(textureFiles[PLANE_TEXTURE]);
}

/*
*	User Input Functions
*/
#pragma region USER_INPUT_FUNCTIONS
void pressNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = true;
	
	if (keys['1'])
	{
		firstPerson = true;
	}
	if (keys['2'])
	{
		firstPerson = false;
	}
	if (keys['3'])
	{
		useEuler = true;
	}
	if (keys['4'])
	{
		useEuler = false;
	}
}

void releaseNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = false;
}

void pressSpecialKeys(int key, int x, int y)
{
	keys[key] = true;
}

void releaseSpecialKeys(int key, int x, int y)
{
	keys[key] = false;
}

void mouseClick(int button, int state, int x, int y)
{}

void processMouse(int x, int y)
{}

void mouseWheel(int button, int dir, int x, int y)
{}
#pragma endregion

/*
*	Main
*/
int main(int argc, char** argv)
{
	srand(time(NULL));

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(screenWidth, screenHeight);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - screenWidth) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - screenHeight) / 4);
	glutCreateWindow("Plane Rotations");

	// Glut display and update functions
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);

	// User input functions
	glutKeyboardFunc(pressNormalKeys);
	glutKeyboardUpFunc(releaseNormalKeys);
	glutSpecialFunc(pressSpecialKeys);
	glutSpecialUpFunc(releaseSpecialKeys);
	glutMouseFunc(mouseClick);
	glutPassiveMotionFunc(processMouse);
	glutMouseWheelFunc(mouseWheel);


	glewExperimental = GL_TRUE; //for non-lab machines, this line gives better modern GL support

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	// Set up meshes and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}