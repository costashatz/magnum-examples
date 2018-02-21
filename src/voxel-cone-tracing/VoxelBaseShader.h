#ifndef Magnum_Examples_VoxelBaseShader_h
#define Magnum_Examples_VoxelBaseShader_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>
        2016 — Bill Robinson <airbaggins@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Magnum/AbstractShaderProgram.h>
#include <Magnum/Texture.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Shaders/Generic.h>

namespace Magnum { namespace Examples {

struct Material {
    Vector3 diffuseColor;
    Vector3 specularColor;
    Float specularDiffusion; // "Reflective and refractive" specular diffusion.
    Float diffuseReflectivity;
    Float specularReflectivity;
    Float emissivity;
    Float refractiveIndex;
    Float transparency;
};

struct PointLight {
    Vector3 position;
    Vector3 transformedPosition;
    Vector3 color;
};

class VoxelBaseShader: public AbstractShaderProgram {
    public:
        typedef Shaders::Generic3D::Position Position;
        typedef Shaders::Generic3D::Normal Normal;

        explicit VoxelBaseShader();

        virtual VoxelBaseShader& getUniformLocations();
        virtual VoxelBaseShader& bindLocations();

        VoxelBaseShader& setModelMatrix(const Matrix4& matrix);

        virtual VoxelBaseShader& setMaterial(const Material& material);

        VoxelBaseShader& setLight(Int i, const PointLight& light);
        VoxelBaseShader& setNumberOfLights(Int numLights);

        VoxelBaseShader& setVoxelTexture(Texture3D& texture);
        VoxelBaseShader& bindVoxelTexture(Texture3D& texture);

    protected:
        Int _maxLights;
    private:
        Int _modelMatrixUniform;

        Int _diffuseColorUniform;
        Int _specularColorUniform;
        Int _diffuseReflectivityUniform;
        Int _specularReflectivityUniform;
        Int _emissivityUniform;
        Int _transparencyUniform;

        Int _lightsUniform;
        Int _lightsNumberUniform;

        Int _texture3DUniform;
};

}}

#endif
