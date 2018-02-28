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

#include "MaterialData.h"

namespace Magnum { namespace Examples {

MaterialData::MaterialData(MaterialData&& other) noexcept: _flags{other._flags}, _shininess{other._shininess}, _emissive(other._emissive) {
    if(_flags & Flag::AmbientTexture)
        _ambient.texture = other._ambient.texture;
    else
        _ambient.color = other._ambient.color;

    if(_flags & Flag::DiffuseTexture)
        _diffuse.texture = other._diffuse.texture;
    else
        _diffuse.color = other._diffuse.color;

    if(_flags & Flag::SpecularTexture)
        _specular.texture = other._specular.texture;
    else
        _specular.color = other._specular.color;
}

MaterialData& MaterialData::operator=(MaterialData&& other) noexcept {
    _flags = other._flags;
    _shininess = other._shininess;
    _emissive = other._emissive;

    if(_flags & Flag::AmbientTexture)
        _ambient.texture = other._ambient.texture;
    else
        _ambient.color = other._ambient.color;

    if(_flags & Flag::DiffuseTexture)
        _diffuse.texture = other._diffuse.texture;
    else
        _diffuse.color = other._diffuse.color;

    if(_flags & Flag::SpecularTexture)
        _specular.texture = other._specular.texture;
    else
        _specular.color = other._specular.color;

    return *this;
}

Color4& MaterialData::ambientColor() {
    CORRADE_ASSERT(!(_flags & Flag::AmbientTexture), "MaterialData::ambientColor(): the material has ambient texture", _ambient.color);
    return _ambient.color;
}

UnsignedInt& MaterialData::ambientTexture() {
    CORRADE_ASSERT(_flags & Flag::AmbientTexture, "MaterialData::ambientTexture(): the material doesn't have ambient texture", _ambient.texture);
    return _ambient.texture;
}

Color4& MaterialData::diffuseColor() {
    CORRADE_ASSERT(!(_flags & Flag::DiffuseTexture), "MaterialData::diffuseColor(): the material has diffuse texture", _diffuse.color);
    return _diffuse.color;
}

UnsignedInt& MaterialData::diffuseTexture() {
    CORRADE_ASSERT(_flags & Flag::DiffuseTexture, "MaterialData::diffuseTexture(): the material doesn't have diffuse texture", _diffuse.texture);
    return _diffuse.texture;
}

Color4& MaterialData::specularColor() {
    CORRADE_ASSERT(!(_flags & Flag::SpecularTexture), "MaterialData::specularColor(): the material has specular texture", _specular.color);
    return _specular.color;
}

UnsignedInt& MaterialData::specularTexture() {
    CORRADE_ASSERT(_flags & Flag::SpecularTexture, "MaterialData::specularTexture(): the material doesn't have specular texture", _specular.texture);
    return _specular.texture;
}

Color4& MaterialData::emissiveColor() {
    return _emissive;
}

}}