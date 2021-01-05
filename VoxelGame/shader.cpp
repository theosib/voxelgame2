#include "gl_includes.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "filelocator.hpp"

static unsigned int compileShader(int shader_type, const char *code)
{
    int  success;
    int shaderId;
    
    shaderId = glCreateShader(shader_type);
    glShaderSource(shaderId, 1, &code, NULL);
    glCompileShader(shaderId);
    
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
        fprintf(stderr, "Error compiling shader: %s\n", infoLog);
        exit(-1);
    }
        
    return shaderId;
}

static unsigned int setupShaders(const char *vertexShaderSource, const char *fragmentShaderSource)
{
    unsigned int fragmentShader, vertexShader;
    unsigned int shaderProgram;
    int  success;
    
    std::cout << "Compiling vertex shader\n";
    vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    std::cout << "Compiling fragment shader\n";
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "Error linking shaders: %s\n", infoLog);
        exit(-1);
    }
    
    // glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);  
    return shaderProgram;
}
  
Shader::Shader(const char* vertex, const char* fragment)
{
    ID = 0;
    
    std::string vertexPath = FileLocator::instance.shader(vertex);
    std::string fragmentPath = FileLocator::instance.shader(fragment);
    
    std::cout << "vertex: " << vertexPath << " frag: " << fragmentPath << std::endl;
    
    // 1. retrieve the vertex/fragment source code from filePath
    // std::string vertexCode;
    // std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();		
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode   = vShaderStream.str();
        fragmentCode = fShaderStream.str();		
    } catch(std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        exit(-1);
    }
}

void Shader::compile()
{
    const char* vertexShaderCode = vertexCode.c_str();
    const char* fragmentShaderCode = fragmentCode.c_str();
    ID = setupShaders(vertexShaderCode, fragmentShaderCode);
}

void Shader::use()
{
    if (!ID) compile();
    glUseProgram(ID);
}  

void Shader::setBool(const std::string &name, bool value)
{   
    use();
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
}

void Shader::setInt(const std::string &name, int value)
{ 
    use();
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
}

void Shader::setFloat(const std::string &name, float value)
{ 
    use();
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
} 

void Shader::setMat4(const std::string &name, const glm::mat4& matrix)
{
    use();
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix)); 
}
