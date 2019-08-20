#ifndef VCTEXAMPLE_MIPMAP_VOLUME_SHADER_H
#define VCTEXAMPLE_MIPMAP_VOLUME_SHADER_H

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

class MipMapVolumeShader: public GL::AbstractShaderProgram {
    public:
        explicit MipMapVolumeShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit MipMapVolumeShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader comp{GL::Version::GL430, GL::Shader::Type::Compute};

            comp.addSource(rs.get("MipMapVolumeShader.comp"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({comp}));

            attachShaders({comp});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _mipDimensionUniform = uniformLocation("mipDimension");
            _mipLevelUniform = uniformLocation("mipLevel");
        }

        MipMapVolumeShader& setMipDimension(UnsignedInt dim) {
            setUniform(_mipDimensionUniform, dim);
            return *this;
        }

        MipMapVolumeShader& setMipLevel(Int level) {
            setUniform(_mipLevelUniform, level);
            return *this;
        }

        MipMapVolumeShader& bindMipMapSourceTextures(GL::Texture3D* texs) {
            for(UnsignedInt i = 0; i < 6; i++)
                texs[i].bind(_mipMapSourcePos + i);
            return *this;
        }

        MipMapVolumeShader& bindMipMapDestTextures(GL::Texture3D* texs, Int level) {
            for(UnsignedInt i = 0; i < 6; i++)
                texs[i].bindImage(_mipMapDestPos + i, level, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

    private:
        Int _mipMapSourcePos = 6, _mipMapDestPos = 0;
        Int _mipDimensionUniform, _mipLevelUniform;
};
}}

#endif