#ifndef VCTEXAMPLE_RENDER_TEXTURE_SHADER_H
#define VCTEXAMPLE_RENDER_TEXTURE_SHADER_H

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>

namespace Magnum { namespace Examples {

class RenderTextureShader: public GL::AbstractShaderProgram {
    public:
        explicit RenderTextureShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit RenderTextureShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

            vert.addSource(rs.get("RenderTextureShader.vert"));
            frag.addSource(rs.get("RenderTextureShader.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _isDepthUniform = uniformLocation("isDepth");
            /* By default not depth */
            setDepth(0);
        }

        RenderTextureShader& setDepth(Int isDepth) {
            setUniform(_isDepthUniform, isDepth);
            return *this;
        }

        RenderTextureShader& bindOutputTexture(GL::Texture2D& tex) {
            tex.bind(_texturePos);
            return *this;
        }

    private:
        Int _isDepthUniform;
        Int _texturePos = 0;
};
}}

#endif