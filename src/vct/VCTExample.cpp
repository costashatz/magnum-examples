/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
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
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/ImageFormat.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Timeline.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Shaders/Generic.h>
#include <Magnum/Shaders/Flat.h>

#include "ClearVoxelsShader.h"
#include "GeometryShader.h"
#include "InjectRadianceShader.h"
#include "MipMapBaseShader.h"
#include "MipMapVolumeShader.h"
#include "RenderTextureShader.h"
#include "VCTShader.h"
#include "VoxelizationShader.h"
#include "VoxelVisualizationShader.h"

using namespace Magnum::Math::Literals;

using Object3D = Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D>;
using Scene3D = Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation3D>;

namespace Magnum { namespace Examples {

class VoxelizedObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit VoxelizedObject(const Color4& color, GL::Mesh& mesh, VoxelizationShader& shader, Object3D& parent, SceneGraph::DrawableGroup3D& drawables): Object3D{&parent}, SceneGraph::Drawable3D{*this, &drawables}, _color{color}, _mesh(mesh), _voxelizationShader(shader) {}

    private:
        virtual void draw(const Matrix4&, SceneGraph::Camera3D&) {
            Matrix4 tr = absoluteTransformationMatrix();
            _voxelizationShader.setTransformationMatrix(tr)
                .setNormalMatrix(tr.normalMatrix())
                .setDiffuseColor(_color)
                .setEmissiveColor(Color3{0.f, 0.f, 0.f});
            // if(_color.r() == 1.f && _color.g() == 0.f)
            //     _voxelizationShader.setEmissiveColor(_color.rgb());
            _mesh.draw(_voxelizationShader);
        }

        Color4 _color;
        GL::Mesh& _mesh;
        VoxelizationShader& _voxelizationShader;
};

class GeometryObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit GeometryObject(const Color3& diffuseColor, GL::Mesh& mesh, GeometryShader& shader, Object3D& parent, SceneGraph::DrawableGroup3D& drawables): Object3D{&parent}, SceneGraph::Drawable3D{*this, &drawables}, _color{diffuseColor}, _mesh(mesh), _geometryShader(shader) {}

    private:
        virtual void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
            Matrix4 tr = transformationMatrix;
            _geometryShader.setTransformationMatrix(camera.projectionMatrix() * tr)
                .setNormalMatrix(absoluteTransformationMatrix().normalMatrix()) // We need normal in world frame
                .setDiffuseColor(_color)
                .setSpecularColor(Color3{0.f, 0.f, 0.f})
                .setShininess(0.f)
                .setEmissiveColor(Color3{0.f, 0.f, 0.f});
            // if(_color.r() == 1.f && _color.g() == 0.f)
            //     _geometryShader.setEmissiveColor(_color);
            _mesh.draw(_geometryShader);
        }

        Color3 _color;
        GL::Mesh& _mesh;
        GeometryShader& _geometryShader;
};

class VCTExample: public Platform::Application {
    public:
        explicit VCTExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void initTextures();
        void initGeometryBuffer();

        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2i& position) const;

        Scene3D _scene;
        Object3D* _cameraObject, *_manipulator;
        Vector3 _previousPosition;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _voxelized, _geometry;

        GL::Mesh _sphere, _cube, _floor, _debugVoxelsMesh;
        VoxelizationShader _voxelizationShader;
        VoxelVisualizationShader _voxelVisualizationShader;
        ClearVoxelsShader _clearVoxelsShader;
        InjectRadianceShader _injectRadianceShader;
        MipMapBaseShader _mipMapBaseShader;
        MipMapVolumeShader _mipMapVolumeShader;
        GeometryShader _geometryShader;
        RenderTextureShader _renderTextureShader;
        VCTShader _vctShader;
        Shaders::Flat3D _flatShader;
        Int _volumeDimension = 128;
        Float _volumeGridSize = 2.f;
        Float _voxelSize = _volumeGridSize / static_cast<Float>(_volumeDimension);
        Float _voxelScale = 1.f / _volumeGridSize;
        Vector3 _minPoint = {-_volumeGridSize / 2.f, -_volumeGridSize / 2.f, -_volumeGridSize / 2.f};
        Vector3 _maxPoint = {_volumeGridSize / 2.f, _volumeGridSize / 2.f, _volumeGridSize / 2.f};
        GL::Texture3D _albedoTexture, _normalTexture, _emissionTexture;
        GL::Texture3D _radianceTexture, _voxelTextures[6];
        GL::Framebuffer _geometrybuffer{Magnum::NoCreate};
        GL::Texture2D _geometryAlbedoTexture, _geometryNormalTexture, _geometrySpecularTexture, _geometryEmissionTexture, _geometryDepthTexture;
        Timeline _timeline;
        std::vector<Matrix4> _projectionMatrices, _projectionIMatrices;
};

