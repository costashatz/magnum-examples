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

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Image.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Renderer.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Trade/MeshData3D.h>

#include "VoxelizationShader.h"
#include "VoxelVisualizationShader.h"
#include "VoxelConeTracingShader.h"

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class GlobalIlluminationExample: public Platform::Application {
    public:
        explicit GlobalIlluminationExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(const Vector2i& size) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2i& _position) const;

        /* Camera manipulation */
        Vector3 _previousPosition;

        /* Scene */
        Scene3D _scene;
        Object3D *_o, *_cameraObject;
        SceneGraph::Camera3D* _camera;

        /* voxelization */
        SceneGraph::DrawableGroup3D _voxels;
        VoxelizationShader _voxelShader;
        VoxelVisualizationShader _voxelVisualShader;
        Texture3D _voxelTexture;
        const Int _voxelDimensions = 128;
        const Float _voxelGridWorldSize = 10.0f;
        Matrix4 _voxelProjectionMatX, _voxelProjectionMatY, _voxelProjectionMatZ;
        Mesh _debugVoxelsMesh;
        Buffer _dummy;

        /* global illumination */
        SceneGraph::DrawableGroup3D _illuminationObjects;
        VoxelConeTracingShader _voxelConeShader;
        std::unique_ptr<LightData> _light;
};

struct BufferMesh {
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
};

class VoxelizedObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit VoxelizedObject(BufferMesh& mesh, const Color4& color, VoxelizationShader& voxelShader, Object3D* parent, SceneGraph::DrawableGroup3D* group) : Object3D{parent}, SceneGraph::Drawable3D{*this, group}, _mesh(std::move(mesh)), _color(color), _voxelShader(&voxelShader) {}

        BufferMesh& mesh() { return _mesh; }

    private:
        void draw(const Matrix4& /* transformationMatrix */, SceneGraph::Camera3D& /* camera */) override {
            _voxelShader->setTransformationMatrix(absoluteTransformation())
                         .setDiffuseColor(_color);
            _mesh.mesh->draw(*_voxelShader);
        }

        BufferMesh _mesh;
        Color4 _color;
        VoxelizationShader* _voxelShader;
};

class GlobalIlluminationObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit GlobalIlluminationObject(BufferMesh& mesh, MaterialData& material, VoxelConeTracingShader& voxelConeShader, Object3D* parent, SceneGraph::DrawableGroup3D* group) : Object3D{parent}, SceneGraph::Drawable3D{*this, group}, _mesh(mesh), _material(std::move(material)), _voxelConeShader(&voxelConeShader) {}

    private:
        void draw(const Matrix4&  transformationMatrix, SceneGraph::Camera3D& camera) override {
            _voxelConeShader->setTransformationMatrix(camera.projectionMatrix() * transformationMatrix)
                             .setModelMatrix(absoluteTransformation())
                            //  .setCameraPosition(camera.object().transformationMatrix().translation())
                             .setMaterial(_material);
            _mesh.mesh->draw(*_voxelConeShader);
        }

        BufferMesh& _mesh;
        MaterialData _material;
        VoxelConeTracingShader* _voxelConeShader;
};

