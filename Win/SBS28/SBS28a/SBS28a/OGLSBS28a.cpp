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
#include "random_texture.h"

#define NUM_ROWS 10
#define NUM_COLUMNS 10

#define MAX_PARTICLES 1000
#define PARTICLE_LIFETIME 10.0f

#define PARTICLE_TYPE_LAUNCHER 0.0f
#define PARTICLE_TYPE_SHELL 1.0f
#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f



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

struct ModelAsset {
	tdogl::Program* psUpdateShaders;
    //GLuint textureObj;
    RandomTexture m_randomTexture;
    bool m_isFirst;
    unsigned int m_currVB;
    unsigned int m_currTFB;
    float m_time = 0;
    GLuint m_particleBuffer[2];
    GLuint m_transformFeedback[2];
    
    
    tdogl::Program* shaders;
	tdogl::Texture* texture;
	GLuint vbo;
	GLuint psvao;
    GLuint vao;
	GLenum drawType;
	GLint drawStart;
	GLint drawCount;
	GLfloat shininess;
	glm::vec3 specularColor;
};

struct ModelInstance {
	ModelAsset* asset;
	glm::mat4 transform;
};

struct Light {
	glm::vec4 position;
	glm::vec3 intensities; //a.k.a. the color of the light
	float attenuation;
	float ambientCoefficient;
	float coneAngle;
	glm::vec3 coneDirection;
};

ModelAsset gFirework;
std::list<ModelInstance> gInstances;
std::list<ModelInstance> gParticleInstances;

std::vector<Light> gLights;

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

static tdogl::Program* LoadPsUpdateShaders(const char *shaderFile1, const char *shaderFile2, const char *shaderFile3) {
    std::vector<tdogl::Shader> shaders;
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile1, GL_VERTEX_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile2, GL_GEOMETRY_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile3, GL_FRAGMENT_SHADER));
    
    const GLchar* Varyings[4];
    Varyings[0] = "Type1";
    Varyings[1] = "Position1";
    Varyings[2] = "Velocity1";
    Varyings[3] = "Age1";
    
    GLsizei count = 4;
    GLenum bufferMode = GL_INTERLEAVED_ATTRIBS;
    
    return new tdogl::Program(shaders, count, Varyings, bufferMode);
}

static tdogl::Texture* LoadTexture(const char *textureFile) {
	tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(textureFile);
	//bmp.flipVertically();
	return new tdogl::Texture(bmp);
}

static void LoadFireworkAsset() {
    gFirework.m_currVB = 0;
    gFirework.m_currTFB = 1;
    gFirework.m_isFirst = true;
    gFirework.m_time = 0;
    
    
    Particle Particles[MAX_PARTICLES] = {0};
    
    Particles[0].Type = PARTICLE_TYPE_LAUNCHER;
    Particles[0].Pos = glm::vec3(0, 0, -2);
    Particles[0].Vel = glm::vec3(0.0f, 0.001f, 0.0f);
    Particles[0].LifetimeMillis = 0.0f;
    
    glGenTransformFeedbacks(2, gFirework.m_transformFeedback);
    glGenBuffers(2, gFirework.m_particleBuffer);
    
    for (unsigned int i = 0; i < 2 ; i++) {
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gFirework.m_transformFeedback[i]);
        glBindBuffer(GL_ARRAY_BUFFER, gFirework.m_particleBuffer[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Particles), Particles, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gFirework.m_particleBuffer[i]);
    }
    
    gFirework.psUpdateShaders = LoadPsUpdateShaders("shader/ps_update.vs",
                                      "shader/ps_update.gs", "shader/ps_update.fs");
    
    gFirework.psUpdateShaders->use();
    gFirework.psUpdateShaders->setUniform("gRandomTexture", 3);//TEXTURE3
    gFirework.psUpdateShaders->setUniform("gLauncherLifetime", 100.0f);
    gFirework.psUpdateShaders->setUniform("gShellLifetime", 10000.0f);
    gFirework.psUpdateShaders->setUniform("gSecondaryShellLifetime", 7000.0f);
    
    if (!gFirework.m_randomTexture.InitRandomTexture(1000)) {
        throw std::runtime_error("InitRandomTexture fail.");
    }
    gFirework.m_randomTexture.Bind(GL_TEXTURE3);
    
    
    gFirework.shaders = LoadShaders("shader/billboard.vs", "shader/billboard.gs", "shader/billboard.fs");
    gFirework.shaders->use();
    gFirework.shaders->setUniform("materialTex", 0);//TEXTURE0
    gFirework.shaders->setUniform("gBillboardSize", 0.01f);
    gFirework.texture = LoadTexture("fireworks_red.jpg");
    
    gFirework.drawType = GL_POINTS;
    gFirework.drawStart = 0;
    gFirework.drawCount = MAX_PARTICLES;
    gFirework.shininess = 80.0;
    gFirework.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
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
    ModelInstance fireworkInstance;
    fireworkInstance.asset = &gFirework;
    fireworkInstance.transform = glm::mat4();
    gParticleInstances.push_back(fireworkInstance);

}

