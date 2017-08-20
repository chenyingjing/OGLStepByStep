#include "glew.h"
#include "glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "tdogl/Camera.h"
#include "tdogl/Shader.h"
#include "tdogl/Program.h"
#include "tdogl/Bitmap.h"
#include "tdogl/Texture.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include<vector>
#include<list>
#include <sstream>
#include <string>
#include "ogldev_skinned_mesh.h"

#include "gbuffer.h"

#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 900

#define NUM_ROWS 50
#define NUM_COLS 20
#define NUM_INSTANCES NUM_ROWS * NUM_COLS

float gDegreesRotated = 45.0f;
tdogl::Camera gCamera;
double gScrollY = 0.0;

long long m_startTime;

struct ModelAsset {
	tdogl::Program* shaders;
	tdogl::Texture* texture;
	GLuint vbo;
	GLuint vao;
	GLenum drawType;
	GLint drawStart;
	GLint drawCount;
	GLfloat shininess;
	glm::vec3 specularColor;
    SkinnedMesh mesh;
};

struct Light {
    glm::vec4 position;
    glm::vec3 intensities; //a.k.a. the color of the light
    float attenuation;
    float ambientCoefficient;
    float coneAngle;
    glm::vec3 coneDirection;
};

struct LightAsset {
    tdogl::Program* shaders;
    tdogl::Program* nullShaders;
    SkinnedMesh mesh;
    Light light;
};

struct ModelInstance {
	ModelAsset* asset;
	glm::mat4 transform;
    glm::mat4 originalTransform;
//    Vector3f m_positions[NUM_INSTANCES];
//    float m_velocity[NUM_INSTANCES];
};

struct LightInstance {
    LightAsset* asset;
    glm::mat4 transform;
};

ModelAsset gBob;
ModelAsset gMonkey;
ModelAsset gHheli;

LightAsset gLight1;
LightAsset gLight2;
LightAsset gLight3;

LightAsset gDirLight;

std::list<ModelInstance> gInstances;
std::list<LightInstance> gLightInstances;

std::vector<Light> gLights;

//GBuffer m_gbuffer;

template <typename T>
void SetColorUniform(tdogl::Program* shaders, size_t colorIndex, const T& value) {
    std::ostringstream ss;
    ss << "gColor[" << colorIndex << "]";
    std::string uniformName = ss.str();
    
    shaders->setUniform(uniformName.c_str(), value);
}

static tdogl::Program* LoadShaders(const char *shaderFile1, const char *shaderFile2) {
	std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile1, GL_VERTEX_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile2, GL_FRAGMENT_SHADER));
	return new tdogl::Program(shaders);
}

tdogl::Texture* LoadTexture(const char *textureFile) {
	tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(textureFile);
	//bmp.flipVertically();
	return new tdogl::Texture(bmp);
}

