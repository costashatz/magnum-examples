/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>

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

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Trade/MeshData3D.h>

#include "SimpleShader.h"
#include "RaytracingShader.h"

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class RaytracingExample: public Platform::Application {
    public:
        explicit RaytracingExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(const Vector2i& size) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2i& _position) const;

        Scene3D _scene;
        Object3D *_o, *_cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;
        SimpleShader _shader;

        Buffer _shaderBuffer;
};

struct Material {
    Vector3 ambientColor,
            diffuseColor,
            specularColor;
    Float shininess;
};

class ColoredObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit ColoredObject(Mesh mesh, Buffer vertexBuffer, Buffer indexBuffer, SimpleShader& shader, const Material& material, Object3D* parent, SceneGraph::DrawableGroup3D* group) : Object3D{parent}, SceneGraph::Drawable3D{*this, group}, _mesh(std::move(mesh)), _vertexBuffer(std::move(vertexBuffer)), _indexBuffer(std::move(indexBuffer)), _shader{shader}, _material(material) {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
            _shader.setAmbientColor(_material.ambientColor)
                .setDiffuseColor(_material.diffuseColor)
                .setSpecularColor(_material.specularColor)
                .setShininess(_material.shininess)
                .setLightPosition({-3.0f, 10.0f, 10.0f})
                .setCameraMatrix(camera.cameraMatrix())
                .setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.rotation())
                .setProjectionMatrix(camera.projectionMatrix());

            _mesh.draw(_shader);
        }

        Mesh _mesh;
        Buffer _vertexBuffer, _indexBuffer;
        SimpleShader& _shader;
        Material _material;
};

RaytracingExample::RaytracingExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Raycasting Example")}
{
    /* Every scene needs a camera */
    (*(_cameraObject = new Object3D{&_scene}))
        .translate(Vector3::zAxis(10.0f));
    (*(_camera = new SceneGraph::Camera3D{*_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 100.0f))
        .setViewport(defaultFramebuffer.viewport().size());
    
    /* Default object, parent of all (for manipulation) */
    _o = new Object3D{&_scene};

    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    /* create shader */
    _shader = SimpleShader{};

    RaytracingShader rayShader{};

    Trade::MeshData3D cube = Primitives::Cube::solid();
    // MeshTools::transformPointsInPlace(Matrix4::scaling(Vector3{0.1f}), cube.positions(0));
    /* Create the mesh */
    Mesh mesh{NoCreate};
    std::unique_ptr<Buffer> vertexBuffer, indexBuffer;
    std::tie(mesh, vertexBuffer, indexBuffer) = MeshTools::compile(cube, BufferUsage::StaticDraw);

    Material material;
    material.ambientColor = Vector3{0.f, 0.f, 0.f};
    material.diffuseColor = Vector3{0.3f, 0.3f, 0.f};
    material.specularColor = Vector3{1.f};
    material.shininess = 80.f;

    // struct Object{
    //     Vector3 position;
    // };

    // std::vector<Object> data;
    // Object ob1, ob2, ob3;
    // ob1.position = Vector3{1.f, 0.f, 0.f};
    // ob2.position = Vector3{0.f, 1.f, 0.f};
    // ob3.position = Vector3{0.f, 0.f, 1.f};
    // data.push_back(ob1);
    // data.push_back(ob2);
    // data.push_back(ob3);
    // _shaderBuffer.setData(data, BufferUsage::DynamicCopy);
    // _shaderBuffer.bind(Buffer::Target::ShaderStorage, 3);

    new ColoredObject(std::move(mesh), std::move(*vertexBuffer.release()), std::move(*indexBuffer.release()), _shader, material, _o, &_drawables);
}

void RaytracingExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color | FramebufferClear::Depth);
    _camera->draw(_drawables);
    swapBuffers();
}

void RaytracingExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    _camera->setViewport(size);
}

void RaytracingExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());
}

void RaytracingExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3();
}

void RaytracingExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    Float distance = _cameraObject->transformation().translation().z();

    /* Move 15% of the distance back or forward */
    distance *= 1 - (event.offset().y() > 0 ? 1/0.85f : 0.85f);
    _cameraObject->translate(Vector3::zAxis(distance));

    redraw();
}

Vector3 RaytracingExample::positionOnSphere(const Vector2i& position) const {
    Vector2 positionNormalized = Vector2(position*2)/Vector2(_camera->viewport()) - Vector2(1.0f);

    Float length = positionNormalized.length();
    Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
}

void RaytracingExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    Vector3 currentPosition = positionOnSphere(event.position());

    Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _cameraObject->rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());

    _previousPosition = currentPosition;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::RaytracingExample)
