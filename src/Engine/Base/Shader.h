#ifndef SHADER_H
#define SHADER_H

#include <glad.h>
#include<glm/glm.hpp>
#include "Object.h"
#include<string>

NAMESPACE_START

class Shader
{
public:
   
    unsigned int ID = 0;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexCode, const char* fragmentCode, const char* geometryCode = nullptr, const char* debugName = nullptr);

    Shader(){}
    ~Shader();

    // 编译候选 program，全部阶段和链接成功后才替换当前 ID。热重载失败时旧 program
    // 保持可用，避免一次保存中的语法错误直接让运行画面变黑。
    bool compileAndReplace(const char* vertexCode, const char* fragmentCode, const char* geometryCode = nullptr);
    bool isValid() const { return ID != 0; }
    const std::string& getDebugName() const { return debugName_; }
    void setDebugName(std::string debugName) { debugName_ = std::move(debugName); }

    // activate the shader
    // ------------------------------------------------------------------------
    void use() const;
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const char * name, bool value) const;
    // ------------------------------------------------------------------------
    void setInt(const char* name, int value) const;
    // ------------------------------------------------------------------------
    void setFloat(const char* name, float value) const;
    // ------------------------------------------------------------------------
    void setVec2(const char* name, const glm::vec2& value) const;

    void setVec2(const char* name, float x, float y) const;
    // ------------------------------------------------------------------------
    void setVec3(const char* name, const glm::vec3& value) const;

    void setVec3(const char* name, float x, float y, float z) const;
    // ------------------------------------------------------------------------
    void setVec4(const char* name, const glm::vec4& value) const;

    void setVec4(const char* name, float x, float y, float z, float w) const;

    // ------------------------------------------------------------------------
    void setMat2(const char* name, const glm::mat2& mat) const;

    // ------------------------------------------------------------------------
    void setMat3(const char* name, const glm::mat3& mat) const;

    // ------------------------------------------------------------------------
    void setMat4(const char* name, const glm::mat4& mat) const;

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    bool checkCompileErrors(GLuint shader, const std::string& type, const char* sourceCode = nullptr);

    std::string debugName_ = "UnnamedShader";

};

NAMESPACE_END

#endif
