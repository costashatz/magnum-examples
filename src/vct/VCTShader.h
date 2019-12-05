#ifndef VCTEXAMPLE_VCT_SHADER_H
#define VCTEXAMPLE_VCT_SHADER_H

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

class VCTShader: public GL::AbstractShaderProgram {
    public:
        explicit VCTShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit VCTShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

            vert.addSource(rs.get("RenderTextureShader.vert"));
            frag.addSource(rs.get("common.glsl"));
            frag.addSource(rs.get("VCTShader.frag"));

            // Utility::Debug{} << frag.sources()[2].c_str();

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _inverseProjectionViewUniform = uniformLocation("inverseProjectionView");
            _cameraPositionUniform = uniformLocation("cameraPosition");
            _voxelScaleUniform = uniformLocation("voxelScale");
            _worldMinPointUniform = uniformLocation("worldMinPoint");
            _worldMaxPointUniform = uniformLocation("worldMaxPoint");
            _volumeDimensionUniform = uniformLocation("volumeDimension");
            _aoAlphaUniform = uniformLocation("aoAlpha");
            _aoFalloffUniform = uniformLocation("aoFalloff");
            _maxTracingDistanceUniform = uniformLocation("maxTracingDistance");
            _samplingFactorUniform = uniformLocation("samplingFactor");
            _bounceStrengthUniform = uniformLocation("bounceStrength");
            _coneShadowToleranceUniform = uniformLocation("coneShadowTolerance");
            _coneShadowApertureUniform = uniformLocation("coneShadowAperture");
            _lightsUniform = uniformLocation("lights[0].position");
        }

        // TO-DO: Make this multi-light
        VCTShader& setLight() {
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

        VCTShader& setInverseProjectionViewMatrix(const Matrix4& matrix) {
            setUniform(_inverseProjectionViewUniform, matrix);
            return *this;
        }

        VCTShader& setCameraPosition(const Vector3& position) {
            setUniform(_cameraPositionUniform, position);
            return *this;
        }

        VCTShader& setVoxelScale(Float voxelScale) {
            setUniform(_voxelScaleUniform, voxelScale);
            return *this;
        }

        VCTShader& setWorldMinPoint(const Vector3& worldMinPoint) {
            setUniform(_worldMinPointUniform, worldMinPoint);
            return *this;
        }

        VCTShader& setWorldMaxPoint(const Vector3& worldMaxPoint) {
            setUniform(_worldMaxPointUniform, worldMaxPoint);
            return *this;
        }

        VCTShader& setVolumeDimension(UnsignedInt volumeDimension) {
            setUniform(_volumeDimensionUniform, volumeDimension);
            return *this;
        }

        VCTShader& setAOAlpha(Float aoAlpha) {
            setUniform(_aoAlphaUniform, aoAlpha);
            return *this;
        }

        VCTShader& setAOFalloff(Float aoFalloff) {
            setUniform(_aoFalloffUniform, aoFalloff);
            return *this;
        }

        VCTShader& setMaxTracingDistance(Float maxDist) {
            setUniform(_maxTracingDistanceUniform, maxDist);
            return *this;
        }

        VCTShader& setSamplingFactor(Float samplingFactor) {
            setUniform(_samplingFactorUniform, samplingFactor);
            return *this;
        }

        VCTShader& setBounceStrength(Float bounce) {
            setUniform(_bounceStrengthUniform, bounce);
            return *this;
        }

        VCTShader& setConeShadowTolerance(Float coneShadowTolerance) {
            setUniform(_coneShadowToleranceUniform, coneShadowTolerance);
            return *this;
        }

        VCTShader& setConeShadowAperture(Float coneShadowAperture) {
            setUniform(_coneShadowApertureUniform, coneShadowAperture);
            return *this;
        }

        VCTShader& bindAlbedoTexture(GL::Texture2D& tex) {
            tex.bind(_albedoPos);
            return *this;
        }

        VCTShader& bindNormalTexture(GL::Texture2D& tex) {
            tex.bind(_normalPos);
            return *this;
        }

        VCTShader& bindSpecularTexture(GL::Texture2D& tex) {
            tex.bind(_specularPos);
            return *this;
        }

        VCTShader& bindEmissionTexture(GL::Texture2D& tex) {
            tex.bind(_emissionPos);
            return *this;
        }

        VCTShader& bindDepthTexture(GL::Texture2D& tex) {
            tex.bind(_depthPos);
            return *this;
        }

         VCTShader& bindVoxelVisibility(GL::Texture3D& voxel) {
            voxel.bind(_voxelVisibility);
            return *this;
        }

        VCTShader& bindVoxelTexture(GL::Texture3D& voxel) {
            voxel.bind(_voxelPos);
            return *this;
        }

        VCTShader& bindMipMapTextures(GL::Texture3D* texs) {
            for(UnsignedInt i = 0; i < 6; i++)
                texs[i].bind(_mipMapPos + i);
            return *this;
        }

    private:
        Int _inverseProjectionViewUniform,
            _cameraPositionUniform,
            _voxelScaleUniform,
            _worldMinPointUniform,
            _worldMaxPointUniform,
            _volumeDimensionUniform,
            _aoAlphaUniform,
            _aoFalloffUniform,
            _maxTracingDistanceUniform,
            _samplingFactorUniform,
            _bounceStrengthUniform,
            _coneShadowToleranceUniform,
            _coneShadowApertureUniform,
            _lightsUniform;

        Int _albedoPos = 0, _normalPos = 1, _specularPos = 2, _emissionPos = 3, _depthPos = 4;
        Int _voxelVisibility = 5, _voxelPos = 6, _mipMapPos = 7;
};
}}

#endif