VCTExample::VCTExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Voxel Cone Tracing Example")}
{
    initTextures();
    initGeometryBuffer();

    auto sphereData = Primitives::uvSphereSolid(16, 32);
    auto cubeData = Primitives::cubeSolid();
    auto floorData = Primitives::cubeSolid();
    MeshTools::transformVectorsInPlace(Matrix4::scaling({0.2f, 0.2f, 0.2f}), sphereData.positions(0));
    MeshTools::transformVectorsInPlace(Matrix4::scaling({0.2f, 0.2f, 0.2f}), cubeData.positions(0));
    MeshTools::transformVectorsInPlace(Matrix4::scaling({1.f, 0.01f, 1.f}), floorData.positions(0));

    _sphere = MeshTools::compile(sphereData); // MeshTools::compile(Primitives::icosphereSolid(4));
    _cube = MeshTools::compile(cubeData);
    _floor = MeshTools::compile(floorData);

    /* create debug mesh for drawing voxels */
    _debugVoxelsMesh.setPrimitive(GL::MeshPrimitive::Points)
                    .setCount(_volumeDimension * _volumeDimension * _volumeDimension);

    Float halfSize = _volumeGridSize / 2.f;
    // projection matrices
    Matrix4 projectionMatrix = Matrix4::orthographicProjection({_volumeGridSize, _volumeGridSize}, 0.f, _volumeGridSize);
    // view matrices
    _projectionMatrices.resize(3);
    _projectionIMatrices.resize(3);
    Vector3 center = {0.f, 0.f, 0.f};
    _projectionMatrices[0] = Matrix4::lookAt(center + Vector3(halfSize, 0.f, 0.f),
                                     center, Vector3(0.f, 1.f, 0.f)).invertedRigid();
    _projectionMatrices[1] = Matrix4::lookAt(center + Vector3(0.f, halfSize, 0.f),
                                     center, Vector3(0.f, 0.f, -1.f)).invertedRigid();
    _projectionMatrices[2] = Matrix4::lookAt(center + Vector3(0.f, 0.f, halfSize),
                                     center, Vector3(0.f, 1.f, 0.f)).invertedRigid();
    for(Int i = 0; i < 3; i++) {
        _projectionMatrices[i] = projectionMatrix * _projectionMatrices[i];
        // Utility::Debug{} << _projectionMatrices[i];
        _projectionIMatrices[i] = _projectionMatrices[i].inverted();
        // Utility::Debug{} << _projectionIMatrices[i];
    }

    Color4 red = {1.f, 0.f, 0.f, 1.f};
    Color4 green = {0.f, 1.f, 0.f, 1.f};
    Color4 yellow = {1.f, 1.f, 0.f, 1.f};

    _manipulator = new Object3D{&_scene};

    (new VoxelizedObject(green, _cube, _voxelizationShader, *_manipulator, _voxelized))->translate({-0.44f, 0.f, -0.3f}).rotateYLocal(25._degf); // .rotateYLocal(25._degf).rotateZLocal(-15._degf);
    (new GeometryObject(green.rgb(), _cube, _geometryShader, *_manipulator, _geometry))->translate({-0.44f, 0.f, -0.3f}).rotateYLocal(25._degf); // .rotateYLocal(25._degf).rotateZLocal(-15._degf);

    (new VoxelizedObject(red, _sphere, _voxelizationShader, *_manipulator, _voxelized))->translate({0.f, 0.f, 0.2f});
    (new GeometryObject(red.rgb(), _sphere, _geometryShader, *_manipulator, _geometry))->translate({0.f, 0.f, 0.2f});

    (new VoxelizedObject(yellow, _floor, _voxelizationShader, *_manipulator, _voxelized))->translate({0.f, -0.5f, 0.f});
    (new GeometryObject(yellow.rgb(), _floor, _geometryShader, *_manipulator, _geometry))->translate({0.f, -0.5f, 0.f});

    /* Configure camera */
    _cameraObject = new Object3D{&_scene};
    // _cameraObject->translate(Vector3::zAxis(4.f));
    _cameraObject->setTransformation(Matrix4::lookAt({2.f, 1.f, 1.f}, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}));
    _camera = new SceneGraph::Camera3D{*_cameraObject};
    _camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(45._degf, 4.f / 3.f, 0.2f, 50.f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);

    redraw();
    _timeline.start();
}

