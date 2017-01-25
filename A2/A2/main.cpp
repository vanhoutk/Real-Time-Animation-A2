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

bool firstMouse = true;
bool keys[1024];
Camera camera(vec3(-1.5f, 2.0f, 7.5f));
enum Meshes { PLANE_MESH };
enum Shaders { SKYBOX, PARTICLE_SHADER, BASIC_TEXTURE_SHADER };
enum Textures { PLANE_TEXTURE };
GLfloat cameraSpeed = 0.005f;
GLfloat deltaTime = 1.0f / 60.0f;
GLfloat friction = 0.98f;
GLfloat lastX = 400, lastY = 300;
GLfloat resilience = 0.01f;
GLfloat yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
GLuint shaderProgramID[NUM_SHADERS];
int screenWidth = 1000;
int screenHeight = 800;
Mesh skyboxMesh, planeMesh;
vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
//vec3 groundVector = vec3(0.0f, -1.0f, 0.0f);
//vec3 groundNormal = vec3(0.0f, 1.0f, 0.0f);

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/airplane3.dae" };
const char * skyboxTextureFiles[6] = { "../Textures/DSposx.png", "../Textures/DSnegx.png", "../Textures/DSposy.png", "../Textures/DSnegy.png", "../Textures/DSposz.png", "../Textures/DSnegz.png"};
const char * textureFiles[NUM_TEXTURES] = { "../Textures/plane.jpg" };

const char * vertexShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxVertexShader.txt", "../Shaders/ParticleVertexShader.txt", "../Shaders/BasicTextureVertexShader.txt" };
const char * fragmentShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxFragmentShader.txt", "../Shaders/ParticleFragmentShader.txt", "../Shaders/BasicTextureFragmentShader.txt" };

GLfloat radians(GLfloat degrees)
{
	// TODO: Make sure this works correctly
	return (degrees * ((2.0 * PI) / 360.0));
}

mat4 getRotationMatrix(GLfloat yawR, GLfloat pitchR, GLfloat rollR)
{
	mat4 rotationMatrix = identity_mat4();
	rotationMatrix.m[0] = cos(yawR) * cos(pitchR);
	rotationMatrix.m[1] = sin(yawR) * cos(pitchR);
	rotationMatrix.m[2] = -1 * sin(pitchR);

	rotationMatrix.m[4] = (cos(yawR) * sin(pitchR) * sin(rollR)) - (sin(yawR) * cos(rollR));
	rotationMatrix.m[5] = (sin(yawR) * sin(pitchR) * sin(rollR)) + (cos(yawR) * cos(rollR));
	rotationMatrix.m[6] = cos(pitchR) * sin(rollR);

	rotationMatrix.m[8] = (cos(yawR) * sin(pitchR) * cos(rollR)) + (sin(yawR) * sin(rollR));
	rotationMatrix.m[9] = (sin(yawR) * sin(pitchR) * cos(rollR)) - (cos(yawR) * sin(rollR));
	rotationMatrix.m[10] = cos(pitchR) * cos(rollR);

	/*rotationMatrix.m[0] = (cos(yawR) * cos(rollR)) - (cos(pitchR) * sin(yawR) * sin(rollR));
	rotationMatrix.m[1] = sin(pitchR) * sin(rollR);
	rotationMatrix.m[2] = (-1 * cos(rollR) * sin(yawR)) - (cos(yawR) * cos(pitchR) * sin(rollR));

	rotationMatrix.m[4] = sin(yawR) * sin(pitchR);
	rotationMatrix.m[5] = cos(pitchR);
	rotationMatrix.m[6] = cos(yawR) * sin(pitchR);

	rotationMatrix.m[8] = (cos(yawR) * sin(rollR)) + (cos(pitchR) * cos(rollR) * sin(yawR));
	rotationMatrix.m[9] = -1 * cos(rollR) * sin(pitchR);
	rotationMatrix.m[10] = (cos(yawR) * cos(pitchR) * cos(rollR)) - (sin(yawR) * sin(rollR));*/

	return rotationMatrix;

}