GlobalIlluminationExample::GlobalIlluminationExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Global Illumination Example")}
{
    /* Create the Voxel texture */
    Containers::Array<Color4ub> tmp{Containers::DirectInit, Math::pow<3>(static_cast<UnsignedInt>(_voxelDimensions)), Color4ub(0, 0)};
    ImageView3D image{PixelFormat::RGBA, PixelType::UnsignedByte, Vector3i{_voxelDimensions}, tmp};
    _voxelTexture.setMagnificationFilter(Sampler::Filter::Nearest)
                 .setMinificationFilter(Sampler::Filter::Linear, Sampler::Mipmap::Linear)
                 .setWrapping(Sampler::Wrapping::ClampToBorder)
                 .setStorage(7, TextureFormat::RGBA8, Vector3i{_voxelDimensions})
                 .setSubImage(0, {}, image)
                 .generateMipmap();

    /* Create projection matrices used to project vertices
     * onto each axis in the voxelization step
     * */
	Float size = _voxelGridWorldSize;
    Matrix4 projectionMatrix = Matrix4::orthographicProjection({size, size}, size * 0.5f, size * 1.5f);
    /* Magnum has inverted lookAt compared to glm/GL */
    _voxelProjectionMatX = projectionMatrix * Matrix4::lookAt({size, 0.f, 0.f}, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}).invertedRigid();
    _voxelProjectionMatY = projectionMatrix * Matrix4::lookAt({0.f, size, 0.f}, {0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}).invertedRigid();
    _voxelProjectionMatZ = projectionMatrix * Matrix4::lookAt({0.f, 0.f, size}, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}).invertedRigid();

    /* create debug mesh for drawing voxels */
    _debugVoxelsMesh.setPrimitive(MeshPrimitive::Points)
                    .setCount(_voxelDimensions * _voxelDimensions * _voxelDimensions);

    /* Every scene needs a camera */
    (*(_cameraObject = new Object3D{&_scene}));
    (*(_camera = new SceneGraph::Camera3D{*_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 100.0f))
        .setViewport(defaultFramebuffer.viewport().size());
    
    /* Camera located at (2,3,10) looking in the center */
    _cameraObject->setTransformation(Matrix4::lookAt({2.f, 3.f, 10.f}, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}));

    /* Default object, parent of all */
    _o = new Object3D{&_scene};

    /* Create cube */
    Trade::MeshData3D cube = Primitives::Cube::solid();
    Mesh mesh{NoCreate};
    std::unique_ptr<Buffer> vertexBuffer, indexBuffer;
    std::tie(mesh, vertexBuffer, indexBuffer) = MeshTools::compile(cube, BufferUsage::StaticDraw);
    BufferMesh cubeMesh;
    cubeMesh.mesh = std::unique_ptr<Mesh>(new Mesh{std::move(mesh)});
    cubeMesh.vertexBuffer = std::move(vertexBuffer);
    cubeMesh.indexBuffer = std::move(indexBuffer);

    MaterialData cubeMaterial{{}, 80.f};
    cubeMaterial.ambientColor() = Color4(0.f, 0.f, 0.f, 1.f);
    cubeMaterial.diffuseColor() = Color4(0.5f, 0.5f, 0.5f, 1.f);
    cubeMaterial.specularColor() = Color4(1.f);
    cubeMaterial.emissiveColor() = Color4(0.f, 0.f, 0.f, 1.f);

    auto cubeObject = new Object3D{_o};
    cubeObject->rotateYLocal(65.0_degf);
    //            .rotateXLocal(35.0_degf)
    //            .rotateZLocal(-55.0_degf);

    /* Create cube object for being voxelized */
    auto voxelCube = new VoxelizedObject(cubeMesh, cubeMaterial.diffuseColor(), _voxelShader, cubeObject, &_voxels);
    /* Create cube object for visualization */
    new GlobalIlluminationObject(voxelCube->mesh(), cubeMaterial, _voxelConeShader, cubeObject, &_illuminationObjects);

    /* Create floor */
    Trade::MeshData3D floor = Primitives::Cube::solid();
    MeshTools::transformPointsInPlace(Matrix4::scaling(Vector3{10.f, 0.2f, 10.f} * 0.5f), floor.positions(0));
    std::tie(mesh, vertexBuffer, indexBuffer) = MeshTools::compile(floor, BufferUsage::StaticDraw);
    BufferMesh floorMesh;
    floorMesh.mesh = std::unique_ptr<Mesh>(new Mesh{std::move(mesh)});
    floorMesh.vertexBuffer = std::move(vertexBuffer);
    floorMesh.indexBuffer = std::move(indexBuffer);

    MaterialData floorMaterial{{}, 80.f};
    floorMaterial.ambientColor() = Color4(0.f, 0.f, 0.f, 1.f);
    floorMaterial.diffuseColor() = Color4(0.f, 0.2f, 0.9f, 1.f);
    floorMaterial.specularColor() = Color4(1.f);
    floorMaterial.emissiveColor() = Color4(0.f, 0.f, 0.f, 1.f);

    auto floorObject = new Object3D{_o};
    floorObject->translate(Vector3{0.f, -1.1f, 0.f});

    /* Create floor object for being voxelized */
    auto voxelFloor = new VoxelizedObject(floorMesh, floorMaterial.diffuseColor(), _voxelShader, floorObject, &_voxels);
    /* Create floor object for visualization */
    new GlobalIlluminationObject(voxelFloor->mesh(), floorMaterial, _voxelConeShader, floorObject, &_illuminationObjects);

    // /* Create wall */
    // Trade::MeshData3D wall1 = Primitives::Cube::solid();
    // MeshTools::transformPointsInPlace(Matrix4::scaling(Vector3{10.f, 10.f, 0.1f} * 0.5f), wall1.positions(0));
    // std::tie(mesh, vertexBuffer, indexBuffer) = MeshTools::compile(floor, BufferUsage::StaticDraw);
    // BufferMesh wall1Mesh;
    // wall1Mesh.mesh = std::unique_ptr<Mesh>(new Mesh{std::move(mesh)});
    // wall1Mesh.vertexBuffer = std::move(vertexBuffer);
    // wall1Mesh.indexBuffer = std::move(indexBuffer);

    // MaterialData wall1Material{{}, 80.f};
    // wall1Material.ambientColor() = Color4(0.f, 0.f, 0.f, 1.f);
    // wall1Material.diffuseColor() = Color4(0.f, 0.2f, 0.9f, 1.f);
    // wall1Material.specularColor() = Color4(1.f);
    // wall1Material.emissiveColor() = Color4(0.f, 0.f, 0.f, 1.f);

    // auto wall1Object = new Object3D{_o};
    // wall1Object->rotateX(90.0_degf)
    //             .translate(Vector3{0.f, 0.f, -5.f});

    // /* Create wall object for being voxelized */
    // auto voxelWall1 = new VoxelizedObject(wall1Mesh, wall1Material.diffuseColor(), _voxelShader, wall1Object, &_voxels);
    // /* Create floor object for visualization */
    // new GlobalIlluminationObject(voxelWall1->mesh(), wall1Material, _voxelConeShader, wall1Object, &_illuminationObjects);

    /* Create sphere */
    Trade::MeshData3D sphere = Primitives::Icosphere::solid(4);
    std::tie(mesh, vertexBuffer, indexBuffer) = MeshTools::compile(sphere, BufferUsage::StaticDraw);
    BufferMesh sphereMesh;
    sphereMesh.mesh = std::unique_ptr<Mesh>(new Mesh{std::move(mesh)});
    sphereMesh.vertexBuffer = std::move(vertexBuffer);
    sphereMesh.indexBuffer = std::move(indexBuffer);

    MaterialData sphereMaterial{{}, 80.f};
    sphereMaterial.ambientColor() = Color4(0.f, 0.f, 0.f, 1.f);
    sphereMaterial.diffuseColor() = Color4(0.8f, 0.2f, 0.2f, 1.f);
    sphereMaterial.specularColor() = Color4(1.f);
    sphereMaterial.emissiveColor() = Color4(0.f, 0.f, 0.f, 1.f);

    auto sphereObject = new Object3D{_o};
    sphereObject->translate(Vector3{0.f, 2.1f, 0.f});
    // sphereObject->translate(Vector3{-2.f, 2.f, -1.f})
    //              .rotateYLocal(65.0_degf)
    //              .rotateZLocal(55.0_degf);

    /* Create sphere object for being voxelized */
    auto voxelSphere = new VoxelizedObject(sphereMesh, sphereMaterial.diffuseColor(), _voxelShader, sphereObject, &_voxels);
    /* Create sphere object for visualization */
    new GlobalIlluminationObject(voxelSphere->mesh(), sphereMaterial, _voxelConeShader, sphereObject, &_illuminationObjects);

    /* add light to the scene */
    _light = std::unique_ptr<LightData>(new LightData{LightData::Type::Infinite, Color3{1.f}, 1.f});
    _light->direction() = Vector3(0.3f, 1.f, 0.6f).normalized();

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
}

