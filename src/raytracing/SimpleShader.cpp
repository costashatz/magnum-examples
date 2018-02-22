/*
    This file is part of Magnum.
    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>
    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "SimpleShader.h"

#include <Corrade/Utility/Resource.h>

#include "Magnum/Context.h"
#include "Magnum/Extensions.h"
#include "Magnum/Shader.h"
#include "Magnum/Texture.h"

namespace Magnum { namespace Examples {

namespace {
    enum: Int {
        AmbientTextureLayer = 0,
        DiffuseTextureLayer = 1,
        SpecularTextureLayer = 2
    };
}

SimpleShader::SimpleShader(const Flags flags): _flags(flags) {
    Utility::Resource rs("RaytracingShaders");

    Shader vert{Version::GL430, Shader::Type::Vertex};
    Shader frag{Version::GL430, Shader::Type::Fragment};

    vert.addSource(flags ? "#define TEXTURED\n" : "")
        .addSource(rs.get("SimpleShader.vert"));
    frag.addSource(flags & Flag::AmbientTexture ? "#define AMBIENT_TEXTURE\n" : "")
        .addSource(flags & Flag::DiffuseTexture ? "#define DIFFUSE_TEXTURE\n" : "")
        .addSource(flags & Flag::SpecularTexture ? "#define SPECULAR_TEXTURE\n" : "")
        .addSource(rs.get("SimpleShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

    attachShaders({vert, frag});

    bindAttributeLocation(Position::Location, "position");
    bindAttributeLocation(Normal::Location, "normal");
    if(flags) bindAttributeLocation(TextureCoordinates::Location, "textureCoordinates");

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    _transformationMatrixUniform = uniformLocation("transformationMatrix");
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _cameraMatrixUniform = uniformLocation("cameraMatrix");
    _normalMatrixUniform = uniformLocation("normalMatrix");
    _lightUniform = uniformLocation("lightPosition");
    _ambientColorUniform = uniformLocation("ambientColor");
    _diffuseColorUniform = uniformLocation("diffuseColor");
    _specularColorUniform = uniformLocation("specularColor");
    _lightColorUniform = uniformLocation("lightColor");
    _shininessUniform = uniformLocation("shininess");

    if(flags & Flag::AmbientTexture) setUniform(uniformLocation("ambientTexture"), AmbientTextureLayer);
    if(flags & Flag::DiffuseTexture) setUniform(uniformLocation("diffuseTexture"), DiffuseTextureLayer);
    if(flags & Flag::SpecularTexture) setUniform(uniformLocation("specularTexture"), SpecularTextureLayer);

    /* Default to fully opaque white so we can see the textures */
    if(flags & Flag::AmbientTexture) setAmbientColor(Color4{1.0f});
    else setAmbientColor(Color4{0.0f, 1.0f});

    if(flags & Flag::DiffuseTexture) setDiffuseColor(Color4{1.0f});

    setSpecularColor(Color4{1.0f});
    setLightColor(Color4{1.0f});
    setShininess(80.0f);
}

SimpleShader& SimpleShader::setAmbientTexture(Texture2D& texture) {
    if(_flags & Flag::AmbientTexture) texture.bind(AmbientTextureLayer);
    return *this;
}

SimpleShader& SimpleShader::setDiffuseTexture(Texture2D& texture) {
    if(_flags & Flag::DiffuseTexture) texture.bind(DiffuseTextureLayer);
    return *this;
}

SimpleShader& SimpleShader::setSpecularTexture(Texture2D& texture) {
    if(_flags & Flag::SpecularTexture) texture.bind(SpecularTextureLayer);
    return *this;
}

SimpleShader& SimpleShader::setTextures(Texture2D* ambient, Texture2D* diffuse, Texture2D* specular) {
    AbstractTexture::bind(AmbientTextureLayer, {ambient, diffuse, specular});
    return *this;
}

}}