void VCTExample::drawEvent() {
    // Utility::Debug{} << _timeline.previousFrameDuration();

    Range2Di viewport = GL::defaultFramebuffer.viewport();
    Vector2i viewportSize = viewport.size();

    /* change viewport size */
    GL::defaultFramebuffer.setViewport({{}, {_volumeDimension, _volumeDimension}});
    _camera->setViewport({_volumeDimension, _volumeDimension});

    /* set clear color to white */
    GL::Renderer::setClearColor(Color4{1.f});
    /* clear color and depth */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    /* disable depth test and face culling */
    GL::Renderer::setColorMask(false, false, false, false);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);

    /* Clear voxel information */
    _clearVoxelsShader.bindAlbedoTexture(_albedoTexture)
        .bindNormalTexture(_normalTexture)
        .bindEmissionTexture(_emissionTexture)
        .bindRadianceTexture(_radianceTexture);
    UnsignedInt sz = std::ceil(_volumeDimension / 8.f);
    _clearVoxelsShader.dispatchCompute({sz, sz, sz});
    GL::Renderer::setMemoryBarrier(GL::Renderer::MemoryBarrier::ShaderImageAccess | GL::Renderer::MemoryBarrier::TextureFetch | GL::Renderer::MemoryBarrier::ShaderStorage);

    /* Voxelize scene */
    _voxelizationShader.setVolumeDimension(static_cast<UnsignedInt>(_volumeDimension))
        .setViewProjections(_projectionMatrices)
        .setViewProjectionsI(_projectionIMatrices)
        .setVoxelScale(_voxelScale)
        .setMinPoint(_minPoint)
        .bindAlbedoTexture(_albedoTexture)
        .bindNormalTexture(_normalTexture)
        .bindEmissionTexture(_emissionTexture);
    _camera->draw(_voxelized);

    GL::Renderer::setMemoryBarrier(GL::Renderer::MemoryBarrier::ShaderImageAccess | GL::Renderer::MemoryBarrier::TextureFetch | GL::Renderer::MemoryBarrier::ShaderStorage);

    /* Inject light information into voxelized texture */
    _injectRadianceShader.setVolumeDimension(_volumeDimension)
        .setVoxelScale(_voxelScale)
        .setVoxelSize(_voxelSize)
        .setMinPoint(_minPoint)
        .setTraceShadowHit(0.8f)
        .bindAlbedoTexture(_albedoTexture)
        .bindNormalTexture(_normalTexture)
        .bindEmissionTexture(_emissionTexture)
        .bindRadianceTexture(_radianceTexture)
        .setLight();
    sz = std::ceil(_volumeDimension / 8.f);
    _injectRadianceShader.dispatchCompute({sz, sz, sz});
    GL::Renderer::setMemoryBarrier(GL::Renderer::MemoryBarrier::ShaderImageAccess | GL::Renderer::MemoryBarrier::TextureFetch | GL::Renderer::MemoryBarrier::ShaderStorage);

    /* Generate MipMapBase */
    UnsignedInt mipDimension = _volumeDimension / 2;
    _mipMapBaseShader.setMipDimension(mipDimension)
        .bindVoxelTexture(_radianceTexture)
        .bindMipMapTextures(_voxelTextures);
    sz = std::ceil(mipDimension / 8.f);
    _mipMapBaseShader.dispatchCompute({sz, sz, sz});
    GL::Renderer::setMemoryBarrier(GL::Renderer::MemoryBarrier::ShaderImageAccess | GL::Renderer::MemoryBarrier::TextureFetch | GL::Renderer::MemoryBarrier::ShaderStorage);

    /* Generate MipMapVolume */
    mipDimension = _volumeDimension / 4;
    Int mipMapLevel = 0;
    while(mipDimension >= 1) {
        _mipMapVolumeShader.setMipDimension(mipDimension)
            .setMipLevel(mipMapLevel)
            .bindMipMapSourceTextures(_voxelTextures)
            .bindMipMapDestTextures(_voxelTextures, mipMapLevel + 1);
        sz = std::ceil(mipDimension / 8.f);
        _mipMapBaseShader.dispatchCompute({sz, sz, sz});
        GL::Renderer::setMemoryBarrier(GL::Renderer::MemoryBarrier::ShaderImageAccess | GL::Renderer::MemoryBarrier::TextureFetch | GL::Renderer::MemoryBarrier::ShaderStorage);
        mipMapLevel++;
        mipDimension /= 2;
    }

    /* Do Geometry pass */
    _geometrybuffer.bind();
    _geometrybuffer.setViewport({{}, viewportSize});
    _camera->setViewport(viewportSize);
    GL::Renderer::setColorMask(true, true, true, true);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* set clear color to black */
    GL::Renderer::setClearColor(Color4{0.f, 0.f, 0.f, 0.f});
    // GL::Renderer::setClearDepth(1.f);
    /* clear color and depth */
    _geometrybuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    _camera->draw(_geometry);

    /* restore renderer/framebuffer */
    GL::defaultFramebuffer.bind();
    GL::defaultFramebuffer.setViewport({{}, viewportSize});
    _camera->setViewport(viewportSize);

    GL::Renderer::setColorMask(true, true, true, true);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* set clear color to black */
    GL::Renderer::setClearColor(Color4{Color3{0.f}, 1.f});
    /* clear color and depth */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    // UnsignedInt drawMipLevel = 0;
    // UnsignedInt drawDir = 0;
    // UnsignedInt vDimension = static_cast<UnsignedInt>(_volumeDimension / pow(2.f, drawMipLevel));

    // Matrix4 modelMatrix = Matrix4::translation(_minPoint) * Matrix4::scaling(Vector3{_volumeGridSize / vDimension});
    // Matrix4 viewMatrix = _camera->cameraMatrix();
    // Matrix4 transformationMatrix = _camera->projectionMatrix() * viewMatrix * modelMatrix;

    // // Utility::Debug{} << transformationMatrix;

    // _voxelVisualizationShader.setVolumeDimension(static_cast<UnsignedInt>(_volumeDimension))
    //     .setTransformationMatrix(transformationMatrix);
    // if(drawMipLevel == 0)
    //     _voxelVisualizationShader.bindVoxelTexture(_radianceTexture);
    // else
    //     _voxelVisualizationShader.bindVoxelTexture(_voxelTextures[drawDir], drawMipLevel - 1);

    // _debugVoxelsMesh.draw(_voxelVisualizationShader);

    // GL::Renderer::setMemoryBarrier(GL::Renderer::MemoryBarrier::ShaderImageAccess | GL::Renderer::MemoryBarrier::TextureFetch | GL::Renderer::MemoryBarrier::ShaderStorage);

    // _renderTextureShader.bindOutputTexture(_geometryDepthTexture);
    // _renderTextureShader.setDepth(1);
    // // _renderTextureShader.bindOutputTexture(_geometryNormalTexture);
    // GL::Mesh mesh;
    // mesh.setCount(3)
    //     .draw(_renderTextureShader);

    Matrix4 trMat = _camera->cameraMatrix().inverted() * _camera->projectionMatrix().inverted();
    Vector3 cameraPosition = _cameraObject->absoluteTransformationMatrix().translation();
    _vctShader.setInverseProjectionViewMatrix(trMat)
        .setCameraPosition(cameraPosition)
        .setVoxelScale(_voxelScale)
        .setWorldMinPoint(_minPoint)
        .setWorldMaxPoint(_maxPoint)
        .setVolumeDimension(static_cast<UnsignedInt>(_volumeDimension))
        .setAOAlpha(0.f)
        .setAOFalloff(800.f)
        .setMaxTracingDistance(0.95f)
        .setSamplingFactor(0.1f)
        .setBounceStrength(0.2f)
        .setConeShadowTolerance(0.2f)
        .setConeShadowAperture(0.2f)
        .bindAlbedoTexture(_geometryAlbedoTexture)
        .bindNormalTexture(_geometryNormalTexture)
        .bindSpecularTexture(_geometrySpecularTexture)
        .bindEmissionTexture(_geometryEmissionTexture)
        .bindDepthTexture(_geometryDepthTexture)
        .bindVoxelVisibility(_normalTexture)
        .bindVoxelTexture(_radianceTexture)
        .bindMipMapTextures(_voxelTextures)
        .setLight();
    GL::Mesh mesh;
    mesh.setCount(3)
        .draw(_vctShader);

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}

