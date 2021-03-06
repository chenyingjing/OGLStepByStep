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


#define MAX_PARTICLES 1000
#define PARTICLE_LIFETIME 10.0f

#define PARTICLE_TYPE_LAUNCHER 0.0f
#define PARTICLE_TYPE_SHELL 1.0f
#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define NUM_ROWS 50
#define NUM_COLS 20
#define NUM_INSTANCES NUM_ROWS * NUM_COLS

struct Particle
{
    float Type;
    glm::vec3 Pos;
    glm::vec3 Vel;
    float LifetimeMillis;
};


float gDegreesRotated = 0.0f;
tdogl::Camera gCamera;
double gScrollY = 0.0;
bool isWireframe = false;
float gDispFactor = 0.25;
//float gTLToSet = 1.0f;
//float gTL = 1.0f;

struct ModelAsset {
	tdogl::Program* psUpdateShaders;
    //GLuint textureObj;
    bool m_isFirst;
    unsigned int m_currVB;
    unsigned int m_currTFB;
    float m_time = 0;
    GLuint m_particleBuffer[2];
    GLuint m_transformFeedback[2];
    
    
    tdogl::Program* shaders;
	tdogl::Texture* texture;
    tdogl::Texture* displacementTexture;
	GLuint vbo;
	GLuint psvao;
    GLuint vao;
	GLenum drawType;
	GLint drawStart;
	GLint drawCount;
	GLfloat shininess;
	glm::vec3 specularColor;
	Mesh mesh;
};

struct ModelInstance {
	ModelAsset* asset;
	glm::mat4 transform;
	glm::mat4 originalTransform;
	Vector3f m_positions[NUM_INSTANCES];
	float m_velocity[NUM_INSTANCES];
};

struct Light {
	glm::vec4 position;
	glm::vec3 intensities; //a.k.a. the color of the light
	float attenuation;
	float ambientCoefficient;
	float coneAngle;
	glm::vec3 coneDirection;
};

ModelAsset gTank;
ModelAsset gHheli;
ModelAsset gSpider;

std::list<ModelInstance> gInstances;

std::vector<Light> gLights;

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

static tdogl::Program* LoadShaders(const char *shaderFile1, const char *shaderFile2, const char *shaderFile3) {
	std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile1, GL_VERTEX_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile2, GL_GEOMETRY_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile3, GL_FRAGMENT_SHADER));
	return new tdogl::Program(shaders);
}

static tdogl::Program* LoadShaders(const char *shaderFile1, const char *shaderFile2, const char *shaderFile3, const char *shaderFile4) {
    std::vector<tdogl::Shader> shaders;
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile1, GL_VERTEX_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile2, GL_TESS_CONTROL_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile3, GL_TESS_EVALUATION_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile4, GL_FRAGMENT_SHADER));
    return new tdogl::Program(shaders);
}

static tdogl::Texture* LoadTexture(const char *textureFile) {
	tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(textureFile);
	bmp.flipVertically();
	return new tdogl::Texture(bmp);
}

static void LoadMainAsset() {

	gSpider.shaders = LoadShaders("shader/lighting.vs", "shader/lighting.fs");
	gSpider.shininess = 80.0;
	gSpider.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);

	gSpider.mesh.LoadMesh("../../../Content/spider.obj");

	gSpider.shaders->use();
	SetColorUniform(gSpider.shaders, 0, glm::vec4(1.0f, 0.5f, 0.5f, 1.0f));
	SetColorUniform(gSpider.shaders, 1, glm::vec4(0.5f, 1.0f, 1.0f, 1.0f));
	SetColorUniform(gSpider.shaders, 2, glm::vec4(1.0f, 0.5f, 1.0f, 1.0f));
	SetColorUniform(gSpider.shaders, 3, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	gSpider.shaders->stopUsing();

}


// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
	return glm::translate(glm::mat4(), glm::vec3(x, y, z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
	return glm::scale(glm::mat4(), glm::vec3(x, y, z));
}

void CalcPositions(ModelInstance &instance)
{
	for (unsigned int i = 0; i < NUM_ROWS; i++) {
		for (unsigned int j = 0; j < NUM_COLS; j++) {
			unsigned int Index = i * NUM_COLS + j;
			instance.m_positions[Index].x = (float)j;
			instance.m_positions[Index].y = RandomFloat() * 5.0f;
			instance.m_positions[Index].z = (float)i;
			instance.m_velocity[Index] = RandomFloat();
			if (i & 1) {
				instance.m_velocity[Index] *= (-1.0f);
			}
		}
	}
}

static void CreateInstances() {

	ModelInstance spider;
	spider.asset = &gSpider;
	float groundScale = 0.01f;
	spider.transform = spider.originalTransform = translate(0, 6, -10) * glm::rotate(glm::mat4(), glm::radians(90.0f), glm::vec3(0, 1, 0)) *scale(groundScale, groundScale, groundScale);
	//spider.transform = spider.originalTransform = glm::rotate(glm::mat4(), glm::radians(90.0f), glm::vec3(0, 1, 0)) *scale(groundScale, groundScale, groundScale);
	CalcPositions(spider);

	gInstances.push_back(spider);

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
	//gInstances.front().transform = translate(-6, -2, -10) * scale(0.1, 0.1, 0.1) * glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));
	//for (auto it = gInstances.begin(); it != gInstances.end(); ++it) {
	//	it->transform = it->originalTransform * glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0));;
	//}

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
    
    if (glfwGetKey(window, 'C')) {
        isWireframe = !isWireframe;
        
        if (isWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
    
    if (glfwGetKey(window, 'I')) {
        //gDispFactor += 0.01f;
        //gTL += 0.01f;
    } else if (glfwGetKey(window, 'K')) {
//        if (gDispFactor >= 0.01f) {
//            gDispFactor -= 0.01f;
//        }
        //if (gTL >= 1.0f) {
        //    gTL -= 0.01f;
        //}
    }

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
	FILE *infile;
	errno_t err = fopen_s(&infile, filename, "rb");
	if (err)
	{
		return NULL;
	}

	if (!infile) {
#ifdef _DEBUG
		std::cerr << "Unable to open file '" << filename << "'" << std::endl;
#endif /* DEBUG */
		return NULL;
	}

	fseek(infile, 0, SEEK_END);
	int len = ftell(infile);
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
    
//    shaders->setUniform("gShadowMap", 1);
//    
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
    //shaders->setUniform("camera", gCamera.matrix());
    //shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    //shaders->setUniform("gDisplacementMap", 4); //set to 4 because the texture will be bound to GL_TEXTURE4
    
    shaders->setUniform("materialShininess", asset->shininess);
    shaders->setUniform("materialSpecularColor", asset->specularColor);
   
	glm::mat4 WVPMatrics[NUM_INSTANCES];
	glm::mat4 WorldMatrices[NUM_INSTANCES];

	glm::mat4 model;

	static float m_scale = 0;
	m_scale += 0.005f;

	for (unsigned int i = 0; i < NUM_INSTANCES; i++) {
		Vector3f Pos(inst.m_positions[i]);
		Pos.y += sinf(m_scale) * inst.m_velocity[i];
		
		model = translate(Pos.x, Pos.y, Pos.z) * inst.originalTransform;
		WVPMatrics[i] = gCamera.matrix() * model;//p.GetWVPTrans().Transpose();
		WorldMatrices[i] = model;//p.GetWorldTrans().Transpose();
	}

	asset->mesh.Render(NUM_INSTANCES, WVPMatrics, WorldMatrices);

    
    shaders->stopUsing();
}

void Render(float millsElapsed, GLFWwindow* window)
{
	// clear everything
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::list<ModelInstance>::const_iterator it;
    GLenum error2 = glGetError();
    if (error2 != GL_NO_ERROR)
        std::cerr << "OpenGL Error2 " << error2 << std::endl;
    for (it = gInstances.begin(); it != gInstances.end(); ++it) {
        //if (it == gInstances.begin()) {
        //    gTLToSet = gTL;
        //} else {
        //    gTLToSet = 1.0;
        //}
        RenderInstance(*it);
    }

	glfwSwapBuffers(window);
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        std::cerr << "OpenGL Error " << error << std::endl;

	// print out some info about the graphics drivers
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

	// OpenGL settings
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
	LoadMainAsset();

	CreateInstances();

	glClearColor(0.196078431372549f, 0.3137254901960784f, 0.5882352941176471f, 1);
	//glClearColor(0.0f, 0.0f, 0.0f, 1);


	//gCamera.setPosition(glm::vec3(7, 3, 0));
	gCamera.setPosition(glm::vec3(3, 7, 0));
	gCamera.offsetOrientation(10, 0);
	gCamera.setViewportAspectRatio((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT);
	gCamera.setNearAndFarPlanes(0.01f, 1000.0f);
    
    Light directionalLight;
    directionalLight.position = glm::vec4(1, 0.8, 0.6, 0); //w == 0 indications a directional light
    directionalLight.intensities = glm::vec3(0.5, 0.5, 0.5); //weak light
    directionalLight.ambientCoefficient = 0.06f;

    gLights.push_back(directionalLight);

	double lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		/* Poll for and process events */
		glfwPollEvents();

		double thisTime = glfwGetTime();
        float secondsElapsed = (float)(thisTime - lastTime);
        lastTime = thisTime;

        Update(secondsElapsed, window);

		Render(secondsElapsed * 1000, window);

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
