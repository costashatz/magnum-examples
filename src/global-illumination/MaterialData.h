#ifndef Magnum_Examples_MaterialData_h
#define Magnum_Examples_MaterialData_h
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

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>

namespace Magnum { namespace Examples {

class MaterialData {
    public:
        enum: UnsignedInt {
            AmbientTextureID = 0,   /**< Ambient texture ID for mapping with texture coordinates */
            DiffuseTextureID = 1,   /**< Diffuse texture ID for mapping with texture coordinates */
            SpecularTextureID = 3   /**< Specular texture ID for mapping with texture coordinates */
        };

        /**
         * @brief Material flag
         *
         * @see @ref Flags, @ref flags()
         */
        enum class Flag: UnsignedByte {
            AmbientTexture = 1 << 0,    /**< The material has ambient texture instead of color */
            DiffuseTexture = 1 << 1,    /**< The material has diffuse texture instead of color */
            SpecularTexture = 1 << 2    /**< The material has specular texture instead of color */
        };

        /**
         * @brief Material flags
         *
         * @see @ref flags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Constructor
         * @param flags             Material flags
         * @param shininess         Shininess
         *
         * Colors and textures should be specified using member functions based
         * on what flags are set.
         */
        explicit MaterialData(Flags flags, Float shininess) noexcept: _flags{flags}, _shininess{shininess} {}

        /** @brief Copying is not allowed */
        MaterialData(const MaterialData&) = delete;

        /** @brief Move constructor */
        MaterialData(MaterialData&& other) noexcept;

        /** @brief Copying is not allowed */
        MaterialData& operator=(const MaterialData&) = delete;

        /** @brief Move assignment */
        MaterialData& operator=(MaterialData&& other) noexcept;

        /** @brief Material flags */
        Flags flags() const { return _flags; }

        /**
         * @brief Ambient color
         *
         * Available only if the material doesn't have @ref Flag::AmbientTexture.
         * @see @ref flags()
         */
        Color4& ambientColor();
        Color4 ambientColor() const; /**< @overload */

        /**
         * @brief Ambient texture ID
         *
         * Available only if the material has @ref Flag::AmbientTexture.
         * @see @ref flags(), @ref AbstractImporter::texture()
         */
        UnsignedInt& ambientTexture();
        UnsignedInt ambientTexture() const; /**< @overload */

        /**
         * @brief Diffuse color
         *
         * Available only if the material doesn't have @ref Flag::DiffuseTexture.
         * @see @ref flags()
         */
        Color4& diffuseColor();
        Color4 diffuseColor() const; /**< @overload */

        /**
         * @brief Diffuse texture ID
         *
         * Available only if the material has @ref Flag::DiffuseTexture.
         * @see @ref flags(), @ref AbstractImporter::texture()
         */
        UnsignedInt& diffuseTexture();
        UnsignedInt diffuseTexture() const; /**< @overload */

        /**
         * @brief Specular color
         *
         * Available only if the material doesn't have @ref Flag::SpecularTexture.
         * @see @ref flags()
         */
        Color4& specularColor();
        Color4 specularColor() const; /**< @overload */

        /**
         * @brief Specular texture ID
         *
         * Available only if the material has @ref Flag::SpecularTexture.
         * @see @ref flags(), @ref AbstractImporter::texture()
         */
        UnsignedInt& specularTexture();
        UnsignedInt specularTexture() const; /**< @overload */

        /** @brief Shininess */
        Float shininess() const { return _shininess; }

        /**
         * @brief Emissive color
         */
        Color4& emissiveColor();
        Color4 emissiveColor() const; /**< @overload */

    private:
        union Source {
            Source() {}

            Color4 color;
            UnsignedInt texture;
        };

        Flags _flags;
        Float _shininess;
        Source _ambient,
            _diffuse,
            _specular;
        Color4 _emissive;
};

CORRADE_ENUMSET_OPERATORS(MaterialData::Flags)

inline Color4 MaterialData::ambientColor() const {
    return const_cast<MaterialData*>(this)->ambientColor();
}

inline UnsignedInt MaterialData::ambientTexture() const {
    return const_cast<MaterialData*>(this)->ambientTexture();
}

inline Color4 MaterialData::diffuseColor() const {
    return const_cast<MaterialData*>(this)->diffuseColor();
}

inline UnsignedInt MaterialData::diffuseTexture() const {
    return const_cast<MaterialData*>(this)->diffuseTexture();
}

inline Color4 MaterialData::specularColor() const {
    return const_cast<MaterialData*>(this)->specularColor();
}

inline UnsignedInt MaterialData::specularTexture() const {
    return const_cast<MaterialData*>(this)->specularTexture();
}

inline Color4 MaterialData::emissiveColor() const {
    return const_cast<MaterialData*>(this)->emissiveColor();
}

}}

#endif