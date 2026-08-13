#pragma once

#include <Base/Constants.h>
#include <memory>
#include <string>

NAMESPACE_START

class Shader;
class ShaderLibrary;

class IBLSystem {
public:
    IBLSystem();
    ~IBLSystem();

    bool initialize(const std::string& hdrPath, ShaderLibrary& shaderLibrary);
    void bindToShader(const Shader& shader, int irradianceSlot = 12, int prefilterSlot = 13, int brdfLutSlot = 14) const;

    bool isInitialized() const { return initialized_; }
    unsigned int getEnvironmentMap() const { return environmentMap_; }
    unsigned int getIrradianceMap() const { return irradianceMap_; }
    unsigned int getPrefilterMap() const { return prefilterMap_; }
    unsigned int getBrdfLUT() const { return brdfLUTTexture_; }

private:
    unsigned int loadHDRTexture(const std::string& hdrPath);
    void createCaptureResources();
    void createCubeGeometry();
    void createQuadGeometry();
    void renderCube() const;
    void renderQuad() const;
    void destroy();

    bool initialized_ = false;
    unsigned int hdrTexture_ = 0;
    unsigned int environmentMap_ = 0;
    unsigned int irradianceMap_ = 0;
    unsigned int prefilterMap_ = 0;
    unsigned int brdfLUTTexture_ = 0;
    unsigned int captureFBO_ = 0;
    unsigned int captureRBO_ = 0;
    unsigned int cubeVAO_ = 0;
    unsigned int cubeVBO_ = 0;
    unsigned int quadVAO_ = 0;
    unsigned int quadVBO_ = 0;

    std::shared_ptr<Shader> equirectangularToCubemapShader_;
    std::shared_ptr<Shader> irradianceShader_;
    std::shared_ptr<Shader> prefilterShader_;
    std::shared_ptr<Shader> brdfShader_;
};

NAMESPACE_END
