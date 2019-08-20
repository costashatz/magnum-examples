#ifndef VCTEXAMPLE_GEOMETRY_SHADER_H
#define VCTEXAMPLE_GEOMETRY_SHADER_H

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

namespace Magnum { namespace Examples {

class GeometryShader: public GL::AbstractShaderProgram {
    public:
        explicit GeometryShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit GeometryShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("vct-data");

            GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

            vert.addSource(rs.get("GeometryShader.vert"));
            frag.addSource(rs.get("GeometryShader.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            // /* Get uniform locations */
            _transformationMatrixUniform = uniformLocation("transformationMatrix");
            _normalMatrixUniform = uniformLocation("normalMatrix");
            _diffuseColorUniform = uniformLocation("diffuseColor");
            _specularColorUniform = uniformLocation("specularColor");
            _shininessUniform = uniformLocation("shininess");
            _emissiveColorUniform = uniformLocation("emissiveColor");
        }

        GeometryShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        GeometryShader& setNormalMatrix(const Matrix3& matrix) {
            setUniform(_normalMatrixUniform, matrix);
            return *this;
        }

        GeometryShader& setDiffuseColor(const Color3& color) {
            setUniform(_diffuseColorUniform, color);
            return *this;
        }

        GeometryShader& setSpecularColor(const Color3& color) {
            setUniform(_specularColorUniform, color);
            return *this;
        }

        GeometryShader& setShininess(Float shininess) {
            setUniform(_shininessUniform, shininess);
            return *this;
        }

        GeometryShader& setEmissiveColor(const Color3& color) {
            setUniform(_emissiveColorUniform, color);
            return *this;
        }

    private:
        Int _transformationMatrixUniform,
            _normalMatrixUniform,
            _diffuseColorUniform,
            _specularColorUniform,
            _shininessUniform,
            _emissiveColorUniform;
};
}}

#endif