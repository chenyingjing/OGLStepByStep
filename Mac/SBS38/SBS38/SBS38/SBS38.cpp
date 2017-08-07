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

#include "gbuffer.h"

#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 900

#define NUM_ROWS 50
#define NUM_COLS 20
#define NUM_INSTANCES NUM_ROWS * NUM_COLS

float gDegreesRotated = 45.0f;
tdogl::Camera gCamera;
double gScrollY = 0.0;

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

ModelAsset gJeep;
ModelAsset gMonkey;
ModelAsset gHheli;

LightAsset gLight1;
LightAsset gLight2;
LightAsset gLight3;

LightAsset gDirLight;

std::list<ModelInstance> gInstances;
std::list<LightInstance> gLightInstances;

std::vector<Light> gLights;

GBuffer m_gbuffer;

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
	gJeep.shaders = LoadShaders("geometry_pass.vs", "geometry_pass.fs");
	gJeep.shininess = 80.0;
	gJeep.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    gJeep.mesh.LoadMesh("jeep.obj");
    

}

static void LoadLightAsset1() {
    gLight1.shaders = LoadShaders("light_pass.vs", "point_light_pass.fs");
    
    gLight1.shaders->use();
    gLight1.shaders->setUniform("gPositionMap", 0);
    gLight1.shaders->setUniform("gColorMap", 1);
    gLight1.shaders->setUniform("gNormalMap", 2);
    gLight1.shaders->setUniform("gScreenSize", (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    gLight1.shaders->stopUsing();
    
    gLight1.light.position = glm::vec4(-10, 1.5f, -9.0f, 1);
    gLight1.light.intensities = glm::vec3(0.0f, 1.0f, 0.0f);
    gLight1.light.attenuation = 0.001f;
    gLight1.light.ambientCoefficient = 0.005f;
    
    gLight1.nullShaders = LoadShaders("null_technique.vs", "null_technique.fs");

    gLight1.mesh.LoadMesh("sphere.obj");
}

static void LoadLightAsset2() {
    gLight2.shaders = LoadShaders("light_pass.vs", "point_light_pass.fs");
    
    gLight2.shaders->use();
    gLight2.shaders->setUniform("gPositionMap", 0);
    gLight2.shaders->setUniform("gColorMap", 1);
    gLight2.shaders->setUniform("gNormalMap", 2);
    gLight2.shaders->setUniform("gScreenSize", (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    gLight2.shaders->stopUsing();
    
    gLight2.light.position = glm::vec4(2.0f, 0.0f, -5.0f, 1);
    gLight2.light.intensities = glm::vec3(1.0f, 0.0f, 0.0f);
    gLight2.light.attenuation = 0.002f;
    gLight2.light.ambientCoefficient = 0.005f;
    
    gLight2.nullShaders = LoadShaders("null_technique.vs", "null_technique.fs");

    gLight2.mesh.LoadMesh("sphere.obj");
}

static void LoadLightAsset3() {
    gLight3.shaders = LoadShaders("light_pass.vs", "point_light_pass.fs");
    
    gLight3.shaders->use();
    gLight3.shaders->setUniform("gPositionMap", 0);
    gLight3.shaders->setUniform("gColorMap", 1);
    gLight3.shaders->setUniform("gNormalMap", 2);
    gLight3.shaders->setUniform("gScreenSize", (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    gLight3.shaders->stopUsing();
    
    gLight3.light.position = glm::vec4(-8.0f, -2.0f, -5.0f, 1);
    gLight3.light.intensities = glm::vec3(0.0f, 0.0f, 1.0f);
    gLight3.light.attenuation = 0.001f;
    gLight3.light.ambientCoefficient = 0.005f;
    
    gLight3.nullShaders = LoadShaders("null_technique.vs", "null_technique.fs");

    gLight3.mesh.LoadMesh("sphere.obj");
}

void LoadDirLightAsset()
{
    gDirLight.shaders = LoadShaders("light_pass.vs", "dir_light_pass.fs");
    
    gDirLight.light.position = glm::vec4(8.0f, 2.0f, 5.0f, 0);
    gDirLight.light.intensities = glm::vec3(0.0f, 1.0f, 1.0f);
    gDirLight.light.attenuation = 0.01f;
    gDirLight.light.ambientCoefficient = 0.005f;
    
    gDirLight.shaders->use();
    gDirLight.shaders->setUniform("gPositionMap", 0);
    gDirLight.shaders->setUniform("gColorMap", 1);
    gDirLight.shaders->setUniform("gNormalMap", 2);
    gDirLight.shaders->setUniform("gScreenSize", (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    
    gDirLight.shaders->setUniform("gDirectionalLight.Base.Color", gDirLight.light.intensities.r, gDirLight.light.intensities.g, gDirLight.light.intensities.b);
    gDirLight.shaders->setUniform("gDirectionalLight.Base.AmbientIntensity", gDirLight.light.ambientCoefficient);
    
    glm::vec3 direction = glm::vec3(-gDirLight.light.position.x, -gDirLight.light.position.y, -gDirLight.light.position.z);
    direction = glm::normalize(direction);
    
    gDirLight.shaders->setUniform("gDirectionalLight.Direction", direction.x, direction.y, direction.z);
    gDirLight.shaders->setUniform("gDirectionalLight.Base.DiffuseIntensity", 1.0f);
    
    gDirLight.shaders->stopUsing();
    
    gDirLight.mesh.LoadMesh("quad.obj");
    //gDirLight.mesh.LoadMesh("sphere.obj");
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
	ModelInstance jeep;
	jeep.asset = &gJeep;
    GLfloat mscale = 0.01f;
    jeep.transform = jeep.originalTransform = translate(0.0f, 0.0f, -12.0f) * scale(mscale, mscale, mscale);
	gInstances.push_back(jeep);
    
    ModelInstance jeep1;
    jeep1.asset = &gJeep;
    jeep1.transform = jeep1.originalTransform = translate(-15.0f, 0.0f, -12.0f) * scale(mscale, mscale, mscale);
    gInstances.push_back(jeep1);
}

static void CreateLightInstances() {
    LightInstance light1;
    light1.asset = &gLight1;
    light1.transform = translate(gLight1.light.position.x, gLight1.light.position.y, gLight1.light.position.z);
    gLightInstances.push_back(light1);
    
    LightInstance light2;
    light2.asset = &gLight2;
    light2.transform = translate(gLight2.light.position.x, gLight2.light.position.y, gLight2.light.position.z);
    gLightInstances.push_back(light2);
    
    LightInstance light3;
    light3.asset = &gLight3;
    light3.transform = translate(gLight3.light.position.x, gLight3.light.position.y, gLight3.light.position.z);
    gLightInstances.push_back(light3);
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

static void RenderInstance(const ModelInstance& inst) {
	ModelAsset* asset = inst.asset;
	tdogl::Program* shaders = asset->shaders;

	//bind the shaders
	shaders->use();

	shaders->setUniform("numLights", (int)gLights.size());

	for (size_t i = 0; i < gLights.size(); ++i) {
		SetLightUniform(shaders, "position", i, gLights[i].position);
		SetLightUniform(shaders, "intensities", i, gLights[i].intensities);
		SetLightUniform(shaders, "attenuation", i, gLights[i].attenuation);
		SetLightUniform(shaders, "ambientCoefficient", i, gLights[i].ambientCoefficient);
		SetLightUniform(shaders, "coneAngle", i, gLights[i].coneAngle);
		SetLightUniform(shaders, "coneDirection", i, gLights[i].coneDirection);
	}



	shaders->setUniform("cameraPosition", gCamera.position());

	//set the shader uniforms
	shaders->setUniform("camera", gCamera.matrix());
	shaders->setUniform("model", inst.transform);
	shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0

	shaders->setUniform("materialShininess", asset->shininess);
	shaders->setUniform("materialSpecularColor", asset->specularColor);

//    glm::mat4 WVPMatrics[NUM_INSTANCES];
//    glm::mat4 WorldMatrices[NUM_INSTANCES];
    
    glm::mat4 model;
    
    static float m_scale = 0;
    m_scale += 0.005f;
    
//    for (unsigned int i = 0; i < NUM_INSTANCES; i++) {
//        Vector3f Pos(inst.m_positions[i]);
//        Pos.y += sinf(m_scale) * inst.m_velocity[i];
//        
//        model = translate(Pos.x, Pos.y, Pos.z) * inst.originalTransform;
//        WVPMatrics[i] = gCamera.matrix() * model;
//        WorldMatrices[i] = model;
//    }
    
//    asset->mesh.Render(NUM_INSTANCES, WVPMatrics, WorldMatrices);
    asset->mesh.Render();
    
	shaders->stopUsing();
}

bool Init()
{
    if (!m_gbuffer.Init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        return false;
    }
    return true;
}

void DSGeometryPassInstance(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    //set the shader uniforms
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("gColorMap", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    
    asset->mesh.Render();
    
    shaders->stopUsing();
}

void DSGeometryPass()
{
    m_gbuffer.BindForGeomPass();
    
    glDepthMask(GL_TRUE);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    
    for (auto it = gInstances.begin(); it != gInstances.end(); ++it) {
        DSGeometryPassInstance(*it);
    }
    
    glDepthMask(GL_FALSE);
}

//void DSLightPass()
//{
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//    
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    
//    m_gbuffer.BindForReading();
//    
//    GLint HalfWidth = (GLint)(WINDOW_WIDTH / 2.0f);
//    GLint HalfHeight = (GLint)(WINDOW_HEIGHT / 2.0f);
//    
//
//    m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_POSITION);
//    glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
//    
//    m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_DIFFUSE);
//    glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, HalfHeight, HalfWidth, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
//    
//    m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
//    glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, HalfWidth, HalfHeight, WINDOW_WIDTH, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
//    
//    m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_TEXCOORD);
//    glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, HalfWidth, 0, WINDOW_WIDTH, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
//}

//void BeginLightPasses()
//{
//    glEnable(GL_BLEND);
//    glBlendEquation(GL_FUNC_ADD);
//    glBlendFunc(GL_ONE, GL_ONE);
//    
//    m_gbuffer.BindForReading();
//    glClear(GL_COLOR_BUFFER_BIT);
//}

float CalcPointLightBSphere(const LightAsset& Light)
{
    //Light.light.attenuation
    float MaxChannel = fmax(fmax(Light.light.intensities.r, Light.light.intensities.g), Light.light.intensities.b);
    
    //float ret = (-Light.Attenuation.Linear + sqrtf(Light.Attenuation.Linear * Light.Attenuation.Linear - 4 * Light.Attenuation.Exp * (Light.Attenuation.Exp - 256 * MaxChannel * Light.DiffuseIntensity)))
    //	/
    //	(2 * Light.Attenuation.Exp);
    float ret = (0 + sqrtf(0 - 4 * Light.light.attenuation * (Light.light.attenuation - 256 * MaxChannel * 1)))
    /
    (2 * Light.light.attenuation);
    return ret;
}

void DSPointLightsPassInstance(const LightInstance& inst) {
    m_gbuffer.BindForLightPass();
    
    LightAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    shaders->setUniform("gEyeWorldPos", gCamera.position());
    
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    
    shaders->setUniform("gPointLight.Base.Color", asset->light.intensities.r, asset->light.intensities.g, asset->light.intensities.b);
    shaders->setUniform("gPointLight.Base.AmbientIntensity", asset->light.ambientCoefficient);
    shaders->setUniform("gPointLight.Position", asset->light.position.x, asset->light.position.y, asset->light.position.z);
    shaders->setUniform("gPointLight.Base.DiffuseIntensity", 1.0f);
    shaders->setUniform("gPointLight.Atten.Constant", 1.0f);
    shaders->setUniform("gPointLight.Atten.Linear", 0.0f);
    shaders->setUniform("gPointLight.Atten.Exp", asset->light.attenuation);
    float BSphereScale = CalcPointLightBSphere(*asset);
    shaders->setUniform("gWVP", gCamera.matrix() * inst.transform * scale(BSphereScale, BSphereScale, BSphereScale));
    asset->mesh.Render();
    shaders->stopUsing();
    
    glCullFace(GL_BACK);
    
    glDisable(GL_BLEND);
}

void DSPointLightsPass()
{
    for (auto it = gLightInstances.begin(); it != gLightInstances.end(); ++it) {
        DSPointLightsPassInstance(*it);
    }
}

void DSDirectionalLightPass()
{
    m_gbuffer.BindForLightPass();
    
    gDirLight.shaders->use();
    gDirLight.shaders->setUniform("gEyeWorldPos", gCamera.position());
    glm::mat4 i = glm::rotate(glm::mat4(), glm::radians(180.0f), glm::vec3(1, 0, 0));
    gDirLight.shaders->setUniform("gWVP", i);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    
    gDirLight.mesh.Render();
    gDirLight.shaders->stopUsing();
    
    glDisable(GL_BLEND);
}

void DSStencilPass(const LightInstance& inst)
{
    LightAsset* asset = inst.asset;
    tdogl::Program* nullShaders = asset->nullShaders;
    
    //bind the shaders
    nullShaders->use();
    
    m_gbuffer.BindForStencilPass();
    glEnable(GL_DEPTH_TEST);
    
    glDisable(GL_CULL_FACE);
    
    glClear(GL_STENCIL_BUFFER_BIT);
    
    
    // We need the stencil test to be enabled but we want it
    // to succeed always. Only the depth test matters.
    glStencilFunc(GL_ALWAYS, 0, 0);
    
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    
    float BSphereScale = CalcPointLightBSphere(*asset);
    nullShaders->setUniform("gWVP", gCamera.matrix() * inst.transform * scale(BSphereScale, BSphereScale, BSphereScale));
    asset->mesh.Render();
    nullShaders->stopUsing();
}

void DSFinalPass()
{
    m_gbuffer.BindForFinalPass();
    glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                      0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void Render(GLFWwindow* window)
{
    m_gbuffer.StartFrame();
    
    DSGeometryPass();
    //DSLightPass();
    //BeginLightPasses();
    glEnable(GL_STENCIL_TEST);
    
    //DSPointLightsPass();
    for (auto it = gLightInstances.begin(); it != gLightInstances.end(); ++it) {
        DSStencilPass(*it);
        DSPointLightsPassInstance(*it);
    }
    
    glDisable(GL_STENCIL_TEST);
    
    DSDirectionalLightPass();
    
    DSFinalPass();
    
    glfwSwapBuffers(window);
}

int main(void)
{
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

    if (!Init()) {
        return -1;
    }
    
    LoadMainAsset();
	CreateInstances();
    
    LoadLightAsset1();
    LoadLightAsset2();
    LoadLightAsset3();
    LoadDirLightAsset();
    CreateLightInstances();

	//glClearColor(0.196078431372549f, 0.3137254901960784f, 0.5882352941176471f, 1);
	glClearColor(0.0f, 0.0f, 0.0f, 1);


    gCamera.setPosition(glm::vec3(3, 7, 20));
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
