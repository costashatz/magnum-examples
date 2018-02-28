#ifndef Magnum_Examples_VoxelVisualizationShader_h
#define Magnum_Examples_VoxelVisualizationShader_h
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

namespace Magnum { namespace Examples {

class VoxelVisualizationShader: public AbstractShaderProgram {
    public:
        /**
         * @brief Constructor
         */
        explicit VoxelVisualizationShader();

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
        explicit VoxelVisualizationShader(NoCreateT) noexcept: AbstractShaderProgram{NoCreate} {}

        /**
         * @brief Set voxel world size
         */
        VoxelVisualizationShader& setVoxelSize(Float voxelSize) {
            setUniform(_voxelSizeUniform, voxelSize);
            return *this;
        }

        /**
         * @brief Set voxel dimensions
         */
        VoxelVisualizationShader& setVoxelDimensions(Int voxelDimensions) {
            setUniform(_voxelDimsUniform, voxelDimensions);
            return *this;
        }

        /**
         * @brief Set transformation matrix
         */
        VoxelVisualizationShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        /**
         * @brief Set voxel texture
         */
        VoxelVisualizationShader& setVoxelTexture(Texture3D& texture);

    private:
        Int _voxelSizeUniform{0},
            _voxelDimsUniform{1},
            _transformationMatrixUniform{2};
        Int _voxelTextureBinding{0};
};

}}

#endif