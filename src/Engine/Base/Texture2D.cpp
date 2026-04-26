#include"Texture2D.h"
#include <stb_image.h>

NAMESPACE_START

unsigned char transparentData[64] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

GLint getFormat(const TextureFormat& textureFormat) {

    switch (textureFormat)
    {
    case TextureFormat::RGBA:
    case TextureFormat::RGBA32F:
        return GL_RGBA;
    case TextureFormat::RGB:
        return GL_RGB;
    case TextureFormat::RG:
        return GL_RG;
    case TextureFormat::R:
        return GL_R;
    case TextureFormat::Depth:
        return GL_DEPTH_COMPONENT;
    case TextureFormat::Depth24_Stencil8:
        return GL_DEPTH_STENCIL;
    default:
        return GL_RGBA;
    }
}

GLint getInternalformat(const TextureFormat& textureFormat) {

    switch (textureFormat)
    {
    case TextureFormat::RGBA:
        return GL_RGBA;
    case TextureFormat::RGBA32F:
        return GL_RGBA32F;
    case TextureFormat::RGB:
        return GL_RGB;
    case TextureFormat::RG:
        return GL_RG;
    case TextureFormat::R:
        return GL_R;
    case TextureFormat::Depth:
        return GL_DEPTH_COMPONENT;
    case TextureFormat::Depth24_Stencil8:
        return GL_DEPTH24_STENCIL8;
    default:
        return GL_RGBA;
    }
}

GLint getDataType(const TextureFormat& textureFormat) {

    switch (textureFormat)
    {
    case TextureFormat::RGBA:
        return GL_UNSIGNED_BYTE;
    case TextureFormat::RGBA32F:
        return GL_FLOAT;
    case TextureFormat::Depth:
        return GL_UNSIGNED_INT;
    case TextureFormat::Depth24_Stencil8:
        return GL_UNSIGNED_INT_24_8;
    default:
        return GL_UNSIGNED_BYTE;
    }
}

GLint getTexelFilter(const TexelFilter& texelFilter, const MipmapMode& mipmapMode) {
    if (mipmapMode == MipmapMode::None) {
        // No Mipmap
        return texelFilter == TexelFilter::Linear ? GL_LINEAR : GL_NEAREST;
    }
    else if (mipmapMode == MipmapMode::Nearest) {
        // use Nearest Mipmap
        return texelFilter == TexelFilter::Linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
    }
    else if (mipmapMode == MipmapMode::Linear) {
        // use Linear Mipmap
        return texelFilter == TexelFilter::Linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
    }
    else   return GL_LINEAR;
}

GLint getAddressMode(const SamplerAddressMode& addressMode) {
    if (addressMode == SamplerAddressMode::Repeat) {
        return GL_REPEAT;
    }
    else if (addressMode == SamplerAddressMode::ClampToEdge) {
        return GL_CLAMP_TO_EDGE;
    }
    else if (addressMode == SamplerAddressMode::ClampToBorder) {
        return GL_CLAMP_TO_BORDER;
    }
    else if (addressMode == SamplerAddressMode::MirroredRepeat) {
        return GL_MIRRORED_REPEAT;
    }
    else   return GL_REPEAT;
}


Texture2D::Texture2D(const char* path) {

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data) {
        width = 4;
        height = 4;
        this->initTexture(TextureUsage::ShaderRead, TextureFormat::RGBA, width, height, transparentData);
    }
    else {
        if (nrChannels == 4) {
            this->initTexture(TextureUsage::ShaderRead, TextureFormat::RGBA, width, height, data);
        }
        if (nrChannels == 3) {
            this->initTexture(TextureUsage::ShaderRead, TextureFormat::RGB, width, height, data);
        }
        delete[] data;
    }
    

};

Texture2D::Texture2D(const TextureUsage& usage, const TextureFormat& textureFormat, SamplerInfo samplerInfo, const int width, const int height, const unsigned char* data) {
    this->sampler = samplerInfo;
    this->initTexture(usage, textureFormat, width, height, data);
}

void Texture2D::initTexture(const TextureUsage& usage, const TextureFormat& textureFormat, const int width, const int height, const unsigned char* data) {

    this->textureUsage = usage;
    this->textureFormat = textureFormat;

    this->width = width;

    this->height = height;
    glGenTextures(1, &id);
    if (useCubeMap) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);

        for (unsigned int i = 0; i < 6; ++i)  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, getInternalformat(textureFormat), width, height, 0, getFormat(textureFormat), getDataType(textureFormat), data);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, getTexelFilter(sampler.minFilter, sampler.mipmapMode));
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, getTexelFilter(sampler.minFilter, sampler.mipmapMode));
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, id);
        //GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels
        glTexImage2D(GL_TEXTURE_2D, 0, getInternalformat(textureFormat), width, height, 0, getFormat(textureFormat), getDataType(textureFormat), data);

        //GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, getFormat(textureFormat), getDataType(textureFormat), data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getTexelFilter(sampler.minFilter, sampler.mipmapMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getTexelFilter(sampler.magFilter, sampler.mipmapMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, getAddressMode(sampler.addressMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, getAddressMode(sampler.addressMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, getAddressMode(sampler.addressMode));
        if (sampler.addressMode == SamplerAddressMode::ClampToBorder) {
            GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        }
        
        if (sampler.mipmapMode == MipmapMode::Linear)  glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

Texture2D::~Texture2D() {

    if (id >= 0) {
        glDeleteTextures(1, &id);
    }
    
}

NAMESPACE_END

