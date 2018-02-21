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

#include "VoxelConeTracingShader.h"

#include <Corrade/Utility/Resource.h>
#include <Magnum/Context.h>
#include <Magnum/Shader.h>
#include <Magnum/Version.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum { namespace Examples {

VoxelConeTracingShader::VoxelConeTracingShader() : VoxelBaseShader() {
    MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL450);

    const Utility::Resource rs{"shaders"};

    Shader vert{Version::GL450, Shader::Type::Vertex};
    Shader frag{Version::GL450, Shader::Type::Fragment};

    vert.addSource(rs.get("VoxelConeTracing.vert"));
    frag.addSource("#define MAX_LIGHTS " + std::to_string(_maxLights) + "\n")
        .addSource(rs.get("VoxelConeTracing.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

    bindLocations();

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    /* get uniform locations */
    getUniformLocations();
}

VoxelConeTracingShader& VoxelConeTracingShader::getUniformLocations() {
    VoxelBaseShader::getUniformLocations();
    /* get extra uniform locations */
    _viewMatrixUniform = uniformLocation("V");
    _projectionMatrixUniform = uniformLocation("P");

    _cameraPosUniform = uniformLocation("cameraPosition");

    _specularDiffusionUniform = uniformLocation("material.specularDiffusion");
    _refractiveIndexUniform = uniformLocation("material.refractiveIndex");

    _indirectSpecularLightUniform = uniformLocation("settings.indirectSpecularLight");
    _indirectDiffuseLightUniform = uniformLocation("settings.indirectDiffuseLight");
    _directLightUniform = uniformLocation("settings.directLight");
    _shadowsUniform = uniformLocation("settings.shadows");

    return *this;
}

VoxelConeTracingShader& VoxelConeTracingShader::setViewMatrix(const Matrix4& matrix) {
    setUniform(_viewMatrixUniform, matrix);
    return *this;
}

VoxelConeTracingShader& VoxelConeTracingShader::setProjectionMatrix(const Matrix4& matrix) {
    setUniform(_projectionMatrixUniform, matrix);
    return *this;
}

VoxelConeTracingShader& VoxelConeTracingShader::setCameraPosition(const Vector3& position) {
    setUniform(_cameraPosUniform, position);
    return *this;
}

VoxelConeTracingShader& VoxelConeTracingShader::setMaterial(const Material& material) {
    VoxelBaseShader::setMaterial(material);
    setUniform(_specularDiffusionUniform, material.specularDiffusion);
    setUniform(_refractiveIndexUniform, material.refractiveIndex);
    return *this;
}

VoxelConeTracingShader& VoxelConeTracingShader::setMaterialSettings(const MaterialSettings& settings) {
    setUniform(_indirectSpecularLightUniform, settings.indirectSpecularLight);
    setUniform(_indirectDiffuseLightUniform, settings.indirectDiffuseLight);
    setUniform(_directLightUniform, settings.directLight);
    setUniform(_shadowsUniform, settings.shadows);
    return *this;
}

}}