versor getQuaternion(GLfloat yawR, GLfloat pitchR, GLfloat rollR)
{
	GLfloat t0 = cos(yawR * 0.5f);
	GLfloat t1 = sin(yawR * 0.5f);
	GLfloat t2 = cos(rollR * 0.5f);
	GLfloat t3 = sin(rollR * 0.5f);
	GLfloat t4 = cos(pitchR * 0.5f);
	GLfloat t5 = sin(pitchR * 0.5f);

	GLfloat qw = t0 * t2 * t4 + t1 * t3 * t5;
	GLfloat qx = t0 * t3 * t4 - t1 * t2 * t5;
	GLfloat qy = t0 * t2 * t5 + t1 * t3 * t4;
	GLfloat qz = t1 * t2 * t4 - t0 * t3 * t5;

	versor q = quat_from_axis_rad(qw, qx, qy, qz);

	return q;
}

void display() 
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(5.0f/255.0f, 1.0f/255.0f, 15.0f/255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox first
	//mat4 view = camera.GetViewMatrix(); 
	mat4 view = look_at(camera.Position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

	skyboxMesh.drawSkybox(view, projection);

	versor planeQuat1 = quat_from_axis_deg(0.0f, 0.0f, 1.0f, 1.0f);
	versor planeQuat2 = getQuaternion(radians(yaw), radians(pitch), radians(roll));

	mat4 rotation = quat_to_mat4(planeQuat1 * planeQuat2);

	mat4 model = identity_mat4();
	model = rotate_x_deg(model, -90.0f);
	model = model * rotation;
	//model = model * getRotationMatrix(radians(yaw), radians(pitch), radians(roll));
	

	planeMesh.drawMesh(view, projection, model);

	
	glutSwapBuffers();
}

void processInput()
{
	if (keys[GLUT_KEY_UP])
		camera.ProcessKeyboard(FORWARD, cameraSpeed);
	if(keys[GLUT_KEY_DOWN])
		camera.ProcessKeyboard(BACKWARD, cameraSpeed);
	if (keys[GLUT_KEY_LEFT])
		camera.ProcessKeyboard(LEFT, cameraSpeed);
	if (keys[GLUT_KEY_RIGHT])
		camera.ProcessKeyboard(RIGHT, cameraSpeed);
	if (keys['a'])
		roll -= 0.1f;
	if (keys['s'])
		roll += 0.1f;
	if (keys['q'])
		yaw -= 0.1f;
	if (keys['w'])
		yaw += 0.1f;
	if (keys['z'])
		pitch -= 0.1f;
	if (keys['x'])
		pitch += 0.1f;
	if (keys['p'])
	{
		cout << roll << " " << pitch << " " << yaw << endl;
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

	/*groundMesh = Mesh(&shaderProgramID[BASIC_TEXTURE_SHADER]);
	groundMesh.generateObjectBufferMesh(meshFiles[PLANE_MESH]);
	groundMesh.loadTexture(textureFiles[1]);

	wallMesh = Mesh(&shaderProgramID[BASIC_TEXTURE_SHADER]);
	wallMesh.generateObjectBufferMesh(meshFiles[PLANE_MESH]);
	wallMesh.loadTexture(textureFiles[2]);*/
}

/*
 *	User Input Functions
 */
#pragma region USER_INPUT_FUNCTIONS
void pressNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = true;
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
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	GLfloat xoffset = x - lastX;
	GLfloat yoffset = lastY - y;

	lastX = x;
	lastY = y;

	//yaw += xoffset;
	//pitch += yoffset;


	//if (pitch > 89.0f)
	//	pitch = 89.0f;
	//if (pitch < -89.0f)
	//	pitch = -89.0f;


	//camera.ProcessMouseMovement(xoffset, yoffset);
}

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