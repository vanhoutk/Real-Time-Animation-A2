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
bool firstPerson = false;
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
vec3 upVector = vec3(0.0f, 0.0f, 1.0f);
vec3 forwardVector = vec3(0.0f, 1.0f, 0.0f);
vec3 leftVector = vec3(1.0f, 0.0f, 0.0f);
vec3 upV = vec4(0.0f, 0.0f, 1.0f, 0.0f);
vec3 fV = vec4(0.0f, 1.0f, 0.0f, 0.0f);
vec3 rightV = vec4(1.0f, 0.0f, 0.0f, 0.0f);
vec3 origin = vec3(0.0f, 0.0f, 0.0f);
versor yawQuat;
versor rollQuat;
versor pitchQuat;
versor orientation;
mat4 rotationMat;
//vec3 groundVector = vec3(0.0f, -1.0f, 0.0f);
//vec3 groundNormal = vec3(0.0f, 1.0f, 0.0f);

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/airplane3.dae" };
const char * skyboxTextureFiles[6] = { "../Textures/DSposx.png", "../Textures/DSnegx.png", "../Textures/DSposy.png", "../Textures/DSnegy.png", "../Textures/DSposz.png", "../Textures/DSnegz.png" };
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
	// Tait-Bryan Angles ZYX
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

mat3 mat4to3(mat4 matrix)
{
	mat3 result = identity_mat3();
	result.m[0] = matrix.m[0];
	result.m[1] = matrix.m[1];
	result.m[2] = matrix.m[2];

	result.m[3] = matrix.m[4];
	result.m[4] = matrix.m[5];
	result.m[5] = matrix.m[6];

	result.m[6] = matrix.m[8];
	result.m[7] = matrix.m[9];
	result.m[8] = matrix.m[10];

	return result;
}

void mult_quat_quat(versor &result, versor r, versor s) {

	result.q[0] = s.q[0] * r.q[0] - s.q[1] * r.q[1] -
		s.q[2] * r.q[2] - s.q[3] * r.q[3];
	result.q[1] = s.q[0] * r.q[1] + s.q[1] * r.q[0] -
		s.q[2] * r.q[3] + s.q[3] * r.q[2];
	result.q[2] = s.q[0] * r.q[2] + s.q[1] * r.q[3] +
		s.q[2] * r.q[0] - s.q[3] * r.q[1];
	result.q[3] = s.q[0] * r.q[3] - s.q[1] * r.q[2] +
		s.q[2] * r.q[1] + s.q[3] * r.q[0];
	// re-normalise in case of mangling
	normalise(result);
}

vec3 matbyvec(mat3 matrix, vec3 vector)
{
	vec3 result;
	result.v[0] = vector.v[0] * matrix.m[0] + vector.v[1] * matrix.m[3] + vector.v[2] * matrix.m[6];
	result.v[1] = vector.v[0] * matrix.m[1] + vector.v[1] * matrix.m[4] + vector.v[2] * matrix.m[7];
	result.v[2] = vector.v[0] * matrix.m[2] + vector.v[1] * matrix.m[5] + vector.v[2] * matrix.m[8];
	return result;
}

void applyYaw(GLfloat yawR/*, vec3 &up, vec3 &forward, vec3 &left*/)
{
	/*cout << "Yaw in degrees: " << yaw << endl;
	cout << "Yaw in radians: " << yawR << endl;
	versor result = quat_from_axis_rad(yawR, up.v[0], up.v[1], up.v[2]);
	cout << "Versor: " << result.q[0] << "," << result.q[1] << "," << result.q[2] << "," << result.q[3] << endl;
	/*forward.v[0] = cos(yawR);
	forward.v[1] = 0.0f;
	forward.v[2] = sin(yawR);
	forward = normalise(forward);*/
	/*cout << "Forward vector: " << forward.v[0] << "," << forward.v[1] << "," << forward.v[2] << endl;
	mat4 rotation = quat_to_mat4(result);
	mat3 rotation3 = mat4to3(rotation);
	forward = matbyvec(rotation3, forward);
	cout << "Forward vector: " << forward.v[0] << "," << forward.v[1] << "," << forward.v[2] << endl;
	forward = normalise(forward);
	cout << "Forward vector: " << forward.v[0] << "," << forward.v[1] << "," << forward.v[2] << endl;
	// Also re-calculate the Right and Up vector
	left = normalise(cross(forward, up));
	return;*/


	versor quat = quat_from_axis_rad(yawR, upV.v[0], upV.v[1], upV.v[2]);
	mult_quat_quat(orientation, quat, orientation);
	rotationMat = quat_to_mat4(orientation);
	fV = rotationMat * vec4(0.0f, 1.0f, 0.0f, 0.0f);
	rightV = rotationMat * vec4(1.0f, 0.0f, 0.0f, 0.0f);
	upV = rotationMat * vec4(0.0f, 0.0f, 1.0f, 0.0f);
	return;
}

