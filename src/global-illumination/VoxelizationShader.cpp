/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>
        2018 Konstantinos Chatzilygeroudis <costashatz@gmail.com>

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

#include <Corrade/Utility/Resource.h>

#include <Magnum/Context.h>
#include <Magnum/ImageFormat.h>
#include <Magnum/Shader.h>
#include <Magnum/Texture.h>
#include <Magnum/Version.h>

#include "VoxelizationShader.h"

namespace Magnum { namespace Examples {

VoxelizationShader::VoxelizationShader(Flags flags) : _flags{flags} {
    MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL450);

    const Utility::Resource rs{"global-illumination-shaders"};

    Shader vert{Version::GL450, Shader::Type::Vertex};
    Shader geom{Version::GL450, Shader::Type::Geometry};
    Shader frag{Version::GL450, Shader::Type::Fragment};

    vert.addSource(flags ? "#define TEXTURED\n" : "")
        .addSource(rs.get("VoxelizationShader.vert"));
    geom.addSource(flags ? "#define TEXTURED\n" : "")
        .addSource(rs.get("VoxelizationShader.geom"));
    frag.addSource(flags & Flag::AmbientTexture ? "#define AMBIENT_TEXTURE\n" : "")
        .addSource(flags & Flag::DiffuseTexture ? "#define DIFFUSE_TEXTURE\n" : "")
        // .addSource(flags & Flag::SpecularTexture ? "#define SPECULAR_TEXTURE\n" : "")
        .addSource(rs.get("VoxelizationShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, geom, frag}));

    /* bind locations */
    bindAttributeLocation(Position::Location, "position");
    bindAttributeLocation(Normal::Location, "normal");
    bindAttributeLocation(TextureCoordinates::Location, "texCoords");

    attachShaders({vert, geom, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    /* no need to get uniform locations */
}

VoxelizationShader& VoxelizationShader::setAmbientTexture(Texture2D& texture) {
    if(_flags & Flag::AmbientTexture) texture.bind(_ambientTextureBinding);
    return *this;
}

VoxelizationShader& VoxelizationShader::setDiffuseTexture(Texture2D& texture) {
    if(_flags & Flag::DiffuseTexture) texture.bind(_diffuseTextureBinding);
    return *this;
}

VoxelizationShader& VoxelizationShader::setVoxelTexture(Texture3D& texture) {
    /* unit, level, access, format */
    texture.bindImageLayered(_voxelTextureBinding, 0, ImageAccess::WriteOnly, ImageFormat::RGBA8);
    return *this;
}

}}