void VCTExample::initTextures() {
    _albedoTexture.setMagnificationFilter(GL::SamplerFilter::Linear)
                .setMinificationFilter(GL::SamplerFilter::Linear)
                .setWrapping(GL::SamplerWrapping::ClampToEdge)
                .setStorage(1, GL::TextureFormat::RGBA8, {_volumeDimension, _volumeDimension, _volumeDimension});

    _normalTexture.setMagnificationFilter(GL::SamplerFilter::Linear)
                .setMinificationFilter(GL::SamplerFilter::Linear)
                .setWrapping(GL::SamplerWrapping::ClampToEdge)
                .setStorage(1, GL::TextureFormat::RGBA8, {_volumeDimension, _volumeDimension, _volumeDimension});

    _emissionTexture.setMagnificationFilter(GL::SamplerFilter::Linear)
                .setMinificationFilter(GL::SamplerFilter::Linear)
                .setWrapping(GL::SamplerWrapping::ClampToEdge)
                .setStorage(1, GL::TextureFormat::RGBA8, {_volumeDimension, _volumeDimension, _volumeDimension});

    _radianceTexture.setMagnificationFilter(GL::SamplerFilter::Linear)
                .setMinificationFilter(GL::SamplerFilter::Linear)
                .setWrapping(GL::SamplerWrapping::ClampToEdge)
                .setStorage(1, GL::TextureFormat::RGBA8, {_volumeDimension, _volumeDimension, _volumeDimension});

    Int mipSize = _volumeDimension / 2;
    Containers::Array<char> data(Containers::ValueInit, mipSize * mipSize * mipSize * 4);
    Image3D zeroImage{PixelFormat::RGBA8UI, {mipSize, mipSize, mipSize}, std::move(data)};
    for(UnsignedInt i = 0; i < 6; i++) {
        _voxelTextures[i].setMagnificationFilter(GL::SamplerFilter::Linear)
                    .setMinificationFilter(GL::SamplerFilter::Linear)
                    .setWrapping(GL::SamplerWrapping::ClampToEdge)
                    .setStorage(Math::log2(mipSize) + 1, GL::TextureFormat::RGBA8, {mipSize, mipSize, mipSize})
                    .setSubImage(0, {}, zeroImage)
                    .generateMipmap();
    }
}

