#ifndef Magnum_Shaders_RaytracingShader_h
#define Magnum_Shaders_RaytracingShader_h
/*
    This file is part of Magnum.
    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>
    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Magnum/AbstractShaderProgram.h"
#include "Magnum/Shaders/Generic.h"
#include "Magnum/Shaders/visibility.h"

#include "RaytracingTypes.h"

namespace Magnum { namespace Examples {

class RaytracingShader: public AbstractShaderProgram {
    public:

        explicit RaytracingShader();

        explicit RaytracingShader(NoCreateT) noexcept: AbstractShaderProgram{NoCreate} {}

        RaytracingShader& setCamera(const Camera& camera) {
            setUniform(_cameraUniform, camera.pos);
            setUniform(_cameraUniform + 1, camera.dir);
            setUniform(_cameraUniform + 2, camera.yAxis);
            setUniform(_cameraUniform + 3, camera.xAxis);
            setUniform(_cameraUniform + 4, camera.tanFovY);
            setUniform(_cameraUniform + 5, camera.tanFovX);
            return *this;
        }

        RaytracingShader& setViewport(UnsignedInt width, UnsignedInt height) {
            setUniform(_widthUniform, width);
            setUniform(_heightUniform, height);
            return *this;
        }

        RaytracingShader& setNumObjects(UnsignedInt numObjects) {
            setUniform(_numObjectsUniform, numObjects);
            return *this;
        }

        RaytracingShader& setNumLights(UnsignedInt numLights) {
            setUniform(_numLightsUniform, numLights);
            return *this;
        }

        RaytracingShader& setReflectionDepth(UnsignedInt depth) {
            setUniform(_reflectionDepthUniform, depth);
            return *this;
        }

        RaytracingShader& setNumMeshes(UnsignedInt numMeshes) {
            setUniform(_numMeshesUniform, numMeshes);
            return *this;
        }

        RaytracingShader& setReflectionDecay(Float reflectionDecay) {
            setUniform(_reflectionDecayUniform, reflectionDecay);
            return *this;
        }

        RaytracingShader& setSceneParams(UnsignedInt numObjects, UnsignedInt numLights, UnsignedInt numMeshes, UnsignedInt depth) {
            this->setNumObjects(numObjects)
                .setNumMeshes(numMeshes)
                .setNumLights(numLights)
                .setReflectionDepth(depth);
            return *this;
        }

        Int objectBufferBindLocation() { return _objectBufferBindLocation; }
        Int materialBufferBindLocation() { return _materialBufferBindLocation; }
        Int lightBufferBindLocation() { return _lightBufferBindLocation; }
        Int meshBufferBindLocation() { return _meshBufferBindLocation; }

        RaytracingShader& setOutputTexture(Texture2D& texture);

    private:
        Int _cameraUniform{0},
            _widthUniform{6},
            _heightUniform{7},
            _numObjectsUniform{8},
            _numLightsUniform{9},
            _reflectionDepthUniform{10},
            _numMeshesUniform{12},
            _reflectionDecayUniform{13};
        Int _objectBufferBindLocation{3},
            _materialBufferBindLocation{4},
            _lightBufferBindLocation{5},
            _outputTextureBindLocation{6},
            _meshBufferBindLocation{11};
};

}}

#endif