static void LoadMainAsset() {
    gBob.shaders = LoadShaders("skinning.vs", "skinning.fs");
    gBob.shininess = 80.0;
    gBob.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    gDirLight.light.position = glm::vec4(0.0f, -10.0f, 13.0f, 0);
    gDirLight.light.intensities = glm::vec3(1.0f, 1.0f, 1.0f);
    gDirLight.light.attenuation = 0.001f;
    gDirLight.light.ambientCoefficient = 0.005f;
    
    
    gBob.shaders->use();
    gBob.shaders->setUniform("gColorMap", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    gBob.shaders->setUniform("gDirectionalLight.Base.Color", gDirLight.light.intensities.r, gDirLight.light.intensities.g, gDirLight.light.intensities.b);
    gBob.shaders->setUniform("gDirectionalLight.Base.AmbientIntensity", gDirLight.light.ambientCoefficient);
    glm::vec3 direction = glm::vec3(-gDirLight.light.position.x, -gDirLight.light.position.y, -gDirLight.light.position.z);
    direction = glm::normalize(direction);
    gBob.shaders->setUniform("gDirectionalLight.Direction", direction.x, direction.y, direction.z);
    gBob.shaders->setUniform("gDirectionalLight.Base.DiffuseIntensity", 1.0f);
    gBob.shaders->setUniform("gMatSpecularIntensity", 0.0f);
    gBob.shaders->setUniform("gSpecularPower", 0.0f);
    gBob.shaders->stopUsing();
    
    gBob.mesh.LoadMesh("boblampclean.md5mesh");

}

// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
	return glm::translate(glm::mat4(), glm::vec3(x, y, z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
	return glm::scale(glm::mat4(), glm::vec3(x, y, z));
}

static void CreateInstances() {
    ModelInstance bob;
    bob.asset = &gBob;
    float modelScale = 0.8;
    bob.transform = bob.originalTransform = translate(0.0f, -26.0f, -12.0f) *scale(modelScale, modelScale, modelScale)
    * glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    gInstances.push_back(bob);
}

// records how far the y axis has been scrolled
void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
	gScrollY += deltaY;
}

void Update(float secondsElapsed, GLFWwindow* window) {
	const GLfloat degreesPerSecond = 40.0f;
	//const GLfloat degreesPerSecond = 0.0f;
	gDegreesRotated += secondsElapsed * degreesPerSecond;
	while (gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;
//    for (auto it = gInstances.begin(); it != gInstances.end(); ++it) {
//        it->transform = it->originalTransform * glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
//    }

	//move position of camera based on WASD keys
	const float moveSpeed = 4.0; //units per second
	if (glfwGetKey(window, 'S')) {
		gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.forward());
	}
	else if (glfwGetKey(window, 'W')) {
		gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.forward());
	}
	if (glfwGetKey(window, 'A')) {
		gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.right());
	}
	else if (glfwGetKey(window, 'D')) {
		gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.right());
	}

	if (glfwGetKey(window, 'Z')) {
		gCamera.offsetPosition(secondsElapsed * moveSpeed * -glm::vec3(0, 1, 0));
	}
	else if (glfwGetKey(window, 'X')) {
		gCamera.offsetPosition(secondsElapsed * moveSpeed * glm::vec3(0, 1, 0));
	}

	//move light
	if (glfwGetKey(window, '1')) {
		gLights[0].position = glm::vec4(gCamera.position(), 1.0);
		gLights[0].coneDirection = gCamera.forward();
	}

	// change light color
	if (glfwGetKey(window, '2'))
		gLights[0].intensities = glm::vec3(1, 0, 0); //red
	else if (glfwGetKey(window, '3'))
		gLights[0].intensities = glm::vec3(0, 1, 0); //green
	else if (glfwGetKey(window, '4'))
		gLights[0].intensities = glm::vec3(1, 1, 1); //white
	else if (glfwGetKey(window, '5'))
		gLights[1].position = glm::vec4(1, 0.8, 0.6, 0);
	else if (glfwGetKey(window, '6'))
		gLights[1].position = glm::vec4(0, 0.8, -0.6, 0);

	//rotate camera based on mouse movement
	const float mouseSensitivity = 0.1f;
	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	gCamera.offsetOrientation(mouseSensitivity * (float)mouseY, mouseSensitivity * (float)mouseX);
	glfwSetCursorPos(window, 0, 0); //reset the mouse, so it doesn't go out of the window

	const float zoomSensitivity = -0.2f;
	float fieldOfView = gCamera.fieldOfView() + zoomSensitivity * (float)gScrollY;
	if (fieldOfView < 5.0f) fieldOfView = 5.0f;
	if (fieldOfView > 130.0f) fieldOfView = 130.0f;
	gCamera.setFieldOfView(fieldOfView);
	gScrollY = 0;

}

const GLchar* ReadShader(const char* filename)
{
	FILE *infile = fopen(filename, "rb");

	if (!infile) {
#ifdef _DEBUG
		std::cerr << "Unable to open file '" << filename << "'" << std::endl;
#endif /* DEBUG */
		return NULL;
	}

	fseek(infile, 0, SEEK_END);
	long len = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	GLchar* source = new GLchar[len + 1];
	fread((void *)source, 1, len, infile);
	source[len] = 0;
	fclose(infile);

	return source;
}

