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
#include "shadow_map_fbo.h"

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600


float gDegreesRotated = 45.0f;
tdogl::Camera gCamera;
tdogl::Camera gCameraFromLight;
double gScrollY = 0.0;

struct ModelAsset {
	tdogl::Program* shaders;
    tdogl::Program* shadersShadowMap;
	tdogl::Texture* texture;
	GLuint vbo;
	GLuint vao;
	GLuint vaoShadowMap;
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

ModelAsset gWoodenCrate;
ModelAsset gGround;

std::list<ModelInstance> gInstances;
std::list<ModelInstance> gInstancesShadowMap;

std::vector<Light> gLights;

ShadowMapFBO gShadowMapFBO;

static tdogl::Program* LoadShaders(const char *shaderFile1, const char *shaderFile2) {
	std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile1, GL_VERTEX_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(shaderFile2, GL_FRAGMENT_SHADER));
	return new tdogl::Program(shaders);
}

static tdogl::Texture* LoadTexture(const char *textureFile) {
	tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(textureFile);
	bmp.flipVertically();
	return new tdogl::Texture(bmp);
}

static void LoadGroundAsset() {
    gGround.shaders = gWoodenCrate.shaders;
    gGround.shadersShadowMap = gWoodenCrate.shadersShadowMap;
    gGround.drawType = GL_TRIANGLES;
    gGround.drawStart = 0;
    gGround.drawCount = 2 * 3;
    gGround.texture = LoadTexture("test.png");
    gGround.shininess = 80.0;
    gGround.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glGenBuffers(1, &gGround.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gGround.vbo);
    
    // Make a quad out of 2 triangles
    GLfloat vertexData[] = {
        //  X     Y     Z       U     V          Normal
        1.0f,0.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f,0.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f,0.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        1.0f,0.0f,1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        1.0f,0.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        -1.0f,0.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glGenVertexArrays(1, &gGround.vao);
	glBindVertexArray(gGround.vao);

    gGround.shaders->use();
    
    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(gGround.shaders->attrib("vert"));
    glVertexAttribPointer(gGround.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);
    
    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(gGround.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gGround.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    
    glEnableVertexAttribArray(gGround.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gGround.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));
    //gGround.shaders->stopUsing();

	//glBindVertexArray(0);


	glGenVertexArrays(1, &gGround.vaoShadowMap);
	glBindVertexArray(gGround.vaoShadowMap);

    gGround.shadersShadowMap->use();
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    // connect the xyz to the "vert" attribute of the vertex shader
	GLuint index = gGround.shadersShadowMap->attrib("vert");
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);
    
    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    index = gGround.shadersShadowMap->attrib("vertTexCoord");
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    //gGround.shadersShadowMap->stopUsing();
    
    // unbind the VAO
    //glBindVertexArray(0);
}

