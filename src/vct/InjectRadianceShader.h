#ifndef VCTEXAMPLE_INJECT_RADIANCE_SHADER_H
#define VCTEXAMPLE_INJECT_RADIANCE_SHADER_H

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

class InjectRadianceShader: public GL::AbstractShaderProgram {
    public:
        explicit InjectRadianceShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit InjectRadianceShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader comp{GL::Version::GL430, GL::Shader::Type::Compute};

            comp.addSource("#extension GL_ARB_shader_image_load_store : require\n");
            comp.addSource(rs.get("common.glsl"));
            comp.addSource(rs.get("InjectRadianceShader.comp"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({comp}));

            attachShaders({comp});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _volumeDimensionUniform = uniformLocation("volumeDimension");
            _voxelSizeUniform = uniformLocation("voxelSize");
            _voxelScaleUniform = uniformLocation("voxelScale");
            _worldMinPointUniform = uniformLocation("worldMinPoint");
            _traceShadowHitUniform = uniformLocation("traceShadowHit");
            _lightsUniform = uniformLocation("lights[0].position");
        }

        // TO-DO: Make this multi-light
        InjectRadianceShader& setLight() {
            /* Create default light for testing */
            // setUniform(_lightsUniform, Vector4(-1.f, -0.5f, -1.f, 0.f));
            setUniform(_lightsUniform, Vector4(0.f, 0.75f, -0.5f, 1.f));
            setUniform(_lightsUniform + 1, Vector4(1.f, 1.f, 1.f, 1.f));
            setUniform(_lightsUniform + 2, Vector3(1.f, 0.f, 0.f));
            setUniform(_lightsUniform + 3, 1.f);
            setUniform(_lightsUniform + 4, Math::Constants<Magnum::Float>::piHalf());
            setUniform(_lightsUniform + 5, 1.f);
            setUniform(_lightsUniform + 6, 1.f);
            setUniform(_lightsUniform + 7, 0.f);
            setUniform(_lightsUniform + 8, 0.f);

            return *this;
        }

        InjectRadianceShader& setVolumeDimension(Int volumeDimension) {
            setUniform(_volumeDimensionUniform, volumeDimension);
            return *this;
        }

        InjectRadianceShader& setVoxelSize(Float voxelSize) {
            setUniform(_voxelSizeUniform, voxelSize);
            return *this;
        }

        InjectRadianceShader& setVoxelScale(Float voxelScale) {
            setUniform(_voxelScaleUniform, voxelScale);
            return *this;
        }

        InjectRadianceShader& setMinPoint(const Vector3& minPoint) {
            setUniform(_worldMinPointUniform, minPoint);
            return *this;
        }

        InjectRadianceShader& setTraceShadowHit(Float traceShadowHit) {
            setUniform(_traceShadowHitUniform, traceShadowHit);
            return *this;
        }

        InjectRadianceShader& bindAlbedoTexture(GL::Texture3D& albedo) {
            albedo.bind(_albedoPos);
            return *this;
        }

        InjectRadianceShader& bindNormalTexture(GL::Texture3D& normal) {
            // normal.bindImage(_normalPos, 0, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::RGBA8);
            normal.bindImageLayered(_normalPos, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::RGBA8);
            return *this;
        }

        InjectRadianceShader& bindEmissionTexture(GL::Texture3D& emission) {
            // emission.bindImage(_emissionPos, 0, 0, GL::ImageAccess::ReadOnly, GL::ImageFormat::RGBA8);
            emission.bindImageLayered(_emissionPos, 0, GL::ImageAccess::ReadOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

        InjectRadianceShader& bindRadianceTexture(GL::Texture3D& radiance) {
            // radiance.bindImage(_radiancePos, 0, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            radiance.bindImageLayered(_radiancePos, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

    private:
        Int _volumeDimensionUniform, _voxelSizeUniform, _voxelScaleUniform, _worldMinPointUniform, _traceShadowHitUniform, _lightsUniform;
        // TO-DO: Check why normal texture is storing (or visualizing) weird values
        Int _albedoPos = 0, _normalPos = 2, _emissionPos = 4, _radiancePos = 6;
};
}}

#endif