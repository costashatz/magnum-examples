#ifndef VCTEXAMPLE_VOXELIZATION_SHADER_H
#define VCTEXAMPLE_VOXELIZATION_SHADER_H

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

class VoxelizationShader: public GL::AbstractShaderProgram {
    public:
        explicit VoxelizationShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit VoxelizationShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
            GL::Shader geom{GL::Version::GL430, GL::Shader::Type::Geometry};
            GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

            bool isNvidia = (GL::Context::hasCurrent() && (GL::Context::current().detectedDriver() == GL::Context::DetectedDriver::NVidia));

            vert.addSource(rs.get("VoxelizationShader.vert"));
            geom.addSource(rs.get("VoxelizationShader.geom"));
            frag.addSource(isNvidia ? "#define NVIDIA\n" : "");
            frag.addSource("#extension GL_ARB_shader_image_load_store : require\n");
            frag.addSource(rs.get("common.glsl"));
            frag.addSource(rs.get("VoxelizationShader.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, geom, frag}));

            attachShaders({vert, geom, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _transformationMatrixUniform = uniformLocation("transformationMatrix");
            _normalMatrixUniform = uniformLocation("normalMatrix");
            _viewProjectionUniform = uniformLocation("viewProjections[0]");
            _viewProjectionIUniform = uniformLocation("viewProjectionsI[0]");
            _volumeDimensionUniform = uniformLocation("volumeDimension");
            _voxelScaleUniform = uniformLocation("voxelScale");
            _worldMinPointUniform = uniformLocation("worldMinPoint");
            _diffuseColorUniform = uniformLocation("diffuseColor");
            _emissiveColorUniform = uniformLocation("emissiveColor");
        }

        VoxelizationShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        VoxelizationShader& setNormalMatrix(const Matrix3& matrix) {
            setUniform(_normalMatrixUniform, matrix);
            return *this;
        }

        VoxelizationShader& setViewProjections(const std::vector<Matrix4>& matrices) {
            // assert(matrices.size() == 3);
            setUniform(_viewProjectionUniform, matrices[0]);
            setUniform(_viewProjectionUniform + 1, matrices[1]);
            setUniform(_viewProjectionUniform + 2, matrices[2]);
            return *this;
        }

        VoxelizationShader& setViewProjectionsI(const std::vector<Matrix4>& matrices) {
            // assert(matrices.size() == 3);
            setUniform(_viewProjectionIUniform, matrices[0]);
            setUniform(_viewProjectionIUniform + 1, matrices[1]);
            setUniform(_viewProjectionIUniform + 2, matrices[2]);
            return *this;
        }

        VoxelizationShader& setVolumeDimension(UnsignedInt volumeDimension) {
            setUniform(_volumeDimensionUniform, volumeDimension);
            return *this;
        }

        VoxelizationShader& setVoxelScale(Float voxelScale) {
            setUniform(_voxelScaleUniform, voxelScale);
            return *this;
        }

        VoxelizationShader& setMinPoint(const Vector3& minPoint) {
            setUniform(_worldMinPointUniform, minPoint);
            return *this;
        }

        VoxelizationShader& setDiffuseColor(const Color4& color) {
            setUniform(_diffuseColorUniform, color);
            return *this;
        }

        VoxelizationShader& setEmissiveColor(const Color3& color) {
            setUniform(_emissiveColorUniform, color);
            return *this;
        }

        VoxelizationShader& bindAlbedoTexture(GL::Texture3D& albedo) {
            // albedo.bindImage(_albedoPos, 0, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::R32UI);
            albedo.bindImageLayered(_albedoPos, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::R32UI);
            return *this;
        }

        VoxelizationShader& bindNormalTexture(GL::Texture3D& normal) {
            // normal.bindImage(_normalPos, 0, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::R32UI);
            normal.bindImageLayered(_normalPos, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::R32UI);
            return *this;
        }

        VoxelizationShader& bindEmissionTexture(GL::Texture3D& emission) {
            // emission.bindImage(_emissionPos, 0, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::R32UI);
            emission.bindImageLayered(_emissionPos, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::R32UI);
            return *this;
        }

    private:
        Int _transformationMatrixUniform,
            _normalMatrixUniform,
            _viewProjectionUniform,
            _viewProjectionIUniform,
            _volumeDimensionUniform,
            _voxelScaleUniform,
            _worldMinPointUniform,
            _diffuseColorUniform,
            _emissiveColorUniform;
        Int _albedoPos = 0, _normalPos = 1, _emissionPos = 2;
};
}}

#endif