void applyRoll(GLfloat rollR, vec3 &up, vec3 &forward, vec3 &left)
{
	/*cout << "Roll in degrees: " << roll << endl;
	cout << "Roll in radians: " << rollR << endl;
	versor result = quat_from_axis_rad(rollR, forward.v[0], forward.v[1], forward.v[2]);
	cout << "Versor: " << result.q[0] << "," << result.q[1] << "," << result.q[2] << "," << result.q[3] << endl;
	cout << "Left vector: " << left.v[0] << "," << left.v[1] << "," << left.v[2] << endl;
	mat4 rotation = quat_to_mat4(result);
	mat3 rotation3 = mat4to3(rotation);
	left = matbyvec(rotation3, left);
	cout << "Left vector: " << left.v[0] << "," << left.v[1] << "," << left.v[2] << endl;
	left = normalise(left);
	cout << "Left vector: " << left.v[0] << "," << left.v[1] << "," << left.v[2] << endl;
	// Also re-calculate the Right and Up vector
	up = normalise(cross(left, forward));
	return;*/
	versor quat = quat_from_axis_rad(rollR, fV.v[0], fV.v[1], fV.v[2]);
	mult_quat_quat(orientation, quat, orientation);
	rotationMat = quat_to_mat4(orientation);
	fV = rotationMat * vec4(0.0f, 1.0f, 0.0f, 0.0f);
	rightV = rotationMat * vec4(1.0f, 0.0f, 0.0f, 0.0f);
	upV = rotationMat * vec4(0.0f, 0.0f, 1.0f, 0.0f);
	return;
}

void applyPitch(GLfloat pitchR, vec3 &up, vec3 &forward, vec3 &left)
{
	/*cout << "Pitch in degrees: " << pitch << endl;
	cout << "Pitch in radians: " << pitchR << endl;
	versor result = quat_from_axis_rad(pitchR, left.v[0], left.v[1], left.v[2]);
	cout << "Versor: " << result.q[0] << "," << result.q[1] << "," << result.q[2] << "," << result.q[3] << endl;
	cout << "Up vector: " << up.v[0] << "," << up.v[1] << "," << up.v[2] << endl;
	mat4 rotation = quat_to_mat4(result);
	mat3 rotation3 = mat4to3(rotation);
	up = matbyvec(rotation3, up);
	cout << "Up vector: " << up.v[0] << "," << up.v[1] << "," << up.v[2] << endl;
	up = normalise(up);
	cout << "Up vector: " << up.v[0] << "," << up.v[1] << "," << up.v[2] << endl;
	// Also re-calculate the Right and Up vector
	forward = normalise(cross(up, left));
	return;*/
	versor quat = quat_from_axis_rad(pitchR, rightV.v[0], rightV.v[1], rightV.v[2]);
	mult_quat_quat(orientation, quat, orientation);
	rotationMat = quat_to_mat4(orientation);
	fV = rotationMat * vec4(0.0f, 1.0f, 0.0f, 0.0f);
	rightV = rotationMat * vec4(1.0f, 0.0f, 0.0f, 0.0f);
	upV = rotationMat * vec4(0.0f, 0.0f, 1.0f, 0.0f);
	return;
}

/*void lookAtModel
{
	vec3 ZAxis = target - orient.WAxis();
ZAxis.normalize();
vec3 XAxis = CrossProduct(Up, ZAxis);
XAxis.normalize();
vec3 YAxis = CrossProduct(ZAxis, XAxis);
YAxis.normalize();
orient.XAxis(XAxis);
orient.YAxis(YAxis);
orient.ZAxis(ZAxis);
}*/

mat4 getRotationFromVectors()
{
	mat4 result = identity_mat4();
	result.m[0] = -leftVector.v[0];
	result.m[4] = -leftVector.v[1];
	result.m[8] = -leftVector.v[2];
	result.m[1] = upVector.v[0];
	result.m[5] = upVector.v[1];
	result.m[9] = upVector.v[2];
	result.m[2] = -forwardVector.v[0];
	result.m[6] = -forwardVector.v[1];
	result.m[10] = -forwardVector.v[2];
	return result;
}

