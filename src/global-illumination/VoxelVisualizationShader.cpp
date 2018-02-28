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

#include "VoxelVisualizationShader.h"

namespace Magnum { namespace Examples {

VoxelVisualizationShader::VoxelVisualizationShader() {
    MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL450);

    const Utility::Resource rs{"global-illumination-shaders"};

    Shader vert{Version::GL450, Shader::Type::Vertex};
    Shader geom{Version::GL450, Shader::Type::Geometry};
    Shader frag{Version::GL450, Shader::Type::Fragment};

    vert.addSource(rs.get("VoxelVisualizationShader.vert"));
    geom.addSource(rs.get("VoxelVisualizationShader.geom"));
    frag.addSource(rs.get("VoxelVisualizationShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, geom, frag}));

    attachShaders({vert, geom, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    /* no need to get uniform locations */
}

VoxelVisualizationShader& VoxelVisualizationShader::setVoxelTexture(Texture3D& texture) {
    texture.bind(_voxelTextureBinding);
    return *this;
}

}}