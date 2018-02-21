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

#include <Corrade/Containers/Array.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Image.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Renderer.h>
// #include <Magnum/ResourceManager.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>

#include "VoxelBaseShader.h"
#include "VoxelizationShader.h"
#include "VoxelConeTracingShader.h"

#include "configure.h"

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

struct ObjectMaterial {
    Material material;
    MaterialSettings settings;
};

class VoxelConeTracingExample: public Platform::Application {
    public:
        explicit VoxelConeTracingExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2i& _position) const;

        Scene3D _scene;
        Object3D *_o, *_cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        SceneGraph::DrawableGroup3D _voxels, _voxelTracers;
        Vector3 _previousPosition;
        std::shared_ptr<Shaders::Phong> _phongShader;
        std::shared_ptr<VoxelizationShader> _voxelizationShader;
        std::shared_ptr<VoxelConeTracingShader> _voxelConeTracingShader;
        PointLight _light;
        std::unique_ptr<Texture3D> _voxelTexture;
};

class DrawableObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit DrawableObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const ObjectMaterial& material, const std::shared_ptr<Shaders::Phong>& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        std::shared_ptr<Mesh> _mesh;
        std::shared_ptr<Buffer> _vertexBuffer, _indexBuffer;
        ObjectMaterial _material;
        std::shared_ptr<Shaders::Phong> _shader;
};

class VoxelizationObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit VoxelizationObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const ObjectMaterial& material, const std::shared_ptr<VoxelizationShader>& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group, Texture3D& voxelTexture, PointLight& light);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        std::shared_ptr<Mesh> _mesh;
        std::shared_ptr<Buffer> _vertexBuffer, _indexBuffer;
        ObjectMaterial _material;
        std::shared_ptr<VoxelizationShader> _shader;
        Texture3D& _voxelTexture;
        PointLight& _light;
};

class VoxelConeTracingObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit VoxelConeTracingObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const ObjectMaterial& material, const std::shared_ptr<VoxelConeTracingShader>& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group, Texture3D& voxelTexture, PointLight& light);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        std::shared_ptr<Mesh> _mesh;
        std::shared_ptr<Buffer> _vertexBuffer, _indexBuffer;
        ObjectMaterial _material;
        std::shared_ptr<VoxelConeTracingShader> _shader;
        Texture3D& _voxelTexture;
        PointLight& _light;
};