template <typename T>
void SetLightUniform(tdogl::Program* shaders, const char* propertyName, size_t lightIndex, const T& value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shaders->setUniform(uniformName.c_str(), value);
}

float GetRunningTime()
{
    float RunningTime = (float)((double)GetCurrentTimeMillis() - (double)m_startTime) / 1000.0f;
    return RunningTime;
}

static void RenderInstance(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    vector<Matrix4f> Transforms;
    
    float RunningTime = GetRunningTime();
    
    inst.asset->mesh.BoneTransform(RunningTime, Transforms);
    
    for (uint i = 0; i < Transforms.size(); i++) {
        //m_pEffect->SetBoneTransform(i, Transforms[i]);
        char Name[128];
        memset(Name, 0, sizeof(Name));
        SNPRINTF(Name, sizeof(Name), "gBones[%d]", i);
        shaders->setUniformMatrix4(Name, Transforms[i], 1, true);
    }
    
    shaders->setUniform("gEyeWorldPos", gCamera.position());
    
    shaders->setUniform("gWVP", gCamera.matrix() * inst.transform);
    shaders->setUniform("gWorld", inst.transform);
    
    asset->mesh.Render();
    
    shaders->stopUsing();

}

void Render(GLFWwindow* window)
{
    // clear everything
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    std::list<ModelInstance>::const_iterator it;
    GLenum error2 = glGetError();
    if (error2 != GL_NO_ERROR)
        std::cerr << "OpenGL Error2 " << error2 << std::endl;
    for (it = gInstances.begin(); it != gInstances.end(); ++it) {
        RenderInstance(*it);
    }
    
    glfwSwapBuffers(window);
}

int main(void)
{
    m_startTime = GetCurrentTimeMillis();
    
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetScrollCallback(window, OnScroll);

	// GLFW settings
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, 0, 0);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE; //stops glew crashing on OSX :-/
	if (glewInit() != GLEW_OK)
	{
		glfwTerminate();
		return -1;
	}

	// print out some info about the graphics drivers
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

	// OpenGL settings
//	glEnable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LESS);
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    LoadMainAsset();
	CreateInstances();
    
//    LoadLightAsset1();
//    LoadLightAsset2();
//    LoadLightAsset3();
//    LoadDirLightAsset();
//    CreateLightInstances();

	//glClearColor(0.196078431372549f, 0.3137254901960784f, 0.5882352941176471f, 1);
	glClearColor(0.0f, 0.0f, 0.0f, 1);


    gCamera.setPosition(glm::vec3(0, 0, 60));
    gCamera.offsetOrientation(0, 0);
    gCamera.setViewportAspectRatio((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT);
    gCamera.setNearAndFarPlanes(0.01f, 1000.0f);

	// setup lights
	Light spotlight;
	spotlight.position = glm::vec4(-4, 0, 10, 1);
	spotlight.intensities = glm::vec3(2, 2, 2); //strong white light
	spotlight.attenuation = 0.001f;
	spotlight.ambientCoefficient = 0.0f; //no ambient light
	spotlight.coneAngle = 15.0f;
	spotlight.coneDirection = glm::vec3(0, 0, -1);

	Light directionalLight;
	directionalLight.position = glm::vec4(1, 0.8, 0.6, 0); //w == 0 indications a directional light
	directionalLight.intensities = glm::vec3(0.4, 0.4, 0.4);
	directionalLight.ambientCoefficient = 0.06f;

	gLights.push_back(spotlight);
	gLights.push_back(directionalLight);

	double lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		/* Poll for and process events */
		glfwPollEvents();

		double thisTime = glfwGetTime();
		Update((float)(thisTime - lastTime), window);
		lastTime = thisTime;

		Render(window);

		// check for errors
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
			std::cerr << "OpenGL Error " << error << std::endl;

		//exit program if escape key is pressed
		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(window, GL_TRUE);
	}

	glfwTerminate();
	return 0;
}