void GlobalIlluminationExample::drawEvent() {
    Range2Di viewport = defaultFramebuffer.viewport();
    Vector2i viewportSize = viewport.size();

    /* change viewport size */
    defaultFramebuffer.setViewport({{}, {_voxelDimensions, _voxelDimensions}});
    _camera->setViewport({_voxelDimensions, _voxelDimensions});

    /* set clear color to white */
    Renderer::setClearColor(Color4{1.f});
    /* clear color and depth */
    defaultFramebuffer.clear(FramebufferClear::Color | FramebufferClear::Depth);

    /* disable depth test and face culling */
    Renderer::setColorMask(false, false, false, false);
    Renderer::disable(Renderer::Feature::DepthTest);
    Renderer::disable(Renderer::Feature::FaceCulling);
    // Renderer::disable(Renderer::Feature::Blending);
    // // conservative rasterization?
    // Renderer::enable(Renderer::Feature(0x9346));

    /* clear voxelTexture */
    Containers::Array<Color4ub> tmp{Containers::DirectInit, Math::pow<3>(static_cast<UnsignedInt>(_voxelDimensions)), Color4ub(0, 0)};
    ImageView3D image{PixelFormat::RGBA, PixelType::UnsignedByte, Vector3i{_voxelDimensions}, tmp};
    _voxelTexture.setSubImage(0, {}, image);

    /* set voxel shader parameters */
    _voxelShader.setVoxelSize(_voxelGridWorldSize / static_cast<Float>(_voxelDimensions))
                .setVoxelWorldSize(_voxelGridWorldSize)
                .setProjectionMatrixX(_voxelProjectionMatX)
                .setProjectionMatrixY(_voxelProjectionMatY)
                .setProjectionMatrixZ(_voxelProjectionMatZ)
                .setVoxelDimensions(_voxelDimensions)
                .setVoxelTexture(_voxelTexture)
                .setConservativeRasterization(true)
                .setLight(*_light);

    /* voxelize scene */
    _camera->draw(_voxels);

    /* restore renderer/framebuffer */
    defaultFramebuffer.setViewport({{}, viewportSize});
    _camera->setViewport(viewportSize);

    Renderer::setColorMask(true, true, true, true);
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);
    // Renderer::enable(Renderer::Feature::Blending);
    // Renderer::setBlendFunction(Renderer::BlendFunction::SourceAlpha, Renderer::BlendFunction::OneMinusSourceAlpha);
    // Renderer::disable(Renderer::Feature(0x9346));

    /* set clear color to black */
    Renderer::setClearColor(Color4{Color3{0.f}, 1.f});
    /* clear color and depth */
    defaultFramebuffer.clear(FramebufferClear::Color | FramebufferClear::Depth);

    /* @todo: draw nice scenes */
    _voxelConeShader.setVoxelWorldProperties(_voxelGridWorldSize, _voxelDimensions)
                    .setCameraPosition(_cameraObject->transformationMatrix().translation())
                    .setVoxelTexture(_voxelTexture)
                    .setLight(*_light);
    _camera->draw(_illuminationObjects);

    // /* debug draw */
    // /* do not use it with more than 256x256x256 grids! */
    // /* set voxel visualization parameters */
    // Matrix4 modelMatrix = Matrix4::scaling(Vector3{_voxelGridWorldSize / static_cast<Float>(_voxelDimensions)});
	// Matrix4 viewMatrix = _camera->cameraMatrix();
	// Matrix4 transformationMatrix = _camera->projectionMatrix() * viewMatrix * modelMatrix;

    // _voxelVisualShader.setVoxelDimensions(_voxelDimensions)
    //                   .setTransformationMatrix(transformationMatrix)
    //                   .setVoxelTexture(_voxelTexture);
    // _debugVoxelsMesh.draw(_voxelVisualShader);

    swapBuffers();
}

void GlobalIlluminationExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    _camera->setViewport(size);
}

void GlobalIlluminationExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());
}

void GlobalIlluminationExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3();
}

void GlobalIlluminationExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    Float distance = _cameraObject->transformation().translation().z();

    /* Move 15% of the distance back or forward */
    distance *= 1 - (event.offset().y() > 0 ? 1/0.85f : 0.85f);
    _cameraObject->translate(Vector3::zAxis(distance));

    redraw();
}

Vector3 GlobalIlluminationExample::positionOnSphere(const Vector2i& position) const {
    Vector2 positionNormalized = Vector2(position*2)/Vector2(_camera->viewport()) - Vector2(1.0f);

    Float length = positionNormalized.length();
    Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
}

void GlobalIlluminationExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    Vector3 currentPosition = positionOnSphere(event.position());

    Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _cameraObject->rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());

    _previousPosition = currentPosition;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::GlobalIlluminationExample)
