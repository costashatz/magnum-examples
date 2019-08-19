#ifndef VCTEXAMPLE_VOXEL_VISUALIZATION_SHADER_H
#define VCTEXAMPLE_VOXEL_VISUALIZATION_SHADER_H

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/ImageFormat.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>

namespace Magnum { namespace Examples {

class VoxelVisualizationShader: public GL::AbstractShaderProgram {
    public:
        explicit VoxelVisualizationShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit VoxelVisualizationShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
            GL::Shader geom{GL::Version::GL430, GL::Shader::Type::Geometry};
            GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

            vert.addSource(rs.get("VoxelVisualizationShader.vert"));
            geom.addSource(rs.get("VoxelVisualizationShader.geom"));
            frag.addSource(rs.get("VoxelVisualizationShader.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, geom, frag}));

            attachShaders({vert, geom, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _transformationMatrixUniform = uniformLocation("transformationMatrix");
            _volumeDimensionUniform = uniformLocation("volumeDimension");
        }

        VoxelVisualizationShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        VoxelVisualizationShader& setVolumeDimension(UnsignedInt volumeDimension) {
            setUniform(_volumeDimensionUniform, volumeDimension);
            return *this;
        }

        VoxelVisualizationShader& bindVoxelTexture(GL::Texture3D& voxel, Int level = 0) {
            voxel.bindImage(_voxelPos, level, 0, GL::ImageAccess::ReadOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

    private:
        Int _transformationMatrixUniform,
            _volumeDimensionUniform;
        Int _voxelPos = 0;
};
}}

#endif