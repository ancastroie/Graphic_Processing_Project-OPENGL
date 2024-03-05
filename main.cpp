//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"


#include <iostream>

int glWindowWidth = 1500;
int glWindowHeight = 1000;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

enum MOVE_DIRECTION { MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT };


gps::Camera myCamera(
	glm::vec3(0.0f, 0.0f, 2.5f),
	glm::vec3(0.0f, 0.0f, -10.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.01f;

bool pressedKeys[1024];
float angleY = 0.0f;

//my objects 
gps::Model3D myModel;
gps::Model3D balloon;
gps::Model3D cat;
gps::Model3D airplane;
gps::Model3D lamp;
gps::Model3D elice;


gps::Shader myCustomShader;
gps::Shader depthMapShader;
gps::Shader lightShader;

//fog
int fogReady;
GLint foginitLoc;

//shadows 
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 2048;


//point light 
int startPoint = 0;
glm::vec3 lightPos1;
GLuint lightPos1Loc;

//car
float objectPositionX = 0.0f;

GLenum glCheckError_(const char* file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	// 	
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO

	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
		0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix()
{
	glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = 0.1f, far_plane = 6.0f;
	glm::mat4 lightProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400, lastY = 300;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.4f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
}
float angle = 0;
float angle1 = 0;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f;

float angle2 = 0.0f;
float rotationSpeed = 50.0f;
void processMovement()
{

	if (glfwGetKey(glWindow, GLFW_KEY_F)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (glfwGetKey(glWindow, GLFW_KEY_E)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	if (glfwGetKey(glWindow, GLFW_KEY_V)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	model = glm::mat4(1.0f);
	if (glfwGetKey(glWindow, GLFW_KEY_R))
	{
		angle += 0.5f;
		if (angle > 360.0f)
			angle -= 360.0f;
	}
	if (glfwGetKey(glWindow, GLFW_KEY_L))
	{
		angle -= 0.5f;
		if (angle < 0.0f)
			angle += 360.0f;
	}
	if (glfwGetKey(glWindow, GLFW_KEY_LEFT))
	{
		angle1 -= 0.002f;
	}
	if (glfwGetKey(glWindow, GLFW_KEY_RIGHT))
	{
		angle1 += 0.002f;
	}
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	float cameraSpeed = 1.0f * deltaTime;
	if (glfwGetKey(glWindow, GLFW_KEY_W))
	{
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);

	}
	if (glfwGetKey(glWindow, GLFW_KEY_S))
	{
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);

	}
	if (glfwGetKey(glWindow, GLFW_KEY_D))
	{
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);

	}
	if (glfwGetKey(glWindow, GLFW_KEY_A))
	{
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);

	}

	//fog
	if (glfwGetKey(glWindow, GLFW_KEY_Q))
	{
		myCustomShader.useShaderProgram();
		fogReady = 1;
		foginitLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fogReady");
		glUniform1i(foginitLoc, fogReady);
	}
	if (glfwGetKey(glWindow, GLFW_KEY_Z))
	{
		myCustomShader.useShaderProgram();
		fogReady = 0;
		foginitLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fogReady");
		glUniform1i(foginitLoc, fogReady);
	}
	//point light 
	if (glfwGetKey(glWindow, GLFW_KEY_O)) {
		myCustomShader.useShaderProgram();
		startPoint = 1;
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "startPoint"), startPoint);
	}
	if (glfwGetKey(glWindow, GLFW_KEY_P)) {
		myCustomShader.useShaderProgram();
		startPoint = 0;
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "startPoint"), startPoint);
	}
	//for car movement 
	float speed = 0.05f;
	if(glfwGetKey(glWindow, GLFW_KEY_RIGHT)) {
		objectPositionX += speed;
	}
	if (glfwGetKey(glWindow, GLFW_KEY_LEFT)) {
		objectPositionX -= speed;
	}
	//for elice 
	if (glfwGetKey(glWindow, GLFW_KEY_1)) {
		angle2 += rotationSpeed * deltaTime;
	}

	// create a translation matrix
	model = glm::translate(model, glm::vec3(2, 0, 0));
	// send matrix data to vertex shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// create rotation matrix
	model = glm::rotate(model, angle1, glm::vec3(0, 1, 0));
	// send matrix data to vertex shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	myCustomShader.useShaderProgram();
	glm::mat4 view = myCamera.getViewMatrix();
	GLint viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//window scaling for HiDPI displays
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	//for sRBG framebuffer
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	//for antialising
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	//glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (__APPLE__)
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}



void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {

	myModel.LoadModel("objects/scene/mysceneWithoutTree.obj");
	balloon.LoadModel("objects/balloon/balloon1.obj");
	cat.LoadModel("objects/cat/mycat.obj");
	airplane.LoadModel("objects/airplane/avion.obj");
	lamp.LoadModel("objects/lamp/lamp.obj");
	elice.LoadModel("objects/airplane/elice.obj");

}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	//depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	//depthMapShader.useShaderProgram();

}

void initUniforms() {
	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, -1.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//point light 
	lightPos1 = glm::vec3(0.0f, 0.3f, 1.6f);
	lightPos1Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos1");
	glUniform3fv(lightPos1Loc, 1, glm::value_ptr(lightPos1));

}
//for balloon movement 
float circleRadius = 0.5f;
float circleSpeed = 0.2f;
float currentAngle = 0.0f;
float currentTime;

void renderScene() {
	float xPos = circleRadius * cos(currentAngle);
	float zPos = circleRadius * sin(currentAngle);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//render scnene with shadows 

	/*

	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	depthMapShader.useShaderProgram();
	glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));


	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.18f, 0.0f, -0.5f));
	model = glm::scale(model, glm::vec3(0.1f));
	model = glm::rotate(model, glm::radians(-70.0f), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	cat.Draw(depthMapShader);
	*/
	//////////////////render the scene 



	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	myCustomShader.useShaderProgram();
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 1);

	view = myCamera.getViewMatrix();

	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	//draw my scene 
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(2.82f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	myModel.Draw(myCustomShader);

	//draw the baloon
	currentAngle += circleSpeed * deltaTime;
	currentTime = glfwGetTime();
	// Calculate the new position of the balloon in the circular path

	model = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0.6f, zPos));
	model = glm::scale(model, glm::vec3(0.3f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	balloon.Draw(myCustomShader);

	// draw the cat 
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.18f, 0.0f, -0.5f));
	model = glm::scale(model, glm::vec3(0.15f));
	model = glm::rotate(model, glm::radians(-70.0f), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	cat.Draw(myCustomShader);

	//draw the lamp
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.6f));
	model = glm::scale(model, glm::vec3(0.1f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	lamp.Draw(myCustomShader);

	//draw airplane
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.2f, 0.04f, 2.0f));
	model = glm::scale(model, glm::vec3(0.003f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 1, 0));

	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	airplane.Draw(myCustomShader);

	//draw elice
	float radians = glm::radians(angle2);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.285f,0.137f,2.0f));
	model = glm::rotate(model, radians, glm::vec3(0.0f, 1.0f,0.0f));
	
	//model = glm::scale(model, glm::vec3(0.003f));

	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	elice.Draw(myCustomShader);
}

void cleanup() {

	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}


int main(int argc, const char* argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();

	//initSkybox();
	//initSkyBoxShader();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();

	return 0;
}
