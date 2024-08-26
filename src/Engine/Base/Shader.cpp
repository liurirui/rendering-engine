

#include <glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include"Shader.h"

NAMESPACE_START

    // ------------------------------------------------------------------------
    Shader::Shader(const char* vertexCode, const char* fragmentCode)
    {
    
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);

    }
    // activate the shader
    // ------------------------------------------------------------------------
    void Shader::use() const
    { 
        glUseProgram(ID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void Shader::setBool(const char* name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(ID, name), (int)value); 
    }
    // ------------------------------------------------------------------------
    void Shader::setInt(const char* name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ID, name), value); 
    }
    // ------------------------------------------------------------------------
    void Shader::setFloat(const char* name, float value) const
    { 
        glUniform1f(glGetUniformLocation(ID, name), value); 
    }
    // ------------------------------------------------------------------------
    void Shader::setVec2(const char* name, const glm::vec2 &value) const
    { 
        glUniform2fv(glGetUniformLocation(ID, name), 1, &value[0]); 
    }
    void Shader::setVec2(const char* name, float x, float y) const
    { 
        glUniform2f(glGetUniformLocation(ID, name), x, y); 
    }
    // ------------------------------------------------------------------------
    void Shader::setVec3(const char* name, const glm::vec3 &value) const
    { 
        glUniform3fv(glGetUniformLocation(ID, name), 1, &value[0]); 
    }
    void Shader::setVec3(const char* name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(ID, name), x, y, z); 
    }
    // ------------------------------------------------------------------------
    void Shader::setVec4(const char* name, const glm::vec4 &value) const
    { 
        glUniform4fv(glGetUniformLocation(ID, name), 1, &value[0]); 
    }
    void Shader::setVec4(const char* name, float x, float y, float z, float w) const
    { 
        glUniform4f(glGetUniformLocation(ID, name), x, y, z, w); 
    }
    // ------------------------------------------------------------------------
    void Shader::setMat2(const char * name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void Shader::setMat3(const char*  name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void Shader::setMat4(const char * name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
    }

    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void Shader::checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }

NAMESPACE_END