VoxelConeTracingExample::VoxelConeTracingExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Voxel Cone Triangle Example")}
{
    /* Every scene needs a camera */
    (*(_cameraObject = new Object3D{&_scene}))
        .translate(Vector3::zAxis(1.0f));
    (*(_camera = new SceneGraph::Camera3D{*_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 100.0f))
        .setViewport(defaultFramebuffer.viewport().size());

    /* create a basic shader */
    _phongShader = std::make_shared<Shaders::Phong>();
    _voxelizationShader = std::make_shared<VoxelizationShader>();
    _voxelConeTracingShader = std::make_shared<VoxelConeTracingShader>();

    /* create a default material */
    ObjectMaterial material;
    material.material.diffuseColor = Vector3(0.2f, 0.0f, 0.8f);
    material.material.specularColor = Vector3(1.f);
    material.material.specularDiffusion = 0.2f;
    material.material.diffuseReflectivity = 1.f;
    material.material.specularReflectivity = 0.2f;
    material.material.emissivity = 0.f;
    material.material.refractiveIndex = 1.4f;
    material.material.transparency = 0.f;

    // material.settings.indirectSpecularLight = true;
    // material.settings.indirectDiffuseLight = true;
    // material.settings.directLight = true;
    // material.settings.shadows = true;
    material.settings.indirectSpecularLight = false;
    material.settings.indirectDiffuseLight = false;
    material.settings.directLight = true;
    material.settings.shadows = true;

    /* Default object, parent of all (for manipulation) */
    _o = new Object3D{&_scene};

    /* create the Voxel texture */
    Image3D image{PixelFormat::RGBA, PixelType::Float, {64, 64, 64}, Containers::Array<char>{Math::pow<3>(64)*16}};
    _voxelTexture = std::unique_ptr<Texture3D>(new Texture3D);
    _voxelTexture->setMagnificationFilter(Sampler::Filter::Nearest)
                  .setMinificationFilter(Sampler::Filter::Linear, Sampler::Mipmap::Linear)
                  .setWrapping(Sampler::Wrapping::ClampToBorder)
                  .setStorage(7, TextureFormat::RGBA8, {64, 64, 64})
                  .setSubImage(0, {}, image)
                  .generateMipmap();

    // _voxelizationShader->setVoxelTexture(*_voxelTexture);
    // _voxelConeTracingShader->setVoxelTexture(*_voxelTexture);

    /* add one light to the scene */
    _light.position = Vector3{0.f, 0.5f, 0.f}; //Vector3{-3.0f, 10.0f, 10.0f};
    _light.color = Vector3{1.0f, 1.0f, 1.0f};
    // _voxelizationShader->setNumberOfLights(1).setLight(0, light);
    // _voxelConeTracingShader->setNumberOfLights(1).setLight(0, light);

    /* create a simple cube */
    Trade::MeshData3D cube = Primitives::Cube::solid();
    MeshTools::transformPointsInPlace(Matrix4::scaling(Vector3{0.1f}), cube.positions(0));
    /* Create the mesh */
    Mesh* mesh = new Mesh{NoCreate};
    std::unique_ptr<Buffer> vertexBuffer, indexBuffer;
    std::tie(*mesh, vertexBuffer, indexBuffer) = MeshTools::compile(cube, BufferUsage::StaticDraw);

    auto mp = std::make_shared<Mesh>(std::move(*mesh));
    auto vb = std::make_shared<Buffer>(std::move(*vertexBuffer.release()));
    auto ib = std::make_shared<Buffer>(std::move(*indexBuffer.release()));

    auto obj = new Object3D{&_scene};
    // new DrawableObject(mp, vb, ib, material, _phongShader, obj, &_drawables);
    new VoxelizationObject(mp, vb, ib, material, _voxelizationShader, obj, &_voxels, *_voxelTexture, _light);
    new VoxelConeTracingObject(mp, vb, ib, material, _voxelConeTracingShader, obj, &_voxelTracers, *_voxelTexture, _light);

    /* create a simple cube */
    Trade::MeshData3D cube2 = Primitives::Cube::solid();
    MeshTools::transformPointsInPlace(Matrix4::scaling(Vector3{0.5f, 0.01f, 0.2f}), cube2.positions(0));
    /* Create the mesh */
    Mesh* mesh2 = new Mesh{NoCreate};
    std::unique_ptr<Buffer> vertexBuffer2, indexBuffer2;
    std::tie(*mesh2, vertexBuffer2, indexBuffer2) = MeshTools::compile(cube2, BufferUsage::StaticDraw);

    auto mp2 = std::make_shared<Mesh>(std::move(*mesh2));
    auto vb2 = std::make_shared<Buffer>(std::move(*vertexBuffer2.release()));
    auto ib2 = std::make_shared<Buffer>(std::move(*indexBuffer2.release()));
    auto obj2 = new Object3D{&_scene};
    obj2->translate(Vector3{0.f, -0.15f, 0.f});
    ObjectMaterial material2 = material;
    material2.material.diffuseColor = Vector3(1.f, 1.f, 1.f);
    material2.material.emissivity = 0.f;
    material.material.specularReflectivity = 1.f;
    new VoxelizationObject(mp2, vb2, ib2, material2, _voxelizationShader, obj2, &_voxels, *_voxelTexture, _light);
    new VoxelConeTracingObject(mp2, vb2, ib2, material2, _voxelConeTracingShader, obj2, &_voxelTracers, *_voxelTexture, _light);
}

void VoxelConeTracingExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    _camera->setViewport(size);
}

void VoxelConeTracingExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    Range2Di viewport = defaultFramebuffer.viewport();
    Vector2i viewportSize = viewport.size();

    Renderer::disable(Renderer::Feature::DepthTest);
    Renderer::disable(Renderer::Feature::FaceCulling);
    Renderer::disable(Renderer::Feature::Blending);
    Renderer::setColorMask(false, false, false, false);

    /* clear voxelTexture */
    Image3D image{PixelFormat::RGBA, PixelType::Float, {64, 64, 64}, Containers::Array<char>{Math::pow<3>(64)*16}};
    _voxelTexture->setSubImage(0, {}, image).generateMipmap();

    /* change viewport size */
    defaultFramebuffer.setViewport({{}, {64, 64}});
    _camera->setViewport({64, 64});

    _camera->draw(_voxels);
    /* maybe regenerate mipmaps? */

    /* restore renderer/framebuffer */
    defaultFramebuffer.setViewport({{}, viewportSize});
    _camera->setViewport(viewportSize);

    Renderer::setColorMask(true, true, true, true);
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);
    Renderer::enable(Renderer::Feature::Blending);
    Renderer::setBlendFunction(Renderer::BlendFunction::SourceAlpha, Renderer::BlendFunction::OneMinusSourceAlpha);

    /* draw meshes */
    _camera->draw(_voxelTracers);
    swapBuffers();
}

void VoxelConeTracingExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());
}

void VoxelConeTracingExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3();
}

void VoxelConeTracingExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    Float distance = _cameraObject->transformation().translation().z();

    /* Move 15% of the distance back or forward */
    distance *= 1 - (event.offset().y() > 0 ? 1/0.85f : 0.85f);
    _cameraObject->translate(Vector3::zAxis(distance));

    redraw();
}

Vector3 VoxelConeTracingExample::positionOnSphere(const Vector2i& position) const {
    Vector2 positionNormalized = Vector2(position*2)/Vector2(_camera->viewport()) - Vector2(1.0f);

    Float length = positionNormalized.length();
    Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
}

void VoxelConeTracingExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    Vector3 currentPosition = positionOnSphere(event.position());

    Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _cameraObject->rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());

    _previousPosition = currentPosition;

    redraw();
}

DrawableObject::DrawableObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const ObjectMaterial& material, const std::shared_ptr<Shaders::Phong>& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _mesh{mesh}, _vertexBuffer(vertexBuffer), _indexBuffer(indexBuffer), _material(material), _shader{shader}
{}

void DrawableObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
     _shader->setAmbientColor(Vector3{0.01f})
        .setDiffuseColor(_material.material.diffuseColor)
        .setSpecularColor(_material.material.specularColor)
        .setShininess(80.f)
        .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix());

    _mesh->draw(*_shader);
}

VoxelizationObject::VoxelizationObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const ObjectMaterial& material, const std::shared_ptr<VoxelizationShader>& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group, Texture3D& voxelTexture, PointLight& light):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _mesh{mesh}, _vertexBuffer(vertexBuffer), _indexBuffer(indexBuffer), _material(material), _shader{shader}, _voxelTexture(voxelTexture), _light(light)
{}

void VoxelizationObject::draw(const Matrix4&, SceneGraph::Camera3D& camera) {
    _light.transformedPosition = camera.cameraMatrix().transformPoint(_light.position);
    _shader->setModelMatrix(absoluteTransformationMatrix())
           .setMaterial(_material.material)
           .setVoxelTexture(_voxelTexture)
           .setNumberOfLights(1)
           .setLight(0, _light);

    _mesh->draw(*_shader);
}

VoxelConeTracingObject::VoxelConeTracingObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const ObjectMaterial& material, const std::shared_ptr<VoxelConeTracingShader>& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group, Texture3D& voxelTexture, PointLight& light):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _mesh{mesh}, _vertexBuffer(vertexBuffer), _indexBuffer(indexBuffer), _material(material), _shader{shader}, _voxelTexture(voxelTexture), _light(light)
{}

void VoxelConeTracingObject::draw(const Matrix4&, SceneGraph::Camera3D& camera) {
    _light.transformedPosition = camera.cameraMatrix().transformPoint(_light.position);
    _shader->setViewMatrix(camera.cameraMatrix())
           .setProjectionMatrix(camera.projectionMatrix())
           .setCameraPosition(camera.object().transformationMatrix().translation())
           .setMaterial(_material.material)
           .setMaterialSettings(_material.settings)
           .setModelMatrix(absoluteTransformationMatrix())
           .bindVoxelTexture(_voxelTexture)
           .setNumberOfLights(1)
           .setLight(0, _light);

    _mesh->draw(*_shader);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::VoxelConeTracingExample)
