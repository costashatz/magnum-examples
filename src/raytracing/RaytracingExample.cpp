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

#include <Corrade/Utility/Resource.h>

#include <Magnum/Buffer.h>
#include <Magnum/Context.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Extensions.h>
#include <Magnum/Image.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Renderer.h>
#include <Magnum/Shader.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Timeline.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Trade/MeshData3D.h>

#include "RaytracingShader.h"

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class RenderTextureShader: public AbstractShaderProgram {
    public:

        explicit RenderTextureShader() {
            Utility::Resource rs("RaytracingShaders");

            Shader vert{Version::GL430, Shader::Type::Vertex};
            Shader frag{Version::GL430, Shader::Type::Fragment};

            vert.addSource(rs.get("RenderTextureShader.vert"));
            frag.addSource(rs.get("RenderTextureShader.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());
        }

        explicit RenderTextureShader(NoCreateT) noexcept: AbstractShaderProgram{NoCreate} {}

        RenderTextureShader& setOutputTexture(Texture2D& texture) {
            texture.bind(_outputTextureBindLocation);
            return *this;
        }

    private:
        Int _outputTextureBindLocation{0};
};

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
        void refreshTexture(const Vector2i& size);
        void refreshCamera();

        Scene3D _scene;
        Object3D *_o, *_cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;
        Timeline _timeline;

        RenderTextureShader _renderShader;
        RaytracingShader _rayShader;
        UnsignedInt _width, _height;
        UnsignedInt _workgroupSize = 20;
        Buffer _objectsBuffer;
        Buffer _materialsBuffer;
        Buffer _lightsBuffer;
        Buffer _meshesBuffer;
        Texture2D _outputImage;
};

class RayObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit RayObject(Buffer& meshesBuffer, UnsignedInt offset, Object3D* parent, SceneGraph::DrawableGroup3D* group) : Object3D{parent}, SceneGraph::Drawable3D{*this, group}, _meshesBuffer{meshesBuffer}, _offset(offset) {}

    private:
        void draw(const Matrix4& /* transformationMatrix */, SceneGraph::Camera3D& /* camera */) override {
            UnsignedInt transformationOffset = 32;
            _meshesBuffer.setSubData(_offset + transformationOffset, std::vector<Matrix4>(1, absoluteTransformation().invertedRigid()));
        }

        Buffer& _meshesBuffer;
        UnsignedInt _offset;
};

