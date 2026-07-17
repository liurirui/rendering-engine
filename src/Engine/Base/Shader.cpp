

#include <glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include"Shader.h"
#include "Logger.h"

NAMESPACE_START

    // ------------------------------------------------------------------------
    Shader::Shader(const char* vertexCode, const char* fragmentCode, const char* geometryCode, const char* debugName)
    {
        if (debugName && debugName[0] != '\0') {
            debugName_ = debugName;
        }
    
        // 2. compile shaders
        unsigned int vertex, fragment, geometry;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX", vertexCode);
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT", fragmentCode);
        //If geometry shader code is provided, compile the geometry shader
        if (geometryCode != nullptr)
        {
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &geometryCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY", geometryCode);
        }

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometryCode != nullptr)   glAttachShader(ID, geometry);
           
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryCode != nullptr)    glDeleteShader(geometry);    
    }

    Shader::~Shader()
    {
        if (ID != 0) {
            glDeleteProgram(ID);
            ID = 0;
        }
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
    static std::string BuildShaderSourcePreview(const char* sourceCode) {
        if (!sourceCode) {
            return "";
        }

        std::stringstream input(sourceCode);
        std::stringstream preview;
        std::string line;
        int lineNumber = 1;
        const int maxLines = 80;
        while (std::getline(input, line) && lineNumber <= maxLines) {
            preview << lineNumber << ": " << line << "\n";
            lineNumber++;
        }
        if (!input.eof()) {
            preview << "... source truncated after " << maxLines << " lines\n";
        }
        return preview.str();
    }

    void Shader::checkCompileErrors(GLuint shader, const std::string& type, const char* sourceCode)
    {
        GLint success = GL_FALSE;
        GLchar infoLog[4096] = {};
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
                Logger::Error("Shader compile failed. name=" + debugName_ + ", stage=" + type + "\n" + infoLog + "\nSource preview:\n" + BuildShaderSourcePreview(sourceCode));
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, sizeof(infoLog), NULL, infoLog);
                Logger::Error("Shader link failed. name=" + debugName_ + "\n" + infoLog);
            }
        }
    }

NAMESPACE_END

