//
//  OpenGLView.m
//  Tutorial01
//
//  Created by aa64mac on 01/11/2016.
//  Copyright © 2016 cyj. All rights reserved.
//

#import "OpenGLView.h"

#include "tdogl/Program.h"
#include "ResourcePath/ResourcePath.hpp"
#include "tdogl/Texture.h"
#include "common/thirdparty/stb_image/stb_image.h"
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "tdogl/Camera.h"
#include <list>
#include <sstream>
#include "shadow_map_fbo.h"

struct Light {
    glm::vec4 position;
    glm::vec3 intensities; //a.k.a. the color of the light
    float attenuation;
    float ambientCoefficient;
    float coneAngle;
    glm::vec3 coneDirection;
};

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

@interface OpenGLView() {
    ModelAsset gWoodenCrate;
    ModelAsset gGround;
    std::list<ModelInstance> gInstances;
    std::list<ModelInstance> gInstancesShadowMap;
    
    GLfloat gDegreesRotated;
    CADisplayLink * _displayLink;
    tdogl::Camera gCamera;
    tdogl::Camera gCameraFromLight;
    
    //Light gLight;
    std::vector<Light> gLights;
    
    ShadowMapFBO gShadowMapFBO;
    
    float delta;
}

- (void)setupLayer;
- (void)setupContext;
- (void)destoryRenderAndFrameBuffer;

@end

@implementation OpenGLView

+ (Class)layerClass {
    // 只有 [CAEAGLLayer class] 类型的 layer 才支持在其上描绘 OpenGL 内容。
    return [CAEAGLLayer class];
}

- (void)setupLayer
{
    _eaglLayer = (CAEAGLLayer*) self.layer;
    
    // CALayer 默认是透明的，必须将它设为不透明才能让其可见
    _eaglLayer.opaque = YES;
    
    // 设置描绘属性，在这里设置不维持渲染内容以及颜色格式为 RGBA8
    _eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                     [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
}

- (void)setupContext {
    // 指定 OpenGL 渲染 API 的版本，在这里我们使用 OpenGL ES 3.0
    EAGLRenderingAPI api = kEAGLRenderingAPIOpenGLES3;
    _context = [[EAGLContext alloc] initWithAPI:api];
    if (!_context) {
        NSLog(@"Failed to initialize OpenGLES 3.0 context");
        exit(1);
    }
    
    // 设置为当前上下文
    if (![EAGLContext setCurrentContext:_context]) {
        NSLog(@"Failed to set current OpenGL context");
        exit(1);
    }
}

- (void)destoryRenderAndFrameBuffer
{
    if (_colorRenderBuffer != 0) {
        glDeleteRenderbuffers(1, &_colorRenderBuffer);
        _colorRenderBuffer = 0;
    }
    
    if (_frameBuffer != 0) {
        glDeleteFramebuffers(1, &_frameBuffer);
        _frameBuffer = 0;
    }
    
    if (_depthRenderBuffer != 0) {
        glDeleteFramebuffers(1, &_depthRenderBuffer);
        _depthRenderBuffer = 0;
    }
}

- (void)layoutSubviews {
    
    [self setupLayer];
    
    [self setupContext];

    [self destoryRenderAndFrameBuffer];
    
    [self setupBuffers];
    
    [self InitFBO];
    
    [self LoadWoodenCrateAsset];
    [self LoadGroundAsset];
    
    [self CreateInstances];
    
    [self InitGL];
    
    [self Render];
}

- (void)setupBuffers
{
    // Setup frame buffer
    //
    glGenFramebuffers(1, &_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    
    // Setup color render buffer
    //
    glGenRenderbuffers(1, &_colorRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderBuffer);
    [_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:_eaglLayer];
    
    // Attach color render buffer and depth render buffer to frameBuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, _colorRenderBuffer);
    
    
    CGSize size = [self getFrameBufferSize];
    
    // Setup depth render buffer
    //
    
    // Create a depth buffer that has the same size as the color buffer.
    glGenRenderbuffers(1, &_depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size.width, size.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, _depthRenderBuffer);
    
    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Status != GL_FRAMEBUFFER_COMPLETE) {
        printf("FB error, status: 0x%x\n", Status);
        exit(1);
    }

}

- (CGSize)getFrameBufferSize
{
    int width, height;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
    return CGSizeMake(width, height);
}

- (void)InitFBO
{
    CGSize size = [self getFrameBufferSize];

    gShadowMapFBO.Init(size.width, size.height);
    
    
    
    glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
}

- (void)InitGL
{
    NSLog(@"OpenGL version: %s", glGetString(GL_VERSION));
    NSLog(@"GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    NSLog(@"Vendor: %s", glGetString(GL_VENDOR));
    NSLog(@"Renderer: %s", glGetString(GL_RENDERER));
    
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);
    //glDepthFunc(GL_GREATER);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0, 0, 0, 1); // black
    
    CGFloat width = self.frame.size.width;
    CGFloat height = self.frame.size.height;
    
    glViewport(0, 0, width, height);
    
    gCamera.setPosition(glm::vec3(-4, 0, 17));
    gCamera.setViewportAspectRatio(width / height);
    gCamera.setNearAndFarPlanes(0.1, 5000);
    gCameraFromLight = gCamera;
    
    Light spotlight;
    spotlight.position = glm::vec4(-4,0,10,1);
    spotlight.intensities = glm::vec3(2,2,2); //strong white light
    spotlight.attenuation = 0.001f;
    spotlight.ambientCoefficient = 0.0f; //no ambient light
    spotlight.coneAngle = 15.0f;
    spotlight.coneDirection = glm::vec3(0,0,-1);
    gCameraFromLight.setPosition(glm::vec3(spotlight.position));
    gCameraFromLight.lookAt(glm::vec3(spotlight.position) + spotlight.coneDirection);
    
    Light directionalLight;
    directionalLight.position = glm::vec4(1, 0.8, 0.6, 0); //w == 0 indications a directional light
    directionalLight.intensities = glm::vec3(0.2, 0.2, 0.2); //weak light
    directionalLight.ambientCoefficient = 0.06;
    
    gLights.push_back(spotlight);
    gLights.push_back(directionalLight);
}

glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
    return glm::translate(glm::mat4(), glm::vec3(x, y, z));
}

glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
    return glm::scale(glm::mat4(), glm::vec3(x, y, z));
}

- (void) CreateInstances {
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

- (void)LoadGroundAsset
{
    gGround.shaders = gWoodenCrate.shaders;
    gGround.shadersShadowMap = gWoodenCrate.shadersShadowMap;
    gGround.drawType = GL_TRIANGLES;
    gGround.drawStart = 0;
    gGround.drawCount = 2 * 3;
    gGround.texture = [self LoadTexture:"test" ext:"png"];
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
    gGround.shaders->stopUsing();
    
    glBindVertexArray(0);
    
    
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
    gGround.shadersShadowMap->stopUsing();
    
    // unbind the VAO
    glBindVertexArray(0);

    
}


- (void)LoadWoodenCrateAsset
{
    gWoodenCrate.shaders = [self LoadShaders:"VertexShader" fs:"FragmentShader"];
    gWoodenCrate.shadersShadowMap = [self LoadShaders:"shadow_map_v" fs:"shadow_map_f"];
    gWoodenCrate.drawType = GL_TRIANGLES;
    gWoodenCrate.drawStart = 0;
    gWoodenCrate.drawCount = 6*2*3;
    gWoodenCrate.texture = [self LoadTexture:"wooden-crate" ext:"jpg"];
    gWoodenCrate.shininess = 80.0;
    gWoodenCrate.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    // make and bind the VBO
    glGenBuffers(1, &gWoodenCrate.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gWoodenCrate.vbo);

    // Put the three triangle verticies into the VBO
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
    glEnableVertexAttribArray(gWoodenCrate.shaders->attrib("vert"));
    glVertexAttribPointer(gWoodenCrate.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), NULL);
    
    glEnableVertexAttribArray(gWoodenCrate.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gWoodenCrate.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    
    glEnableVertexAttribArray(gWoodenCrate.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gWoodenCrate.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));
    gWoodenCrate.shaders->stopUsing();
    
    glBindVertexArray(0);

    
    
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
    gWoodenCrate.shadersShadowMap->stopUsing();
    
    // unbind the VAO
    glBindVertexArray(0);
    
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
}

- (tdogl::Texture *)LoadTexture: (const char *)filename ext: (const char *)ext
{
    tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(filename, ext));
    bmp.flipVertically();
    return new tdogl::Texture(bmp);
}

- (tdogl::Program *)LoadShaders: (const char *)vShader fs:(const char *)fShader
{
    std::vector<tdogl::Shader> shaders;
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(vShader, "glsl"), GL_VERTEX_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(fShader, "glsl"), GL_FRAGMENT_SHADER));
    return new tdogl::Program(shaders);
}

template <typename T>
void SetLightUniform(tdogl::Program* shaders, const char* propertyName, size_t lightIndex, const T& value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();
    
    shaders->setUniform(uniformName.c_str(), value);
}

- (void) RenderInstance: (const ModelInstance&)inst
{
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    GLint gShadowMapSlot = glGetUniformLocation(shaders->object(), "gShadowMap");
    glUniform1i(gShadowMapSlot, 1);
    
    GLint numLightsSlot = glGetUniformLocation(shaders->object(), "numLights");
    glUniform1i(numLightsSlot, (int)gLights.size());
    
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

    glm::mat3 normalMatrix3 = glm::transpose(glm::inverse(glm::mat3(inst.transform)));
    shaders->setUniform("normalMatrix", normalMatrix3);
    
    GLint materialTexSlot = glGetUniformLocation(shaders->object(), "materialTex");
    glUniform1i(materialTexSlot, 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    
    shaders->setUniform("materialShininess", asset->shininess);
    shaders->setUniform("materialSpecularColor", asset->specularColor);
    GLint deltaSlot = glGetUniformLocation(shaders->object(), "delta");
    glUniform1f(deltaSlot, delta);
    //glUniform1f(deltaSlot, 0.0003);
    
    
    //bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asset->texture->object());
    
    //bind VAO and draw
    glBindVertexArray(asset->vao);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);
    
    //unbind everything
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shaders->stopUsing();
}

