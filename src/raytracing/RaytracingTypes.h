#ifndef Magnum_Shaders_RaytracingTypes_h
#define Magnum_Shaders_RaytracingTypes_h
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

#include "Magnum/Magnum.h"
#include "Magnum/Math/Color.h"
#include "Magnum/Math/Vector3.h"

namespace Magnum { namespace Examples {

/* Camera model */
struct Camera {
	Vector3 pos;
    Vector3 dir;
    Vector3 yAxis;
    Vector3 xAxis;
	Float tanFovY;
    Float tanFovX;
};

/* Material model */
struct Material {
	Color4 diffuse;
	Color4 specular;
	Color4 emission;
	Float shininess;
    /* todo: add refraction once first draft is working */
    /* todo: add textures once first draft is working */
};

/* assume counter-clockwise direction for normal */
struct Triangle {
    /* vertices of triangle */
	Vector3 A;
    Vector3 B;
    Vector3 C;
    /* todo: maybe include normal vectors as well */
    /* todo: maybe include texture coordinates */
};

/* Triangle object */
struct Object {
	Triangle triangle;
	Int materialIndex;
};

/* Light model */
struct Light {
    Vector4 position;
    Color4 ambient;
    Color4 diffuse;
    Color4 specular;
    Vector3 spotDirection;
    Float spotExponent;
    Float spotCutoff;
    Float intensity;
    Float constantAttenuation;
    Float linearAttenuation;
    Float quadraticAttenuation;
};

}}

#endif