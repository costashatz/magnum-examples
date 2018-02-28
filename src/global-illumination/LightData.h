#ifndef Magnum_Examples_LightData_h
#define Magnum_Examples_LightData_h
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
#include <Magnum/Math/Vector3.h>

namespace Magnum { namespace Examples {

class LightData {
    public:
        /**
         * @brief Light type
         *
         * @see @ref type()
         */
        enum class Type: UnsignedByte {
            /**
             * Light at position that is infinitely far away so its rays are
             * parallel.
             */
            Infinite,

            /** Point light, radiating in all directions */
            Point,

            /**
             * Spot light, radiating in a limited range of direction.
             */
            Spot
        };

        /**
         * @brief Constructor
         * @param type              Light type
         * @param color             Light color
         * @param intensity         Light intensity
         * @param attenuation       Light attenuation (constant, linear, quadratic) -> defaults to (1, 0, 0)
         * 
         *
         * Position, direction and the rest of the members should be specified using member functions
         * based on the type of the light. The type is set on construction and cannot change.
         */
        explicit LightData(Type type, const Color3& color, Float intensity, Vector3 attenuation = {1.f, 0.f, 0.f}) noexcept: _type{type}, _color{color}, _position{}, _direction{0.f, 0.f, 1.f}, _spotExponent{1.f}, _spotCutoff{}, _intensity{intensity}, _constantAttenuation(attenuation[0]), _linearAttenuation(attenuation[1]), _quadraticAttenuation(attenuation[2]) {}

        /** @brief Copying is not allowed */
        LightData(const LightData&) = delete;

        /** @brief Move constructor */
        LightData(LightData&&) noexcept = default;

        /** @brief Copying is not allowed */
        LightData& operator=(const LightData&) = delete;

        /** @brief Move assignment */
        LightData& operator=(LightData&&) noexcept = default;

        /** @brief Light type */
        Type type() const { return _type; }

        /** @brief Light color */
        Color3& color() { return _color; }
        Color3 color() const { return _color; }

        /** @brief Light position 
         * available only with Point and Spot-lights
         */
        Vector3& position();
        Vector3 position() const;

        /** @brief Light direction
         * available only with Infinite and Spot-lights
         */
        Vector3& direction();
        Vector3 direction() const;

        /** @brief Light spotExponent
         * available only with Spot-lights
         */
        Float& spotExponent();
        Float spotExponent() const;

        /** @brief Light spotExponent
         * available only with Spot-lights
         */
        Float& spotCutoff();
        Float spotCutoff() const;

        /** @brief Light intensity */
        Float& intensity() { return _intensity; }
        Float intensity() const { return _intensity; }

        /** @brief Light attenuation */
        Vector3 attenuation() const { return Vector3(_constantAttenuation, _linearAttenuation, _quadraticAttenuation); }
        LightData& setAttenuation(const Vector3& attenuation) {
            _constantAttenuation = attenuation[0];
            _linearAttenuation = attenuation[1];
            _quadraticAttenuation = attenuation[2];

            return *this;
        }

        /** @brief Light constant attenuation */
        Float& constantAttenuation() { return _constantAttenuation; }
        Float constantAttenuation() const { return _constantAttenuation; }

        /** @brief Light linear attenuation */
        Float& linearAttenuation() { return _linearAttenuation; }
        Float linearAttenuation() const { return _linearAttenuation; }

        /** @brief Light quadratic attenuation */
        Float& quadraticAttenuation() { return _quadraticAttenuation; }
        Float quadraticAttenuation() const { return _quadraticAttenuation; }

    private:
        Type _type;
        Color3 _color;
        Vector3 _position;
        Vector3 _direction;
        Float _spotExponent;
        Float _spotCutoff;
        Float _intensity;
        Float _constantAttenuation;
        Float _linearAttenuation;
        Float _quadraticAttenuation;
};

inline Vector3 LightData::position() const {
    return const_cast<LightData*>(this)->position();
}

inline Vector3 LightData::direction() const {
    return const_cast<LightData*>(this)->direction();
}

inline Float LightData::spotExponent() const {
    return const_cast<LightData*>(this)->spotExponent();
}

inline Float LightData::spotCutoff() const {
    return const_cast<LightData*>(this)->spotCutoff();
}

}}

#endif