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
    std::vector<Mesh::MeshEntry> m_Entries;
    std::vector<Texture*> m_Textures;
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

ModelAsset gWoodenCrate;
std::list<ModelInstance> gInstances;

std::vector<Light> gLights;

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

static void LoadWoodenCrateAsset() {
	gWoodenCrate.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
	gWoodenCrate.drawType = GL_TRIANGLES;
	gWoodenCrate.drawStart = 0;
	gWoodenCrate.drawCount = 6 * 2 * 3;
	gWoodenCrate.texture = LoadTexture("wooden-crate.jpg");
	gWoodenCrate.shininess = 80.0;
	gWoodenCrate.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	
	glGenBuffers(1, &gWoodenCrate.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gWoodenCrate.vbo);

	glGenVertexArrays(1, &gWoodenCrate.vao);
	glBindVertexArray(gWoodenCrate.vao);
    
    Mesh::LoadMesh("model.obj", gWoodenCrate.m_Entries, gWoodenCrate.m_Textures);

	// Make a cube out of triangles (two triangles per side)
//	GLfloat vertexData[] = {
//		//  X     Y     Z       U     V          Normal
//		// bottom
//		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
//		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
//		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
//		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
//		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
//		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
//
//		// top
//		-1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
//		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
//		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
//		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
//		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
//		1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
//
//		// front
//		-1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
//		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
//		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
//		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
//		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
//		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
//
//		// back
//		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
//		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
//		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
//		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
//		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
//		1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,
//
//		// left
//		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
//		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
//		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
//		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
//		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
//		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
//
//		// right
//		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
//		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
//		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
//		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
//		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
//		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
//	};
//	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	// connect the xyz to the "vert" attribute of the vertex shader
	glEnableVertexAttribArray(gWoodenCrate.shaders->attrib("vert"));
	glVertexAttribPointer(gWoodenCrate.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

	// connect the uv coords to the "vertTexCoord" attribute of the vertex shader
	glEnableVertexAttribArray(gWoodenCrate.shaders->attrib("vertTexCoord"));
	glVertexAttribPointer(gWoodenCrate.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

	glEnableVertexAttribArray(gWoodenCrate.shaders->attrib("vertNormal"));
	glVertexAttribPointer(gWoodenCrate.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

	// unbind the VAO
	glBindVertexArray(0);
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
	ModelInstance dot;
	dot.asset = &gWoodenCrate;
	dot.transform = glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1, 0, 0));//glm::mat4();
	gInstances.push_back(dot);

//	ModelInstance i;
//	i.asset = &gWoodenCrate;
//	i.transform = translate(0, -4, 0) * scale(1, 2, 1);
//	gInstances.push_back(i);
//
//	ModelInstance hLeft;
//	hLeft.asset = &gWoodenCrate;
//	hLeft.transform = translate(-8, 0, 0) * scale(1, 6, 1);
//	gInstances.push_back(hLeft);
//
//	ModelInstance hRight;
//	hRight.asset = &gWoodenCrate;
//	hRight.transform = translate(-4, 0, 0) * scale(1, 6, 1);
//	gInstances.push_back(hRight);
//
//	ModelInstance hMid;
//	hMid.asset = &gWoodenCrate;
//	hMid.transform = translate(-6, 0, 0) * scale(2, 1, 0.8f);
//	gInstances.push_back(hMid);
}

// records how far the y axis has been scrolled
void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
	gScrollY += deltaY;
}

void Update(float secondsElapsed, GLFWwindow* window) {
	const GLfloat degreesPerSecond = 180.0f;
	//const GLfloat degreesPerSecond = 0.0f;
	gDegreesRotated += secondsElapsed * degreesPerSecond;
	while (gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;
	gInstances.front().transform = glm::rotate(glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1, 0, 0)), glm::radians(gDegreesRotated), glm::vec3(0, 0, 1));

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


								   //bind the texture
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, asset->texture->object());

	//bind VAO and draw
	glBindVertexArray(asset->vao);
	//glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);
    
    for (unsigned int i = 0; i < asset->m_Entries.size(); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, asset->m_Entries[i].VB);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asset->m_Entries[i].IB);
        
        const unsigned int MaterialIndex = asset->m_Entries[i].MaterialIndex;
        
        if (MaterialIndex < asset->m_Textures.size() && asset->m_Textures[MaterialIndex]) {
            //bind the texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, asset->m_Textures[MaterialIndex]->object());
//            asset->m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
        }
        
        glDrawElements(GL_TRIANGLES, asset->m_Entries[i].NumIndices, GL_UNSIGNED_INT, 0);
    }

	//unbind everything
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	shaders->stopUsing();
}

void Render(GLFWwindow* window)
{
	// clear everything
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	std::list<ModelInstance>::const_iterator it;
	for (it = gInstances.begin(); it != gInstances.end(); ++it) {
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

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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

	LoadWoodenCrateAsset();

	CreateInstances();

	//glClearColor(0.196078431372549f, 0.3137254901960784f, 0.5882352941176471f, 1);
	glClearColor(0.0f, 0.0f, 0.0f, 1);


    gCamera.setPosition(glm::vec3(0, 0, 5));
    gCamera.setViewportAspectRatio(800.0f / 600.0f);
    gCamera.setNearAndFarPlanes(0.01f, 1000.0f);

	// setup lights
	Light spotlight;
	spotlight.position = glm::vec4(-4, 0, 10, 1);
	spotlight.intensities = glm::vec3(2, 2, 2); //strong white light
	spotlight.attenuation = 0.1f;
	spotlight.ambientCoefficient = 0.0f; //no ambient light
	spotlight.coneAngle = 15.0f;
	spotlight.coneDirection = glm::vec3(0, 0, -1);

	Light directionalLight;
	directionalLight.position = glm::vec4(1, 0.8, 0.6, 0); //w == 0 indications a directional light
	directionalLight.intensities = glm::vec3(0.4, 0.3, 0.1); //weak yellowish light
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
