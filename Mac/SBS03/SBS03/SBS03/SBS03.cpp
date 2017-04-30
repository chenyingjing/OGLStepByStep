#include "glew.h"
#include "glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "ogldev_math_3d.h"
#include "filePath.h"

GLuint gVAO = 0;

GLuint VBO;

static void CreateVertexBuffer()
{
    Vector3f Vertices[3];
    Vertices[0] = Vector3f(-1.0f, -1.0f, 0.0f);
    Vertices[1] = Vector3f(1.0f, -1.0f, 0.0f);
    Vertices[2] = Vector3f(0.0f, 1.0f, 0.0f);
    
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &gVAO);
    glBindVertexArray(gVAO);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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

void init()
{
    GLuint program = glCreateProgram();
    
    
    {
        GLuint shader = glCreateShader(GL_VERTEX_SHADER);
        std::string shaderFilePath = ResourcePath("vertex-shader.txt");
        const GLchar* source = ReadShader(shaderFilePath.c_str());
        if (source == NULL)
        {
            return;
        }
        glShaderSource(shader, 1, &source, NULL);
        delete[] source;
        glCompileShader(shader);
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            std::string msg("Compile failure in shader:\n");
            
            GLint infoLogLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
            char* strInfoLog = new char[infoLogLength + 1];
            glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
            msg += strInfoLog;
            delete[] strInfoLog;
            
            glDeleteShader(shader); shader = 0;
            throw std::runtime_error(msg);
        }
        glAttachShader(program, shader);
    }
    
    {
        GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
        std::string shaderFilePath = ResourcePath("fragment-shader.txt");
        const GLchar* source = ReadShader(shaderFilePath.c_str());
        if (source == NULL)
        {
            return;
        }
        glShaderSource(shader, 1, &source, NULL);
        delete[] source;
        glCompileShader(shader);
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            std::string msg("Compile failure in shader:\n");
            
            GLint infoLogLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
            char* strInfoLog = new char[infoLogLength + 1];
            glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
            msg += strInfoLog;
            delete[] strInfoLog;
            
            glDeleteShader(shader); shader = 0;
            throw std::runtime_error(msg);
        }
        glAttachShader(program, shader);
    }
    
    glLinkProgram(program);
    
    //glDetachShader(program, shader);
    
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        std::string msg("Program linking failure: ");
        
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* strInfoLog = new char[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        msg += strInfoLog;
        delete[] strInfoLog;
        
        glDeleteProgram(program); program = 0;
        throw std::runtime_error(msg);
    }
    
    glUseProgram(program);
    
    glClearColor(0.6784313725490196, 0.607843137254902, 0.3215686274509804, 1);
}

void Render(GLFWwindow* window)
{

    // clear everything
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindVertexArray(gVAO);
    
    // draw the VAO
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    glBindVertexArray(0);
    
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
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
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
    
    if(!GLEW_VERSION_3_2)
        throw std::runtime_error("OpenGL 3.2 API is not available.");
    
    init();
    CreateVertexBuffer();
    
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Poll for and process events */
        glfwPollEvents();
        
        Render(window);
    }
    
    glfwTerminate();
    return 0;
}
