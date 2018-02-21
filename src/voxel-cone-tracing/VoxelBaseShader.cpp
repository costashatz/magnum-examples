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

#include "VoxelBaseShader.h"

#include <Corrade/Utility/Resource.h>
#include <Magnum/Context.h>
#include <Magnum/Shader.h>
#include <Magnum/Version.h>
#include <Magnum/ImageFormat.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum { namespace Examples {

VoxelBaseShader::VoxelBaseShader() {
    /* maximum number of lights */
    _maxLights = 10;
}

VoxelBaseShader& VoxelBaseShader::getUniformLocations() {
    /* get uniform locations */
    _modelMatrixUniform = uniformLocation("M");

    _diffuseColorUniform = uniformLocation("material.diffuseColor");
    _specularColorUniform = uniformLocation("material.specularColor");
    _diffuseReflectivityUniform = uniformLocation("material.diffuseReflectivity");
    _specularReflectivityUniform = uniformLocation("material.specularReflectivity");
    _emissivityUniform = uniformLocation("material.emissivity");
    _transparencyUniform = uniformLocation("material.transparency");

    _lightsUniform = uniformLocation("pointLights[0].position");
    _lightsNumberUniform = uniformLocation("numberOfLights");

    _texture3DUniform = uniformLocation("texture3D");

    return *this;
}

VoxelBaseShader& VoxelBaseShader::bindLocations() {
    bindAttributeLocation(Position::Location, "position");
    bindAttributeLocation(Normal::Location, "normal");

    return *this;
}

VoxelBaseShader& VoxelBaseShader::setModelMatrix(const Matrix4& matrix) {
    setUniform(_modelMatrixUniform, matrix);
    return *this;
}

VoxelBaseShader& VoxelBaseShader::setMaterial(const Material& material) {
    setUniform(_diffuseColorUniform, material.diffuseColor);
    setUniform(_specularColorUniform, material.specularColor);
    setUniform(_diffuseReflectivityUniform, material.diffuseReflectivity);
    setUniform(_specularReflectivityUniform, material.specularReflectivity);
    setUniform(_emissivityUniform, material.emissivity);
    setUniform(_transparencyUniform, material.transparency);
    return *this;
}

VoxelBaseShader& VoxelBaseShader::setLight(Int i, const PointLight& light) {
    CORRADE_INTERNAL_ASSERT(i >= 0 && i < _maxLights);
    // Int locSize = 2;
    // setUniform(_lightsUniform + i * locSize, light.position);
    // setUniform(_lightsUniform + i * locSize + 1, light.color);
    setUniform(uniformLocation("pointLights["+std::to_string(i)+"].position"), light.position);
    setUniform(uniformLocation("pointLights["+std::to_string(i)+"].color"), light.color);
    return *this;
}

VoxelBaseShader& VoxelBaseShader::setNumberOfLights(Int numLights) {
    setUniform(_lightsNumberUniform, numLights);
    return *this;
}

VoxelBaseShader& VoxelBaseShader::setVoxelTexture(Texture3D& texture) {
    Int texUnit = 0;
    /* unit, level, access, format */
    texture.bindImageLayered(texUnit, 0, ImageAccess::WriteOnly, ImageFormat::RGBA8);
    /* unit */
    setUniform(_texture3DUniform, texUnit);
    return *this;
}

VoxelBaseShader& VoxelBaseShader::bindVoxelTexture(Texture3D& texture) {
    Int texUnit = 0;
    /* unit, level, access, format */
    texture.bind(texUnit);
    return *this;
}

}}