void display()
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(5.0f / 255.0f, 1.0f / 255.0f, 15.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 view = identity_mat4();

	// Draw skybox first
	//mat4 view = camera.GetViewMatrix(); 
	if(!firstPerson)
		view = look_at(camera.Position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	else
	{
		view = look_at(origin, origin - fV, upV);
		view = translate(view, vec3(0.0f, 1.0f, 0.0f));
	}
	//mat4 view2 = identity_mat4();
	//view2 = rotationMat * view2;
	
	
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

	skyboxMesh.drawSkybox(view, projection);

	mat4 rotation = quat_to_mat4(pitchQuat * rollQuat * yawQuat);

	//mat4 model = look_at(origin, forwardVector, upVector);
	//mat4 model = getRotationFromVectors();
	mat4 model = rotationMat;
	//mat4 model2 = identity_mat4();
	//model2 = rotate_x_deg(model2, -90.0f);
	//model2 = model2 * rotation;
	//model = model * getRotationMatrix(radians(yaw), radians(roll), radians(pitch));


	planeMesh.drawMesh(view, projection, model);


	glutSwapBuffers();
}

void processInput()
{
	//if (keys[GLUT_KEY_UP])
		//camera.ProcessKeyboard(FORWARD, cameraSpeed);
	//if (keys[GLUT_KEY_DOWN])
		//camera.ProcessKeyboard(BACKWARD, cameraSpeed);
	//if (keys[GLUT_KEY_LEFT])
		//camera.ProcessKeyboard(LEFT, cameraSpeed);
	//if (keys[GLUT_KEY_RIGHT])
		//camera.ProcessKeyboard(RIGHT, cameraSpeed);
	if (keys['q'])
	{
		yaw -= 1.0f;
		applyYaw(radians(-1.0f)/*, upVector, forwardVector, leftVector*/);
	}
	if (keys['w'])
	{
		yaw += 1.0f;
		applyYaw(radians(1.0f)/*, upVector, forwardVector, leftVector*/);
	}
	if (keys['a'])
	{
		roll -= 1.0f;
		applyRoll(radians(-1.0f), upVector, forwardVector, leftVector);
	}
	if (keys['s'])
	{
		roll += 1.0f;
		applyRoll(radians(1.0f), upVector, forwardVector, leftVector);
	}
	if (keys['z'])
	{
		pitch -= 1.0f;
		applyPitch(radians(-1.0f), upVector, forwardVector, leftVector);
	}
	if (keys['x'])
	{
		pitch += 1.0f;
		applyPitch(radians(1.0f), upVector, forwardVector, leftVector);
	}

	if (keys['p'])
	{
		cout << "Roll = " << roll << ", Pitch = " << pitch << ", Yaw = " << yaw << endl;
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

	// rotation matrix from my maths library. just holds 16 floats
	mat4 R;
	// make a quaternion representing negated initial camera orientation
	float quaternion[4];
	orientation = quat_from_axis_deg(-90.0f, rightV.v[0], rightV.v[1], rightV.v[2]);
	rotationMat = quat_to_mat4(orientation);
	applyYaw(0.0f);

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

	versor yawQuat = quat_from_axis_deg(1.0f, 0.0f, 0.0f, 0.0f);
	versor rollQuat = quat_from_axis_deg(1.0f, 0.0f, 0.0f, 0.0f);
	versor pitchQuat = quat_from_axis_deg(1.0f, 0.0f, 0.0f, 0.0f);
}

/*
*	User Input Functions
*/
#pragma region USER_INPUT_FUNCTIONS
void pressNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = true;
	/*if (keys['q'])
	{
		yaw -= 90.0f;
		applyYaw(radians(-90.0f), upVector, forwardVector, leftVector);
	}
	if (keys['w'])
	{
		yaw += 90.0f;
		applyYaw(radians(90.0f), upVector, forwardVector, leftVector);
	}
	if (keys['a'])
	{
		roll -= 90.0f;
		applyRoll(radians(-90.0f), upVector, forwardVector, leftVector);
	}
	if (keys['s'])
	{
		roll += 90.0f;
		applyRoll(radians(90.0f), upVector, forwardVector, leftVector);
	}
	if (keys['z'])
	{
		pitch -= 90.0f;
		applyPitch(radians(-90.0f), upVector, forwardVector, leftVector);
	}
	if (keys['x'])
	{
		pitch += 90.0f;
		applyPitch(radians(90.0f), upVector, forwardVector, leftVector);
	}*/
	if (keys['1'])
	{
		firstPerson = true;
	}
	if (keys['2'])
	{
		firstPerson = false;
	}
	if (keys['f'])
	{
		cout << "Forward = " << forwardVector.v[0] << ", " << forwardVector.v[1] << ", " << forwardVector.v[2] << endl;
	}
	if (keys['g'])
	{
		cout << "Left = " << leftVector.v[0] << ", " << leftVector.v[1] << ", " << leftVector.v[2] << endl;
	}
	if (keys['h'])
	{
		cout << "Up = " << upVector.v[0] << ", " << upVector.v[1] << ", " << upVector.v[2] << endl;
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