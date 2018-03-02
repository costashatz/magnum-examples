#ifndef Magnum_Examples_VoxelConeTracingShader_h
#define Magnum_Examples_VoxelConeTracingShader_h
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

class VoxelConeTracingShader: public AbstractShaderProgram {
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
        explicit VoxelConeTracingShader(Flags flags = {});

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
        explicit VoxelConeTracingShader(NoCreateT) noexcept: AbstractShaderProgram{NoCreate} {}

        /** @brief Flags */
        Flags flags() const { return _flags; }

        /**
         * @brief Set transformation matrix (projection * view * model)
         */
        VoxelConeTracingShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        /**
         * @brief Set model matrix
         */
        VoxelConeTracingShader& setModelMatrix(const Matrix4& matrix) {
            setUniform(_modelMatrixUniform, matrix);
            return *this;
        }

        /**
         * @brief Set camera position
         */
        VoxelConeTracingShader& setCameraPosition(const Vector3& position) {
            setUniform(_cameraPositionUniform, position);
            return *this;
        }

        /**
         * @brief Set voxel world properties
         */
        VoxelConeTracingShader& setVoxelWorldProperties(Float voxelGridWorldSize, Int voxelDimensions) {
            setUniform(_voxelGridWorldSizeUniform, voxelGridWorldSize);
            setUniform(_voxelDimensionsUniform, voxelDimensions);
            return *this;
        }

        /**
         * @brief Set voxel dimensions
         */
        VoxelConeTracingShader& setVoxelDimensions(Int voxelDimensions) {
            setUniform(_voxelDimensionsUniform, voxelDimensions);
            return *this;
        }

        /**
         * @brief Set voxel world size
         */
        VoxelConeTracingShader& setVoxelWorldSize(Float voxelGridWorldSize) {
            setUniform(_voxelGridWorldSizeUniform, voxelGridWorldSize);
            return *this;
        }

        /**
         * @brief Set ambient color
         */
        VoxelConeTracingShader& setAmbientColor(const Color4& color) {
            setUniform(_ambientColorUniform, color);
            return *this;
        }

        /**
         * @brief Set diffuse color
         */
        VoxelConeTracingShader& setDiffuseColor(const Color4& color) {
            setUniform(_diffuseColorUniform, color);
            return *this;
        }

        /**
         * @brief Set specular color
         */
        VoxelConeTracingShader& setSpecularColor(const Color4& color) {
            setUniform(_specularColorUniform, color);
            return *this;
        }

        /**
         * @brief Set emissive color
         */
        VoxelConeTracingShader& setEmissiveColor(const Color4& color) {
            setUniform(_emissiveColorUniform, color);
            return *this;
        }

        /**
         * @brief Set shininess
         */
        VoxelConeTracingShader& setShininess(Float shininess) {
            setUniform(_shininessUniform, shininess);
            return *this;
        }

        /**
         * @brief Set material
         */
        VoxelConeTracingShader& setMaterial(MaterialData& material) {
            if(material.flags() & MaterialData::Flag::AmbientTexture)
                setAmbientTexture(material.ambientTexture());
            else
                setAmbientColor(material.ambientColor());
            if(material.flags() & MaterialData::Flag::DiffuseTexture)
                setDiffuseTexture(material.diffuseTexture());
            else
                setDiffuseColor(material.diffuseColor());
            if(material.flags() & MaterialData::Flag::SpecularTexture)
                setSpecularTexture(material.specularTexture());
            else
                setSpecularColor(material.specularColor());

            setEmissiveColor(material.emissiveColor());
            setShininess(material.shininess());
            return *this;
        }

        /**
         * @brief Set light
         */
        VoxelConeTracingShader& setLight(const LightData& light) {
            LightData::Type type = light.type();
            setUniform(_lightUniform, (type == LightData::Type::Infinite) ? 0 : (type == LightData::Type::Point) ? 1 : 2);
            setUniform(_lightUniform + 1, light.color());
            if(type != LightData::Type::Infinite) {
                setUniform(_lightUniform + 2, light.position());
            }
            if(type != LightData::Type::Point) {
                setUniform(_lightUniform + 3, light.direction());
            }
            if(type == LightData::Type::Spot) {
                setUniform(_lightUniform + 4, light.spotExponent());
                setUniform(_lightUniform + 5, light.spotCutoff());
            }
            setUniform(_lightUniform + 6, light.intensity());
            setUniform(_lightUniform + 7, light.constantAttenuation());
            setUniform(_lightUniform + 8, light.linearAttenuation());
            setUniform(_lightUniform + 9, light.quadraticAttenuation());
            return *this;
        }

        /**
         * @brief Set voxel texture
         */
        VoxelConeTracingShader& setVoxelTexture(Texture3D& texture);

        /**
         * @brief Set ambient texture
         */
        VoxelConeTracingShader& setAmbientTexture(Texture2D& texture);

        /**
         * @brief Set diffuse texture
         */
        VoxelConeTracingShader& setDiffuseTexture(Texture2D& texture);

        /**
         * @brief Set specular texture
         */
        VoxelConeTracingShader& setSpecularTexture(Texture2D& texture);

    private:
        Flags _flags;
        Int _transformationMatrixUniform{0},
            _modelMatrixUniform{1},
            _cameraPositionUniform{2},
            _voxelGridWorldSizeUniform{3},
            _voxelDimensionsUniform{4},
            _ambientColorUniform{5},
            _diffuseColorUniform{6},
            _specularColorUniform{7},
            _emissiveColorUniform{8},
            _shininessUniform{9},
            _lightUniform{10};
        Int _voxelTextureBinding{0},
            _ambientTextureBinding{1},
            _diffuseTextureBinding{2},
            _specularTextureBinding{3};
};

CORRADE_ENUMSET_OPERATORS(VoxelConeTracingShader::Flags)

}}

#endif