void VCTExample::initGeometryBuffer() {
    Range2Di viewport = GL::defaultFramebuffer.viewport();
    Vector2i viewportSize = viewport.size();

    _geometrybuffer = GL::Framebuffer(viewport);
    _geometryAlbedoTexture.setMagnificationFilter(GL::SamplerFilter::Nearest)
                    .setMinificationFilter(GL::SamplerFilter::Nearest)
                    .setStorage(1, GL::TextureFormat::RGB8, viewportSize);
    _geometryNormalTexture.setMagnificationFilter(GL::SamplerFilter::Nearest)
                    .setMinificationFilter(GL::SamplerFilter::Nearest)
                    .setStorage(1, GL::TextureFormat::RGBA16F, viewportSize);
    _geometrySpecularTexture.setMagnificationFilter(GL::SamplerFilter::Nearest)
                    .setMinificationFilter(GL::SamplerFilter::Nearest)
                    .setStorage(1, GL::TextureFormat::RGBA8, viewportSize);
    _geometryEmissionTexture.setMagnificationFilter(GL::SamplerFilter::Nearest)
                    .setMinificationFilter(GL::SamplerFilter::Nearest)
                    .setStorage(1, GL::TextureFormat::RGB8, viewportSize);
    _geometryDepthTexture.setMagnificationFilter(GL::SamplerFilter::Nearest)
                    .setMinificationFilter(GL::SamplerFilter::Nearest)
                    .setStorage(1, GL::TextureFormat::DepthComponent24, viewportSize);

    /* Color information*/
    _geometrybuffer.attachTexture(
        GL::Framebuffer::ColorAttachment(0), _geometryAlbedoTexture, 0);
    /* Normal information */
    _geometrybuffer.attachTexture(
        GL::Framebuffer::ColorAttachment(1), _geometryNormalTexture, 0);
    /* Specular and shininess information */
    _geometrybuffer.attachTexture(
        GL::Framebuffer::ColorAttachment(2), _geometrySpecularTexture, 0);
    /* Emission information */
    _geometrybuffer.attachTexture(
        GL::Framebuffer::ColorAttachment(3), _geometryEmissionTexture, 0);
    /* Depth information */
    _geometrybuffer.attachTexture(
        GL::Framebuffer::BufferAttachment::Depth, _geometryDepthTexture, 0);

    _geometrybuffer.mapForDraw({{0, GL::Framebuffer::ColorAttachment(0)},
                            {1, GL::Framebuffer::ColorAttachment(1)},
                            {2, GL::Framebuffer::ColorAttachment(2)},
                            {3, GL::Framebuffer::ColorAttachment(3)}});
}

void VCTExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());
}

void VCTExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3();
}

void VCTExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    const Float distance = _cameraObject->transformation().translation().z();

    /* Move 15% of the distance back or forward */
    _cameraObject->translate(Vector3::zAxis(
        distance*(1.0f - (event.offset().y() > 0 ? 1/0.85f : 0.85f))));

    redraw();
}

Vector3 VCTExample::positionOnSphere(const Vector2i& position) const {
    const Vector2 positionNormalized = Vector2{position}/Vector2{_camera->viewport()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result*Vector3::yScale(-1.0f)).normalized();
}

void VCTExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector3 currentPosition = positionOnSphere(event.position());
    const Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _cameraObject->rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());
    _previousPosition = currentPosition;

    redraw();
}
}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::VCTExample)
