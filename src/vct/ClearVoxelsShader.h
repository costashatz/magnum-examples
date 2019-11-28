#ifndef VCTEXAMPLE_CLEAR_VOXELS_SHADER_H
#define VCTEXAMPLE_CLEAR_VOXELS_SHADER_H

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

class ClearVoxelsShader: public GL::AbstractShaderProgram {
    public:
        explicit ClearVoxelsShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit ClearVoxelsShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader comp{GL::Version::GL430, GL::Shader::Type::Compute};

            comp.addSource(rs.get("ClearVoxelsShader.comp"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({comp}));

            attachShaders({comp});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());
        }

        ClearVoxelsShader& bindAlbedoTexture(GL::Texture3D& albedo) {
            // albedo.bindImage(_albedoPos, 0, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::RGBA8);
            albedo.bindImageLayered(_albedoPos, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::RGBA8);
            return *this;
        }

        ClearVoxelsShader& bindNormalTexture(GL::Texture3D& normal) {
            // normal.bindImage(_normalPos, 0, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            normal.bindImageLayered(_normalPos, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

        ClearVoxelsShader& bindEmissionTexture(GL::Texture3D& emission) {
            // emission.bindImage(_emissionPos, 0, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            emission.bindImageLayered(_emissionPos, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

        ClearVoxelsShader& bindRadianceTexture(GL::Texture3D& radiance) {
            // radiance.bindImage(_radiancePos, 0, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            radiance.bindImageLayered(_radiancePos, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

    private:
        Int _albedoPos = 0, _normalPos = 1, _emissionPos = 2, _radiancePos = 3;
};
}}

#endif