RaytracingExample::RaytracingExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Raycasting Example")}
{
    /* Every scene needs a camera */
    (*(_cameraObject = new Object3D{&_scene}));
    (*(_camera = new SceneGraph::Camera3D{*_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 100.0f))
        .setViewport(defaultFramebuffer.viewport().size());

    /* Default object, parent of all (for manipulation) */
    _o = new Object3D{&_scene};

    /* create shader */
    _rayShader = RaytracingShader{};

    /* initialize and bind output image */
    refreshTexture(defaultFramebuffer.viewport().size());

    /* initialize and setup camera in shader */
    /* camera located at (2,3,10) looking in the center */
    _cameraObject->setTransformation(Matrix4::lookAt({2.f, 3.f, 10.f}, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}));
    refreshCamera();

    /* bind meshes buffer */
    _meshesBuffer.bind(Buffer::Target::ShaderStorage, _rayShader.meshBufferBindLocation());

    std::vector<RayMesh> meshes;

    /* setup materials */
    Material material;
    material.diffuse = Color4(0.3f, 0.8f, 0.f, 1.f);
    material.specular = Color4(1.f);
    material.emission = Color4(0.f, 0.f, 0.f, 1.f);
    material.shininess = 80.f;

    std::vector<Material> mats;
    mats.emplace_back(material);

    material.diffuse = Color4(0.f, 0.3f, 0.8f, 1.f);
    material.specular = Color4(1.f);
    material.emission = Color4(0.f, 0.f, 0.f, 1.f);
    material.shininess = 80.f;
    mats.emplace_back(material);

    /* bind materials buffer */
    _materialsBuffer.bind(Buffer::Target::ShaderStorage, _rayShader.materialBufferBindLocation());
    _materialsBuffer.setData(mats, BufferUsage::DynamicCopy);

    /* setup lights */
    Light light;
    light.position = Vector4(-5.f, 3.f, 3.f, 1.f); // w = 1.f means that it's not directional
    light.ambient = Color4(0.f, 0.f, 0.f, 1.f);
    light.diffuse = Color4(1.f);
    light.specular = Color4(1.f);
    light.spotDirection.xyz() = Vector3(1.f, -0.5f, -0.2f).normalized();
    light.spotExponent = 1.f;
    light.spotCutoff = Magnum::Math::Constants<Magnum::Float>::piHalf();
    light.intensity = 30.f;
    light.constantAttenuation = 0.f;
    light.linearAttenuation = 0.f;
    light.quadraticAttenuation = 1.f;

    std::vector<Light> lights;
    lights.emplace_back(light);

    light.position = Vector4(0.6f, 0.6f, 0.6f, 0.f).normalized(); // w = 0.f means directional
    light.intensity = 0.5f;
    lights.emplace_back(light);

    /* bind lights buffer */
    _lightsBuffer.bind(Buffer::Target::ShaderStorage, _rayShader.lightBufferBindLocation());
    _lightsBuffer.setData(lights, BufferUsage::DynamicCopy);

    /* create cube and set object buffer */
    Trade::MeshData3D cube = Primitives::Cube::solid();

    std::vector<Object> triangle_objects;
    UnsignedInt startIndex = 0;

    Float minX = Constants::inf();
    Float minY = Constants::inf();
    Float minZ = Constants::inf();
    Float maxX = -Constants::inf();
    Float maxY = -Constants::inf();
    Float maxZ = -Constants::inf();

    for(UnsignedInt i = 0; i < cube.indices().size(); i += 3) {
        Object obj;
        obj.triangle.A.xyz() = cube.positions(0)[cube.indices()[i]];
        obj.triangle.B.xyz() = cube.positions(0)[cube.indices()[i + 1]];
        obj.triangle.C.xyz() = cube.positions(0)[cube.indices()[i + 2]];

        obj.triangle.normal.xyz() = (cross((obj.triangle.B - obj.triangle.A).xyz(), (obj.triangle.C - obj.triangle.A).xyz())).normalized();

        obj.meshId = 0;
        obj.materialIndex = 0;

        triangle_objects.emplace_back(obj);

        /* bounding box computation */
        for(UnsignedInt j = 0; j < 3; j++) {
            Vector3 vec =  cube.positions(0)[cube.indices()[i + j]];
            if(vec.x() < minX)
                minX = vec.x();
            if(vec.x() > maxX)
                maxX = vec.x();
            if(vec.y() < minY)
                minY = vec.y();
            if(vec.y() > maxY)
                maxY = vec.y();
            if(vec.z() < minZ)
                minZ = vec.z();
            if(vec.z() > maxZ)
                maxZ = vec.z();
        }
    }
    RayMesh mBox;
    mBox.minPoint = Vector4(minX, minY, minZ, 1.f);
    mBox.maxPoint = Vector4(maxX, maxY, maxZ, 1.f);
    mBox.objectStart = startIndex;
    mBox.objectSize = triangle_objects.size() - startIndex;
    startIndex = triangle_objects.size();
    meshes.push_back(mBox);
    auto boxObj = new RayObject{_meshesBuffer, 0, _o, &_drawables};
    boxObj->rotateY(Rad(Magnum::Math::Constants<Magnum::Float>::piHalf() / 2.f));

    minX = Constants::inf();
    minY = Constants::inf();
    minZ = Constants::inf();
    maxX = -Constants::inf();
    maxY = -Constants::inf();
    maxZ = -Constants::inf();
    cube = Primitives::Cube::solid();
    MeshTools::transformPointsInPlace(Matrix4::scaling(Vector3{20.f, 0.1f, 20.f}), cube.positions(0));
    // MeshTools::transformPointsInPlace(Matrix4::translation(Vector3{0.f, -1.05f, 0.f}), cube.positions(0));
    /* add ground */
    for(UnsignedInt i = 0; i < cube.indices().size(); i += 3) {
        Object obj;
        obj.triangle.A.xyz() = cube.positions(0)[cube.indices()[i]];
        obj.triangle.B.xyz() = cube.positions(0)[cube.indices()[i + 1]];
        obj.triangle.C.xyz() = cube.positions(0)[cube.indices()[i + 2]];

        obj.triangle.normal.xyz() = (cross((obj.triangle.B - obj.triangle.A).xyz(), (obj.triangle.C - obj.triangle.A).xyz())).normalized();

        obj.meshId = 1;
        obj.materialIndex = 1;

        triangle_objects.emplace_back(obj);

        /* bounding box computation */
        for(UnsignedInt j = 0; j < 3; j++) {
            Vector3 vec =  cube.positions(0)[cube.indices()[i + j]];
            if(vec.x() < minX)
                minX = vec.x();
            if(vec.x() > maxX)
                maxX = vec.x();
            if(vec.y() < minY)
                minY = vec.y();
            if(vec.y() > maxY)
                maxY = vec.y();
            if(vec.z() < minZ)
                minZ = vec.z();
            if(vec.z() > maxZ)
                maxZ = vec.z();
        }
    }
    RayMesh mGround;
    mGround.minPoint = Vector4(minX, minY, minZ, 1.f);
    mGround.maxPoint = Vector4(maxX, maxY, maxZ, 1.f);
    mGround.objectStart = startIndex;
    mGround.objectSize = triangle_objects.size() - startIndex;
    startIndex = triangle_objects.size();
    meshes.push_back(mGround);
    auto groundObj = new RayObject{_meshesBuffer, sizeof(RayMesh), _o, &_drawables};
    groundObj->translate({0.f, -1.05f, 0.f});

    Trade::MeshData3D sphere = Primitives::Icosphere::solid(2);
    // MeshTools::transformPointsInPlace(Matrix4::translation(Vector3{4.f, 1.f, 1.f}), sphere.positions(0));
    /* add a sphere */
    minX = Constants::inf();
    minY = Constants::inf();
    minZ = Constants::inf();
    maxX = -Constants::inf();
    maxY = -Constants::inf();
    maxZ = -Constants::inf();
    for(UnsignedInt i = 0; i < sphere.indices().size(); i += 3) {
        Object obj;
        obj.triangle.A.xyz() = sphere.positions(0)[sphere.indices()[i]];
        obj.triangle.B.xyz() = sphere.positions(0)[sphere.indices()[i + 1]];
        obj.triangle.C.xyz() = sphere.positions(0)[sphere.indices()[i + 2]];

        obj.triangle.normal.xyz() = (cross((obj.triangle.B - obj.triangle.A).xyz(), (obj.triangle.C - obj.triangle.A).xyz())).normalized();

        obj.meshId = 2;
        obj.materialIndex = 0;

        triangle_objects.emplace_back(obj);

        /* bounding box computation */
        for(UnsignedInt j = 0; j < 3; j++) {
            Vector3 vec =  sphere.positions(0)[sphere.indices()[i + j]];
            if(vec.x() < minX)
                minX = vec.x();
            if(vec.x() > maxX)
                maxX = vec.x();
            if(vec.y() < minY)
                minY = vec.y();
            if(vec.y() > maxY)
                maxY = vec.y();
            if(vec.z() < minZ)
                minZ = vec.z();
            if(vec.z() > maxZ)
                maxZ = vec.z();
        }
    }
    RayMesh mSphere;
    mSphere.minPoint = Vector4(minX, minY, minZ, 1.f);
    mSphere.maxPoint = Vector4(maxX, maxY, maxZ, 1.f);
    mSphere.objectStart = startIndex;
    mSphere.objectSize = triangle_objects.size() - startIndex;
    startIndex = triangle_objects.size();
    meshes.push_back(mSphere);
    auto sphereObj = new RayObject{_meshesBuffer, 2 * sizeof(RayMesh), _o, &_drawables};
    sphereObj->translate({4.f, 1.f, 1.f});

    // Trade::MeshData3D sphere2 = Primitives::Icosphere::solid(2);
    // // Debug{} << sphere.indices().size();
    // // MeshTools::transformPointsInPlace(Matrix4::translation(Vector3{-4.f, 1.f, 1.f}), sphere2.positions(0));
    // /* add another sphere */
    // minX = Constants::inf();
    // minY = Constants::inf();
    // minZ = Constants::inf();
    // maxX = -Constants::inf();
    // maxY = -Constants::inf();
    // maxZ = -Constants::inf();
    // for(UnsignedInt i = 0; i < sphere2.indices().size(); i += 3) {
    //     Object obj;
    //     obj.triangle.A.xyz() = sphere2.positions(0)[sphere2.indices()[i]];
    //     obj.triangle.B.xyz() = sphere2.positions(0)[sphere2.indices()[i + 1]];
    //     obj.triangle.C.xyz() = sphere2.positions(0)[sphere2.indices()[i + 2]];

    //     obj.triangle.normal.xyz() = (cross((obj.triangle.B - obj.triangle.A).xyz(), (obj.triangle.C - obj.triangle.A).xyz())).normalized();

    //     obj.meshId = 3;
    //     obj.materialIndex = 0;

    //     triangle_objects.emplace_back(obj);

    //     /* bounding box computation */
    //     for(UnsignedInt j = 0; j < 3; j++) {
    //         Vector3 vec =  sphere2.positions(0)[sphere2.indices()[i + j]];
    //         if(vec.x() < minX)
    //             minX = vec.x();
    //         if(vec.x() > maxX)
    //             maxX = vec.x();
    //         if(vec.y() < minY)
    //             minY = vec.y();
    //         if(vec.y() > maxY)
    //             maxY = vec.y();
    //         if(vec.z() < minZ)
    //             minZ = vec.z();
    //         if(vec.z() > maxZ)
    //             maxZ = vec.z();
    //     }
    // }
    // RayMesh mSphere2;
    // mSphere2.minPoint = Vector4(minX, minY, minZ, 1.f);
    // mSphere2.maxPoint = Vector4(maxX, maxY, maxZ, 1.f);
    // mSphere2.objectStart = startIndex;
    // mSphere2.objectSize = triangle_objects.size() - startIndex;
    // startIndex = triangle_objects.size();
    // meshes.push_back(mSphere2);
    // auto sphereObj2 = new RayObject{_meshesBuffer, 3 * sizeof(RayMesh), _o, &_drawables};
    // sphereObj2->translate({-2.f, 1.2f, -3.f});

    Debug{} << "Number of triangles:" << triangle_objects.size();

    /* bind objects buffer */
    _objectsBuffer.bind(Buffer::Target::ShaderStorage, _rayShader.objectBufferBindLocation());
    _objectsBuffer.setData(triangle_objects, BufferUsage::DynamicCopy);

    /* set meshes buffer */
    _meshesBuffer.setData(meshes, BufferUsage::DynamicCopy);

    /* set scene params */
    UnsignedInt reflectionDepth = 2;
    _rayShader.setSceneParams(triangle_objects.size(), lights.size(), meshes.size(), reflectionDepth);
    _rayShader.setReflectionDecay(0.8f);

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();

    redraw();
}

void RaytracingExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);
    // Debug{} << _timeline.previousFrameDuration() * 1e3f;
    /* refresh camera */
    refreshCamera();
    _camera->draw(_drawables);
    Renderer::setMemoryBarrier(Renderer::MemoryBarrier::TextureUpdate | Renderer::MemoryBarrier::ShaderStorage | Renderer::MemoryBarrier::BufferUpdate);
    /* zero image and re-bind */
    refreshTexture(defaultFramebuffer.viewport().size());
    /* perform raycasting */
    _rayShader.dispatchCompute({_width / _workgroupSize, _height / _workgroupSize, 1});
    Renderer::setMemoryBarrier(Renderer::MemoryBarrier::TextureUpdate | Renderer::MemoryBarrier::ShaderStorage | Renderer::MemoryBarrier::BufferUpdate);

    _renderShader.setOutputTexture(_outputImage);
    Mesh mesh;
    mesh.setCount(3)
        .draw(_renderShader);
    swapBuffers();
    redraw();
    _timeline.nextFrame();
}

void RaytracingExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    _camera->setViewport(size);
    _rayShader.setViewport(size[0], size[1]);
    refreshTexture(size);
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

void RaytracingExample::refreshTexture(const Vector2i& size) {
    _width = size[0];
    _height = size[1];

    Image2D image{PixelFormat::RGBA, PixelType::Float, size, Containers::Array<char>(size[0]*size[1]*16)};
    _outputImage.setWrapping(Sampler::Wrapping::ClampToEdge)
                .setMinificationFilter(Sampler::Filter::Nearest)
                .setMagnificationFilter(Sampler::Filter::Nearest)
                .setImage(0, TextureFormat::RGBA, image);

    _rayShader.setOutputTexture(_outputImage);
    _rayShader.setViewport(_width, _height);
}

void RaytracingExample::refreshCamera() {
    Matrix4 cameraTransformation = _cameraObject->transformationMatrix();
    Camera camera;
    camera.pos = cameraTransformation.translation();
    camera.xAxis = -cameraTransformation.right().normalized();
    camera.yAxis = -cameraTransformation.up().normalized();
    camera.dir = -cameraTransformation.backward().normalized();
    camera.tanFovY = std::tan(35.f * float(M_PI) / 180.f / 2.f);
    camera.tanFovX = (static_cast<float>(_width) * camera.tanFovY) / static_cast<float>(_height);

    _rayShader.setCamera(camera);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::RaytracingExample)
