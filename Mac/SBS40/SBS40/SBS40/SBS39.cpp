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
#include "mesh.h"

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
    tdogl::Program* nullShaders;
    tdogl::Program* shadowVolShaders;
	tdogl::Texture* texture;
	GLuint vbo;
	GLuint vao;
	GLenum drawType;
	GLint drawStart;
	GLint drawCount;
	GLfloat shininess;
	glm::vec3 specularColor;
    Mesh mesh;
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
    Mesh mesh;
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

ModelAsset gBox;
ModelAsset gGround;

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

tdogl::Program* LoadShaders(const char *shaderFile1, const char *shaderFile2, const char *shaderFile3) {
    std::vector<tdogl::Shader> shaders;
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile1, GL_VERTEX_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile2, GL_GEOMETRY_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile3, GL_FRAGMENT_SHADER));
    return new tdogl::Program(shaders);
}

tdogl::Texture* LoadTexture(const char *textureFile) {
	tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(textureFile);
	//bmp.flipVertically();
	return new tdogl::Texture(bmp);
}

void LoadMainAsset() {
    gBox.shaders = LoadShaders("lighting.vs", "lighting.fs");
    //gBox.shaders = LoadShaders("basic_lighting.vs", "basic_lighting.fs");
    gBox.nullShaders = LoadShaders("null_technique.vs", "null_technique.fs");
    gBox.shadowVolShaders = LoadShaders("shadow_volume.vs", "shadow_volume.gs", "shadow_volume.fs");
    gBox.shininess = 80.0;
    gBox.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    gBox.mesh.LoadMesh("box.obj", true);
}

void LoadGroundAsset()
{
    gGround.shaders = LoadShaders("lighting.vs", "lighting.fs");
    gGround.nullShaders = LoadShaders("null_technique.vs", "null_technique.fs");
    gGround.shadowVolShaders = LoadShaders("shadow_volume.vs", "shadow_volume.gs", "shadow_volume.fs");
    //gGround.silhouetteShaders = LoadShaders("silhouette.vs", "silhouette.gs", "silhouette.fs");
    gGround.shininess = 0.0;
    gGround.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    gGround.mesh.LoadMesh("quad.obj", false);
    gGround.texture = LoadTexture("test.png");
}

// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
	return glm::translate(glm::mat4(), glm::vec3(x, y, z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
	return glm::scale(glm::mat4(), glm::vec3(x, y, z));
}

static void CreateInstances()
{
    ModelInstance box;
    box.asset = &gBox;
    float modelScale = 1;
    box.transform = box.originalTransform = translate(0.0f, 0.5f, 0.0f) *scale(modelScale, modelScale, modelScale);
    //* glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    gInstances.push_back(box);
    
    ModelInstance ground;
    ground.asset = &gGround;
    modelScale = 10;
    ground.transform = ground.originalTransform = translate(0.0f, -1.0f, 0.0f) *scale(modelScale, modelScale, modelScale)
    * glm::rotate(glm::mat4(), glm::radians(90.0f), glm::vec3(1, 0, 0));
    gInstances.push_back(ground);
}

// records how far the y axis has been scrolled
void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
	gScrollY += deltaY;
}