static void LoadWoodenCrateAsset() {
	gWoodenCrate.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    gWoodenCrate.shadersShadowMap = LoadShaders("shadow_map.vs", "shadow_map.fs");
	gWoodenCrate.drawType = GL_TRIANGLES;
	gWoodenCrate.drawStart = 0;
	gWoodenCrate.drawCount = 6 * 2 * 3;
	gWoodenCrate.texture = LoadTexture("wooden-crate.jpg");
	gWoodenCrate.shininess = 80.0;
	gWoodenCrate.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	
	glGenBuffers(1, &gWoodenCrate.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gWoodenCrate.vbo);

	// Make a cube out of triangles (two triangles per side)
	GLfloat vertexData[] = {
		//  X     Y     Z       U     V          Normal
		// bottom
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,

		// top
		-1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

		// front
		-1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,

		// back
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,

		// left
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,

		// right
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
	};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glGenVertexArrays(1, &gWoodenCrate.vao);
	glBindVertexArray(gWoodenCrate.vao);

    gWoodenCrate.shaders->use();
    // connect the xyz to the "vert" attribute of the vertex shader
	GLuint indexVert = gWoodenCrate.shaders->attrib("vert");
	glEnableVertexAttribArray(indexVert);
	glVertexAttribPointer(indexVert, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

	// connect the uv coords to the "vertTexCoord" attribute of the vertex shader
	GLuint indexTexCor = gWoodenCrate.shaders->attrib("vertTexCoord");
	glEnableVertexAttribArray(indexTexCor);
	glVertexAttribPointer(indexTexCor, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

	GLuint indexNormal = gWoodenCrate.shaders->attrib("vertNormal");
	glEnableVertexAttribArray(indexNormal);
	glVertexAttribPointer(indexNormal, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));
    //gWoodenCrate.shaders->stopUsing();

	//glBindVertexArray(0);



	glGenVertexArrays(1, &gWoodenCrate.vaoShadowMap);
	glBindVertexArray(gWoodenCrate.vaoShadowMap);
	
	gWoodenCrate.shadersShadowMap->use();
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    // connect the xyz to the "vert" attribute of the vertex shader
	GLuint indexVertMap = gWoodenCrate.shadersShadowMap->attrib("vert");
    glEnableVertexAttribArray(indexVertMap);
    glVertexAttribPointer(indexVertMap, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);
    
    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
	GLuint indexTexCoord = gWoodenCrate.shadersShadowMap->attrib("vertTexCoord");
    glEnableVertexAttribArray(indexTexCoord);
    glVertexAttribPointer(indexTexCoord, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    //gWoodenCrate.shadersShadowMap->stopUsing();
    
    // unbind the VAO
	//glBindVertexArray(0);
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
	dot.transform = glm::mat4();
	gInstances.push_back(dot);
    gInstancesShadowMap.push_back(dot);

	ModelInstance i;
	i.asset = &gWoodenCrate;
	i.transform = translate(0, -4, 0) * scale(1, 2, 1);
	gInstances.push_back(i);
    gInstancesShadowMap.push_back(i);

	ModelInstance hLeft;
	hLeft.asset = &gWoodenCrate;
	hLeft.transform = translate(-8, 0, 0) * scale(1, 6, 1);
	gInstances.push_back(hLeft);
    gInstancesShadowMap.push_back(hLeft);

	ModelInstance hRight;
	hRight.asset = &gWoodenCrate;
	hRight.transform = translate(-4, 0, 0) * scale(1, 6, 1);
	gInstances.push_back(hRight);
    gInstancesShadowMap.push_back(hRight);

	ModelInstance hMid;
	hMid.asset = &gWoodenCrate;
	hMid.transform = translate(-6, 0, 0) * scale(2, 1, 0.8f);
	gInstances.push_back(hMid);
    gInstancesShadowMap.push_back(hMid);
    
    ModelInstance ground;
    ground.asset = &gGround;
    float groundScale = 20.0;
    ground.transform = translate(-4, -6, 0) * scale(groundScale, groundScale, groundScale);
    gInstances.push_back(ground);
    
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
	gInstances.front().transform = glm::rotate(glm::mat4(), gDegreesRotated, glm::vec3(0, 1, 0));
    gInstancesShadowMap.front().transform = gInstances.front().transform;

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
        gCameraFromLight = gCamera;
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

static void RenderInstanceMap(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shadersShadowMap;
    
    //bind the shaders
    shaders->use();

    //set the shader uniforms
    shaders->setUniform("camera", gCameraFromLight.matrix());
    shaders->setUniform("model", inst.transform);

    //bind VAO and draw
    glBindVertexArray(asset->vao);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);
    
    //unbind everything
    //glBindVertexArray(0);

    //shaders->stopUsing();
}

static void RenderInstance1(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shadersShadowMap;
    
    //bind the shaders
    shaders->use();
    
    shaders->setUniform("gShadowMap", 1);
    
    //set the shader uniforms
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("model", inst.transform);
    
    //bind VAO and draw
    glBindVertexArray(asset->vaoShadowMap);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);
    
    //unbind everything
    //glBindVertexArray(0);
    
    //shaders->stopUsing();
}

static void RenderInstance(const ModelInstance& inst) {
	ModelAsset* asset = inst.asset;
	tdogl::Program* shaders = asset->shaders;

	//bind the shaders
	shaders->use();
	
	shaders->setUniform("gShadowMap", 1);

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
	shaders->setUniform("cameraFromLight", gCameraFromLight.matrix());
	shaders->setUniform("camera", gCamera.matrix());
	shaders->setUniform("model", inst.transform);
	shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0

	shaders->setUniform("materialShininess", asset->shininess);
	shaders->setUniform("materialSpecularColor", asset->specularColor);


								   //bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, asset->texture->object());

	//bind VAO and draw
	glBindVertexArray(asset->vao);
	glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);

	//unbind everything
	//glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//shaders->stopUsing();
}

bool Init()
{
    if (!gShadowMapFBO.Init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        return false;
    }
    return true;
}

void ShadowMapPass()
{
    gShadowMapFBO.BindForWriting();
    glClear(GL_DEPTH_BUFFER_BIT);
    
    std::list<ModelInstance>::const_iterator it;
    for (it = gInstancesShadowMap.begin(); it != gInstancesShadowMap.end(); ++it) {
        RenderInstanceMap(*it);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    gShadowMapFBO.BindForReading(GL_TEXTURE1);
    RenderInstance1(gInstances.back());

	//std::list<ModelInstance>::const_iterator it;
	//for (it = gInstances.begin(); it != gInstances.end(); ++it) {
	//	RenderInstance(*it);
	//}
    
}

void Render(GLFWwindow* window)
{
    ShadowMapPass();
    
    RenderPass();

	glfwSwapBuffers(window);
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

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

    
    if (!Init()) {
        return -1;
    }
    
	LoadWoodenCrateAsset();
    LoadGroundAsset();

	CreateInstances();

	//glClearColor(0.196078431372549f, 0.3137254901960784f, 0.5882352941176471f, 1);
	glClearColor(0.0f, 0.0f, 0.0f, 1);


	gCamera.setPosition(glm::vec3(-4, 0, 17));
	gCamera.setViewportAspectRatio(WINDOW_WIDTH / WINDOW_HEIGHT);
	gCamera.setNearAndFarPlanes(0.5f, 100.0f);
    gCameraFromLight = gCamera;

	// setup lights
	Light spotlight;
	spotlight.position = glm::vec4(-4, 0, 10, 1);
	spotlight.intensities = glm::vec3(2, 2, 2); //strong white light
	spotlight.attenuation = 0.01f;
	spotlight.ambientCoefficient = 0.0f; //no ambient light
	spotlight.coneAngle = 15.0f;
	spotlight.coneDirection = glm::vec3(0, 0, -1);
    gCameraFromLight.setPosition(glm::vec3(spotlight.position));
    gCameraFromLight.lookAt(glm::vec3(spotlight.position) + spotlight.coneDirection);
    

	Light directionalLight;
	directionalLight.position = glm::vec4(1, 0.8, 0.6, 0); //w == 0 indications a directional light
	directionalLight.intensities = glm::vec3(0.01, 0.01, 0.01); //weak light
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