// records how far the y axis has been scrolled
void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
	gScrollY += deltaY;
}

void Update(float secondsElapsed, GLFWwindow* window) {
	//const GLfloat degreesPerSecond = 180.0f;
//	const GLfloat degreesPerSecond = 0.0f;
//	gDegreesRotated += secondsElapsed * degreesPerSecond;
//	while (gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;
//	gInstances.front().transform = glm::rotate(glm::mat4(), gDegreesRotated, glm::vec3(0, 1, 0));

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

static void UpdateParticles(const ModelInstance& inst, float millsElapsed) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* psUpdateShaders = asset->psUpdateShaders;
    psUpdateShaders->use();
    psUpdateShaders->setUniform("gTime", asset->m_time);
    psUpdateShaders->setUniform("gDeltaTimeMillis", millsElapsed);
    
    asset->m_randomTexture.Bind(GL_TEXTURE3);
    glEnable(GL_RASTERIZER_DISCARD);
    
    glBindBuffer(GL_ARRAY_BUFFER, asset->m_particleBuffer[asset->m_currVB]);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, asset->m_transformFeedback[asset->m_currTFB]);
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), 0);                          // type
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)4);         // position
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)16);        // velocity
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)28);          // lifetime
    
    glBeginTransformFeedback(GL_POINTS);
    
    if (asset->m_isFirst) {
        glDrawArrays(GL_POINTS, 0, 1);

		//GLenum error1 = glGetError();
		//if (error1 != GL_NO_ERROR)
		//	std::cerr << "OpenGL Error1 " << error1 << std::endl;

        asset->m_isFirst = false;
    }
    else {
        glDrawTransformFeedback(GL_POINTS, asset->m_transformFeedback[asset->m_currVB]);
    }
    
    glEndTransformFeedback();

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);

}

static void RenderParticles(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    shaders->use();
    
    shaders->setUniform("gCameraPos", gCamera.position());
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0);//GL_TEXTURE0
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asset->texture->object());
    
    glDisable(GL_RASTERIZER_DISCARD);
    
    glBindBuffer(GL_ARRAY_BUFFER, asset->m_particleBuffer[asset->m_currTFB]);
    
    glEnableVertexAttribArray(shaders->attrib("vert"));
    
    glVertexAttribPointer(shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)4);  // position
   
    glDrawTransformFeedback(GL_POINTS, asset->m_transformFeedback[asset->m_currTFB]);
    
    glDisableVertexAttribArray(shaders->attrib("vert"));
    
    
}

static void RenderParticleInstance(const ModelInstance& inst, float millsElapsed) {
    ModelAsset* asset = inst.asset;
    
    asset->m_time += millsElapsed;
    
    UpdateParticles(inst, millsElapsed);
    
    RenderParticles(inst);
    
    asset->m_currVB = asset->m_currTFB;
    asset->m_currTFB = (asset->m_currTFB + 1) & 0x1;
}

void Render(float millsElapsed, GLFWwindow* window)
{
	// clear everything
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::list<ModelInstance>::const_iterator it;
    for (it = gParticleInstances.begin(); it != gParticleInstances.end(); ++it) {
        RenderParticleInstance(*it, millsElapsed);
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
	window = glfwCreateWindow(800, 600, "Hello World", NULL, NULL);
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

    LoadFireworkAsset();

	CreateInstances();

	//glClearColor(0.196078431372549f, 0.3137254901960784f, 0.5882352941176471f, 1);
	glClearColor(0.0f, 0.0f, 0.0f, 1);


    gCamera.setPosition(glm::vec3(0, 1, 2));
	gCamera.offsetOrientation(0, 0);
	gCamera.setViewportAspectRatio(800.0f / 600.0f);
	gCamera.setNearAndFarPlanes(0.5f, 100.0f);

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