void Update(float secondsElapsed, GLFWwindow* window) {
	const GLfloat degreesPerSecond = 10.0f;
	//const GLfloat degreesPerSecond = 0.0f;
	gDegreesRotated += secondsElapsed * degreesPerSecond;
	while (gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;
    gInstances.front().transform = gInstances.front().originalTransform * glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
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

void RenderShadowedSceneInstance(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    //    shaders->setUniform("gShadowMap", 1);
    //
    shaders->setUniform("numLights", (int)gLights.size());
    
    for (size_t i = 0; i < gLights.size(); ++i) {
        SetLightUniform(shaders, "position", i, gLights[i].position);
        SetLightUniform(shaders, "intensities", i, gLights[i].intensities);
        SetLightUniform(shaders, "attenuation", i, gLights[i].attenuation);
        //SetLightUniform(shaders, "ambientCoefficient", i, gLights[i].ambientCoefficient);
        SetLightUniform(shaders, "ambientCoefficient", i, 0.0f);
        SetLightUniform(shaders, "dCoefficient", i, 1.0f);
        SetLightUniform(shaders, "coneAngle", i, gLights[i].coneAngle);
        SetLightUniform(shaders, "coneDirection", i, gLights[i].coneDirection);
    }
    
    shaders->setUniform("cameraPosition", gCamera.position());
    
    //set the shader uniforms
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    //shaders->setUniform("gDisplacementMap", 4); //set to 4 because the texture will be bound to GL_TEXTURE4
    
    shaders->setUniform("materialShininess", asset->shininess);
    shaders->setUniform("materialSpecularColor", asset->specularColor);
    
    if (asset->texture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asset->texture->object());
    }
    
    asset->mesh.Render();
    
    shaders->stopUsing();
}

void RenderSceneIntoDepthInstance(const ModelInstance& inst)
{
    ModelAsset* asset = inst.asset;
    tdogl::Program* nullShaders = asset->nullShaders;
    
    //bind the shaders
    nullShaders->use();
    
    nullShaders->setUniform("camera", gCamera.matrix());
    nullShaders->setUniform("model", inst.transform);
    
    asset->mesh.Render();
    
    nullShaders->stopUsing();
}

void RenderSceneIntoDepth()
{
    glDrawBuffer(GL_NONE);
    
    for (auto it = gInstances.begin(); it != gInstances.end(); ++it) {
        RenderSceneIntoDepthInstance(*it);
    }
}

void RenderShadowVolIntoStencilInstance(const ModelInstance& inst)
{
    ModelAsset* asset = inst.asset;
    tdogl::Program* shadowVolShaders = asset->shadowVolShaders;
    
    //bind the shaders
    shadowVolShaders->use();
    
    shadowVolShaders->setUniform("gLightPos", glm::vec3(gLights[0].position.x, gLights[0].position.y, gLights[0].position.z));
    shadowVolShaders->setUniform("gWorld", inst.transform);
    shadowVolShaders->setUniform("gVP", gCamera.matrix());
    
    asset->mesh.Render();
    
    shadowVolShaders->stopUsing();
}

void RenderShadowVolIntoStencil()
{
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_CLAMP);
    glDisable(GL_CULL_FACE);
    
    // We need the stencil test to be enabled but we want it
    // to succeed always. Only the depth test matters.
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    
    //for (auto it = gInstances.begin(); it != gInstances.end(); ++it) {
    //	RenderShadowVolIntoStencilInstance(*it);
    //}
    auto it = gInstances.begin();
    RenderShadowVolIntoStencilInstance(*it);
    
    // Restore local stuff
    glDisable(GL_DEPTH_CLAMP);
    glEnable(GL_CULL_FACE);
}

void RenderShadowedScene()
{
    glDrawBuffer(GL_BACK);
    
    // Draw only if the corresponding stencil value is zero
    glStencilFunc(GL_EQUAL, 0x0, 0xFF);
    
    // prevent update to the stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    
    for (auto it = gInstances.begin(); it != gInstances.end(); ++it) {
        RenderShadowedSceneInstance(*it);
    }
}

void RenderAmbientLightInstance(const ModelInstance& inst)
{
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    //    shaders->setUniform("gShadowMap", 1);
    //
    shaders->setUniform("numLights", (int)gLights.size());
    
    for (size_t i = 0; i < gLights.size(); ++i) {
        SetLightUniform(shaders, "position", i, gLights[i].position);
        SetLightUniform(shaders, "intensities", i, gLights[i].intensities);
        SetLightUniform(shaders, "attenuation", i, gLights[i].attenuation);
        SetLightUniform(shaders, "ambientCoefficient", i, gLights[i].ambientCoefficient);
        SetLightUniform(shaders, "dCoefficient", i, 0.0f);
        SetLightUniform(shaders, "coneAngle", i, gLights[i].coneAngle);
        SetLightUniform(shaders, "coneDirection", i, gLights[i].coneDirection);
    }
    
    shaders->setUniform("cameraPosition", gCamera.position());
    
    //set the shader uniforms
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    //shaders->setUniform("gDisplacementMap", 4); //set to 4 because the texture will be bound to GL_TEXTURE4
    
    shaders->setUniform("materialShininess", asset->shininess);
    shaders->setUniform("materialSpecularColor", asset->specularColor);
    
    if (asset->texture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asset->texture->object());
    }
    
    asset->mesh.Render();
    
    shaders->stopUsing();
}

void RenderAmbientLight()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    
    for (auto it = gInstances.begin(); it != gInstances.end(); ++it) {
        RenderAmbientLightInstance(*it);
    }
    
    glDisable(GL_BLEND);
}


void Render(GLFWwindow* window)
{
    glDepthMask(GL_TRUE);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    RenderSceneIntoDepth();
    
    glEnable(GL_STENCIL_TEST);
    
    RenderShadowVolIntoStencil();
    
    RenderShadowedScene();
    
    glDisable(GL_STENCIL_TEST);
    
    RenderAmbientLight();
    
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
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    LoadMainAsset();
    LoadGroundAsset();
    CreateInstances();

	//glClearColor(0.196078431372549f, 0.3137254901960784f, 0.5882352941176471f, 1);
	glClearColor(0.0f, 0.0f, 0.0f, 1);


    gCamera.setPosition(glm::vec3(0, 3, 10));
    gCamera.offsetOrientation(10, 0);
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

	//gLights.push_back(spotlight);
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
