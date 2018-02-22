#ifndef Magnum_Shaders_SimpleShader_h
#define Magnum_Shaders_SimpleShader_h
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

#include "Magnum/AbstractShaderProgram.h"
#include "Magnum/Math/Color.h"
#include "Magnum/Math/Matrix4.h"
#include "Magnum/Shaders/Generic.h"
#include "Magnum/Shaders/visibility.h"

namespace Magnum { namespace Examples {

class SimpleShader: public AbstractShaderProgram {
    public:
        typedef Shaders::Generic3D::Position Position;
        typedef Shaders::Generic3D::Normal Normal;
        typedef Shaders::Generic3D::TextureCoordinates TextureCoordinates;

        enum class Flag: UnsignedByte {
            AmbientTexture = 1 << 0,    /**< The shader uses ambient texture instead of color */
            DiffuseTexture = 1 << 1,    /**< The shader uses diffuse texture instead of color */
            SpecularTexture = 1 << 2    /**< The shader uses specular texture instead of color */
        };

        typedef Containers::EnumSet<Flag> Flags;

        explicit SimpleShader(Flags flags = {});

        explicit SimpleShader(NoCreateT) noexcept: AbstractShaderProgram{NoCreate} {}

        Flags flags() const { return _flags; }

        SimpleShader& setAmbientColor(const Color4& color) {
            setUniform(_ambientColorUniform, color);
            return *this;
        }

        SimpleShader& setAmbientTexture(Texture2D& texture);

        SimpleShader& setDiffuseColor(const Color4& color) {
            setUniform(_diffuseColorUniform, color);
            return *this;
        }

        SimpleShader& setDiffuseTexture(Texture2D& texture);

        SimpleShader& setSpecularColor(const Color4& color) {
            setUniform(_specularColorUniform, color);
            return *this;
        }

        SimpleShader& setSpecularTexture(Texture2D& texture);

        SimpleShader& setTextures(Texture2D* ambient, Texture2D* diffuse, Texture2D* specular);

        SimpleShader& setShininess(Float shininess) {
            setUniform(_shininessUniform, shininess);
            return *this;
        }

        SimpleShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        SimpleShader& setNormalMatrix(const Matrix3x3& matrix) {
            setUniform(_normalMatrixUniform, matrix);
            return *this;
        }

        SimpleShader& setProjectionMatrix(const Matrix4& matrix) {
            setUniform(_projectionMatrixUniform, matrix);
            return *this;
        }

        SimpleShader& setCameraMatrix(const Matrix4& matrix) {
            setUniform(_cameraMatrixUniform, matrix);
            return *this;
        }

        SimpleShader& setLightPosition(const Vector3& light) {
            setUniform(_lightUniform, light);
            return *this;
        }

        SimpleShader& setLightColor(const Color4& color) {
            setUniform(_lightColorUniform, color);
            return *this;
        }

    private:
        Flags _flags;
        Int _transformationMatrixUniform,
            _projectionMatrixUniform,
            _cameraMatrixUniform,
            _normalMatrixUniform,
            _lightUniform,
            _ambientColorUniform,
            _diffuseColorUniform,
            _specularColorUniform,
            _lightColorUniform,
            _shininessUniform;
};

CORRADE_ENUMSET_OPERATORS(SimpleShader::Flags)

}}

#endif