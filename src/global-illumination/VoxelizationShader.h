#ifndef Magnum_Examples_VoxelizationShader_h
#define Magnum_Examples_VoxelizationShader_h
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

#include <Magnum/AbstractShaderProgram.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Shaders/Generic.h>

#include "MaterialData.h"
#include "LightData.h"

namespace Magnum { namespace Examples {

class VoxelizationShader: public AbstractShaderProgram {
    public:
        /**
         * @brief Vertex position
         *
         * @ref shaders-generic "Generic attribute", @ref Vector3.
         */
        typedef Shaders::Generic3D::Position Position;

        /**
         * @brief Normal direction
         *
         * @ref shaders-generic "Generic attribute", @ref Vector3.
         */
        typedef Shaders::Generic3D::Normal Normal;

        /**
         * @brief 2D texture coordinates
         *
         * @ref shaders-generic "Generic attribute", @ref Vector2, used only if
         * at least one of @ref Flag::AmbientTexture, @ref Flag::DiffuseTexture
         * and @ref Flag::SpecularTexture is set.
         */
        typedef Shaders::Generic3D::TextureCoordinates TextureCoordinates;

        /**
         * @brief Flag
         *
         * @see @ref Flags, @ref flags()
         */
        enum class Flag: UnsignedByte {
            AmbientTexture = 1 << 0,    /**< The shader uses ambient texture instead of color */
            DiffuseTexture = 1 << 1,    /**< The shader uses diffuse texture instead of color */
            SpecularTexture = 1 << 2    /**< The shader uses specular texture instead of color */
        };

        /**
         * @brief Flags
         *
         * @see @ref flags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Constructor
         * @param flags     Flags
         */
        explicit VoxelizationShader(Flags flags = {});

        /**
         * @brief Construct without creating the underlying OpenGL object
         *
         * The constructed instance is equivalent to moved-from state. Useful
         * in cases where you will overwrite the instance later anyway. Move
         * another object over it to make it useful.
         *
         * This function can be safely used for constructing (and later
         * destructing) objects even without any OpenGL context being active.
         */
        explicit VoxelizationShader(NoCreateT) noexcept: AbstractShaderProgram{NoCreate} {}

        /** @brief Flags */
        Flags flags() const { return _flags; }

        /**
         * @brief Set transformation matrix
         */
        VoxelizationShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        /**
         * @brief Set projection X matrix
         */
        VoxelizationShader& setProjectionMatrixX(const Matrix4& matrix) {
            setUniform(_projectionMatrixXUniform, matrix);
            return *this;
        }

        /**
         * @brief Set projection Y matrix
         */
        VoxelizationShader& setProjectionMatrixY(const Matrix4& matrix) {
            setUniform(_projectionMatrixYUniform, matrix);
            return *this;
        }

        /**
         * @brief Set projection Z matrix
         */
        VoxelizationShader& setProjectionMatrixZ(const Matrix4& matrix) {
            setUniform(_projectionMatrixZUniform, matrix);
            return *this;
        }

        /**
         * @brief Set diffuse color
         */
        VoxelizationShader& setDiffuseColor(const Color4& color) {
            setUniform(_diffuseColorUniform, color);
            return *this;
        }

        /**
         * @brief Set voxel dimensions
         */
        VoxelizationShader& setVoxelDimensions(Int voxelDimensions) {
            setUniform(_voxelDimsUniform, voxelDimensions);
            return *this;
        }

        /**
         * @brief Set voxel size
         */
        VoxelizationShader& setVoxelSize(Float voxelSize) {
            setUniform(_voxelSizeUniform, voxelSize);
            return *this;
        }

        /**
         * @brief Set voxel world size
         */
        VoxelizationShader& setVoxelWorldSize(Float voxelWorldSize) {
            setUniform(_voxelWorldSizeUniform, voxelWorldSize);
            return *this;
        }

        /**
         * @brief Set voxel texture
         */
        VoxelizationShader& setVoxelTexture(Texture3D& texture);

        /**
         * @brief Set diffuse texture
         */
        VoxelizationShader& setDiffuseTexture(Texture2D& texture);

    private:
        Flags _flags;
        Int _transformationMatrixUniform{0},
            _projectionMatrixXUniform{1},
            _projectionMatrixYUniform{2},
            _projectionMatrixZUniform{3},
            _diffuseColorUniform{4},
            _voxelDimsUniform{5},
            _voxelSizeUniform{6},
            _voxelWorldSizeUniform{7};
        Int _voxelTextureBinding{0},
            _diffuseTextureBinding{1};
};

CORRADE_ENUMSET_OPERATORS(VoxelizationShader::Flags)

}}

#endif