- (void) RenderInstance1: (const ModelInstance&)inst
{
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shadersShadowMap;
    
    //bind the shaders
    shaders->use();
    
    GLint gShadowMapSlot = glGetUniformLocation(shaders->object(), "gShadowMap");
    glUniform1i(gShadowMapSlot, 1);
    
    
    //set the shader uniforms
    shaders->setUniform("camera", gCamera.matrix());
    shaders->setUniform("model", inst.transform);
    
    //bind VAO and draw
    glBindVertexArray(asset->vaoShadowMap);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);
    
    //unbind everything
    glBindVertexArray(0);
    
    shaders->stopUsing();
}

- (void) RenderInstanceMap: (const ModelInstance&)inst
{
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shadersShadowMap;
    
    //bind the shaders
    shaders->use();
    
    //set the shader uniforms
    shaders->setUniform("camera", gCameraFromLight.matrix());
    shaders->setUniform("model", inst.transform);
    
    //bind VAO and draw
    glBindVertexArray(asset->vaoShadowMap);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);
    
    //unbind everything
    glBindVertexArray(0);
    
    shaders->stopUsing();
}

- (void)ShadowMapPass
{
    gShadowMapFBO.BindForWriting();
    glClear(GL_DEPTH_BUFFER_BIT);
    
    std::list<ModelInstance>::const_iterator it;
    for (it = gInstancesShadowMap.begin(); it != gInstancesShadowMap.end(); ++it) {
        [self RenderInstanceMap:*it];
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    
}

- (void)RenderPass
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    gShadowMapFBO.BindForReading(GL_TEXTURE1);
    //[self RenderInstance1:gInstances.back()];
    
    std::list<ModelInstance>::const_iterator it;
    for (it = gInstances.begin(); it != gInstances.end(); ++it) {
        [self RenderInstance:*it];
    }
}


- (void)Render
{
    [self ShadowMapPass];
    
    [self RenderPass];
    
    [_context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)Update: (float)secondsElapsed
{
    const GLfloat degreesPerSecond = 90.0f;
    gDegreesRotated += secondsElapsed * degreesPerSecond;
    
    //don't go over 360 degrees
    while(gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;
    gInstances.front().transform = glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0,1,0));
    gInstancesShadowMap.front().transform = gInstances.front().transform;


    //move position of camera based on WASD keys
    const float moveSpeed = 2.0; //units per second
    if (self.backwardButton.highlighted){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.forward());
    } else if (self.forwardButton.highlighted){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.forward());
    }
    if (self.leftButton.highlighted){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.right());
    } else if (self.rightButton.highlighted){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.right());
    }
    if (self.downButton.highlighted){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -glm::vec3(0,1,0));
    } else if (self.upButton.highlighted){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * glm::vec3(0,1,0));
    }
    
    if(self.lightPositionButton.highlighted) {
        gLights[0].position = glm::vec4(gCamera.position(), 1.0);
        gLights[0].coneDirection = gCamera.forward();
        gCameraFromLight = gCamera;
    }
    
    if(self.lightRedButton.highlighted)
        gLights[0].intensities = glm::vec3(1,0,0); //red
    else if(self.lightBlueButton.highlighted)
        gLights[0].intensities = glm::vec3(0,0,1); //blue
    else if(self.lightWhiteButton.highlighted)
        gLights[0].intensities = glm::vec3(1,1,1); //white
    
    const float mouseSensitivity = 0.1f;
    gCamera.offsetOrientation(mouseSensitivity * self.moveY, mouseSensitivity * self.moveX);
    
    float fieldOfView = 50;
    fieldOfView *= self.scale;
    if(fieldOfView < 5.0f) fieldOfView = 5.0f;
    if(fieldOfView > 130.0f) fieldOfView = 130.0f;
    gCamera.setFieldOfView(fieldOfView);
    
    delta = self.deltaSlider.value;

}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {

        
        [self toggleDisplayLink];
        
    }
    
    return self;
}

- (void)toggleDisplayLink
{
    if (_displayLink == nil) {
        _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkCallback:)];
        [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    }
    else {
        [_displayLink invalidate];
        [_displayLink removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        _displayLink = nil;
    }
}

- (void)displayLinkCallback:(CADisplayLink*)displayLink
{
    [self Update:displayLink.duration];
    
    [self Render];
}

@end
