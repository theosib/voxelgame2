#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shader
{
private:
    unsigned int ID;
    std::string vertexCode;
    std::string fragmentCode;
  
public:
    // constructor reads and builds the shader
    Shader(const char* vertexPath, const char* fragmentPath);
    
    // use/activate the shader
    void compile();
    void use();
    
    // utility uniform functions
    void setBool(const std::string &name, bool value);  
    void setInt(const std::string &name, int value);   
    void setFloat(const std::string &name, float value);
    void setMat4(const std::string &name, const glm::mat4& matrix);
};
  
#endif
