/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>
        2013 — Jan Dupal <dupal.j@gmail.com>

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

#include <ctime>
#include <vector>

// #include <dart/collision/dart/DARTCollisionDetector.hpp>
#include <dart/dart.hpp>
#include <dart/dynamics/Skeleton.hpp>

#include <Magnum/DartIntegration/ConvertShapeNode.h>
#include <Magnum/DartIntegration/World.h>

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Mesh.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Renderer.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.hpp>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Timeline.h>

#include <configure.h>


#if DART_MAJOR_VERSION == 6
    #include <dart/utils/urdf/urdf.hpp>
    #define DartLoader dart::utils::DartLoader
#else
    #include <dart/io/urdf/urdf.hpp>
    #define DartLoader dart::io::DartLoader
#endif

const double default_domino_height = 0.3;
const double default_domino_width = 0.4 * default_domino_height;
const double default_domino_depth = default_domino_width / 5.0;

const double default_domino_density = 2.6e3; // kg/m^3
const double default_domino_mass =
    default_domino_density
    * default_domino_height
    * default_domino_width
    * default_domino_depth;

inline dart::dynamics::SkeletonPtr createDomino()
{
    /* Create a Skeleton with the name "domino" */
    dart::dynamics::SkeletonPtr domino = dart::dynamics::Skeleton::create("domino");

    /* Create a body for the domino */
    dart::dynamics::BodyNodePtr body =
        domino->createJointAndBodyNodePair<dart::dynamics::FreeJoint>(nullptr).second;

    /* Create a shape for the domino */
    std::shared_ptr<dart::dynamics::BoxShape> box(
        new dart::dynamics::BoxShape(Eigen::Vector3d(default_domino_depth,
                                        default_domino_width,
                                        default_domino_height)));
    auto shapeNode = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);
    shapeNode->getVisualAspect()->setColor(Eigen::Vector3d(0.6, 0.6, 0.6));

    /* Set up inertia for the domino */
    dart::dynamics::Inertia inertia;
    inertia.setMass(default_domino_mass);
    inertia.setMoment(box->computeInertia(default_domino_mass));
    body->setInertia(inertia);

    domino->getDof("Joint_pos_z")->setPosition(default_domino_height / 2.0);

    return domino;
}

/* Add a soft body with the specified Joint type to a chain */
template<class JointType>
dart::dynamics::BodyNode* addSoftBody(const dart::dynamics::SkeletonPtr& chain, const std::string& name, dart::dynamics::BodyNode* parent = nullptr)
{
    constexpr double default_shape_density = 1000; // kg/m^3
    constexpr double default_shape_height  = 0.2;  // m
    constexpr double default_shape_width   = 0.03; // m
    constexpr double default_skin_thickness = 1e-2; // m

    constexpr double default_vertex_stiffness = 1000.0;
    constexpr double default_edge_stiffness = 1.0;
    constexpr double default_soft_damping = 5.0;

    /* Set the Joint properties */
    typename JointType::Properties joint_properties;
    joint_properties.mName = name+"_joint";
    if(parent)
    {
        /* If the body has a parent, we should position the joint to be in the
         * middle of the centers of the two bodies */
        Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
        tf.translation() = Eigen::Vector3d(0, 0, default_shape_height / 2.0);
        joint_properties.mT_ParentBodyToJoint = tf;
        joint_properties.mT_ChildBodyToJoint = tf.inverse();
    }

    /* Set the properties of the soft body */
    dart::dynamics::SoftBodyNode::UniqueProperties soft_properties;
    /* Make a wide and short box */
    double width = default_shape_height, height = 2*default_shape_width;
    Eigen::Vector3d dims(width, width, height);

    double mass = 2*dims[0]*dims[1] + 2*dims[0]*dims[2] + 2*dims[1]*dims[2];
    mass *= default_shape_density * default_skin_thickness;
    soft_properties = dart::dynamics::SoftBodyNodeHelper::makeBoxProperties(dims, Eigen::Isometry3d::Identity(), Eigen::Vector3i(4,4,4), mass);

    soft_properties.mKv = default_vertex_stiffness;
    soft_properties.mKe = default_edge_stiffness;
    soft_properties.mDampCoeff = default_soft_damping;

    /* Create the Joint and Body pair */
    dart::dynamics::SoftBodyNode::Properties body_properties(dart::dynamics::BodyNode::AspectProperties(name), soft_properties);
    dart::dynamics::SoftBodyNode* bn = chain->createJointAndBodyNodePair<JointType, dart::dynamics::SoftBodyNode>(parent, joint_properties, body_properties).second;

    /* Zero out the inertia for the underlying BodyNode */
    // dart::dynamics::Inertia inertia;
    // inertia.setMoment(1e-8*Eigen::Matrix3d::Identity());
    // inertia.setMass(1e-8);
    // bn->setInertia(inertia);

    /* Add a rigid collision geometry and inertia */
    Eigen::Vector3d dims_body(default_shape_height, default_shape_height, 2*default_shape_width);
    dims_body *= 0.6;
    std::shared_ptr<dart::dynamics::BoxShape> box = std::make_shared<dart::dynamics::BoxShape>(dims_body);
    bn->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);

    dart::dynamics::Inertia inertia;
    inertia.setMass(default_shape_density * box->getVolume());
    inertia.setMoment(box->computeInertia(inertia.getMass()));
    bn->setInertia(inertia);

    return bn;
}

namespace Magnum { namespace Examples {

typedef ResourceManager<Buffer, Mesh, Shaders::Phong> ViewerResourceManager;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

struct MaterialData{
    Vector3 _ambientColor,
            _diffuseColor,
            _specularColor;
    Float _shininess;
};

class ColoredObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit ColoredObject(Mesh* mesh, const MaterialData& material, Object3D* parent, SceneGraph::DrawableGroup3D* group);

        ColoredObject& setMesh(Mesh* mesh);
        ColoredObject& setMaterial(const MaterialData& material);
        ColoredObject& setSoftBody(bool softBody = true);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Mesh* _mesh;
        Resource<Shaders::Phong> _shader;
        MaterialData _material;
        bool _isSoftBody = false;
};


class DartExample: public Platform::Application {
    public:
        explicit DartExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        void updateGraphics();
        void updateManipulator(dart::dynamics::SkeletonPtr skel);
        dart::dynamics::SkeletonPtr createNewDomino(Eigen::Vector6d position, double angle, double& totalAngle);

        ViewerResourceManager _resourceManager;

        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        SceneGraph::Camera3D* _camera;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject;

        /* DART */
        std::shared_ptr<DartIntegration::World> _dartWorld;
        std::unordered_map<std::shared_ptr<DartIntegration::Object>, ColoredObject*> _coloredObjects;
        std::vector<Object3D*> _dartObjs;
        size_t _dominoId;
        dart::dynamics::SkeletonPtr _manipulator, _dominoSkel;

        /* DART control */
        Eigen::VectorXd _desiredPositions;
        const double _pGain = 200.0;
        const double _dGain = 20.0;
};

DartExample::DartExample(const Arguments& arguments): Platform::Application(arguments, NoCreate) {
    /* Try 16x MSAA */
    Configuration conf;
    conf.setTitle("Magnum Dart Integration Example")
        .setSampleCount(8);
    if(!tryCreateContext(conf))
        createContext(conf.setSampleCount(0));

    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    /* Camera setup */
    (_cameraRig = new Object3D(&_scene));
        // ->translate({0.f, 4.f, 0.f});
    (_cameraObject = new Object3D(_cameraRig));
        // ->translate({0.f, 0.f, 20.f});
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(35.0f), 1.0f, 0.001f, 100.0f))
        .setViewport(defaultFramebuffer.viewport().size());
    // _cameraObject->setTransformation(Magnum::Matrix4::lookAt(Vector3{0.f, 15.f, 2.f}, Vector3{0.f, 0.f, 0.f}, Vector3{0.f, 0.f, 1.f}));
    _cameraObject->setTransformation(Magnum::Matrix4::lookAt(Vector3{0.f, 3.f, 1.f}, Vector3{0.f, 0.f, 0.5f}, Vector3{0.f, 0.f, 1.f}));

    /* DART: Load Skeleton */
    DartLoader loader;
    std::string filename = std::string(DARTEXAMPLE_DIR) + "/urdf/test.urdf";
    _manipulator = loader.parseSkeleton(filename);
    for(size_t i = 0; i < _manipulator->getNumJoints(); i++)
        _manipulator->getJoint(i)->setPositionLimitEnforced(true);
    _manipulator->enableSelfCollisionCheck();

    /* Position its base in a reasonable way */
    Eigen::Isometry3d tf2 = Eigen::Isometry3d::Identity();
    tf2.translation() = Eigen::Vector3d(-0.65, 0.0, 0.0);
    _manipulator->getJoint(0)->setTransformFromParentBodyNode(tf2);

    /* Get it into a useful configuration */
    _manipulator->getDof(1)->setPosition(140.0 * M_PI / 180.0);
    _manipulator->getDof(2)->setPosition(-140.0 * M_PI / 180.0);

    _desiredPositions = _manipulator->getPositions();

    /* Create a floor */
    dart::dynamics::SkeletonPtr floor = dart::dynamics::Skeleton::create("floor");
    /* Give the floor a body */
    dart::dynamics::BodyNodePtr body =
        floor->createJointAndBodyNodePair<dart::dynamics::WeldJoint>(nullptr).second;

    /* Give the body a shape */
    double floor_width = 10.0;
    double floor_height = 0.02;
    std::shared_ptr<dart::dynamics::BoxShape> box(
            new dart::dynamics::BoxShape(Eigen::Vector3d(floor_width, floor_width, floor_height)));
    auto shapeNode
        = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);
    shapeNode->getVisualAspect()->setColor(Eigen::Vector3d(0.3, 0.3, 0.4));

    /* Put the body into position */
    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.translation() = Eigen::Vector3d(0.0, 0.0, -floor_height / 2.0);
    body->getParentJoint()->setTransformFromParentBodyNode(tf);

    /* Create first domino */
    _dominoSkel = createDomino();

    auto world = dart::simulation::WorldPtr(new dart::simulation::World);
    world->addSkeleton(_manipulator);
    world->addSkeleton(floor);
    world->addSkeleton(_dominoSkel);

    world->setTimeStep(0.001);

    auto dartObj = new Object3D{&_scene};
    _dartWorld = std::make_shared<DartIntegration::World>(*dartObj, world);

    /* Phong shader instance */
    _resourceManager.set("color", new Shaders::Phong);

    /* Add more dominoes for fun! */
    const double default_angle = 20.0 * M_PI / 180.0;

    Eigen::Vector6d positions = _dominoSkel->getPositions();
    double totalAngle = 0.;
    _dominoId = 0;

    for (int i = 0; i < 5; i++) {
        auto domino = createNewDomino(positions, default_angle, totalAngle);
        world->addSkeleton(domino);
        positions = domino->getPositions();
    }

    /* Create a soft body node in different location */
    Eigen::Vector6d pos(Eigen::Vector6d::Zero());
    // pos(4) = -0.5;
    pos(5) = 0.5;
    auto soft = dart::dynamics::Skeleton::create("soft");
    addSoftBody<dart::dynamics::FreeJoint>(soft, "soft box");
    soft->getJoint(0)->setPositions(pos);

    // world->addSkeleton(soft);

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();

    redraw();
}

void DartExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    _camera->setViewport(size);
}

void DartExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);
    // Debug{}<<_timeline.previousFrameDuration();

    /* Step DART simulation */
    /* Need 15 steps as the time step in DART is 15 times smaller 60Hz */
    for (int i = 0; i < 15; i++) {
        updateManipulator(_manipulator);
        _dartWorld->step();
    }

    /* Update graphic meshes/materials and render */
    updateGraphics();
    _camera->draw(_drawables);

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}

void DartExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Down)
        _cameraObject->rotateX(Deg(5.0f));
    else if(event.key() == KeyEvent::Key::Up)
        _cameraObject->rotateX(Deg(-5.0f));
    else if(event.key() == KeyEvent::Key::Left)
        _cameraRig->rotateY(Deg(-5.0f));
    else if(event.key() == KeyEvent::Key::Right)
        _cameraRig->rotateY(Deg(5.0f));
    else if(event.key() == KeyEvent::Key::P)
    {
        _desiredPositions(1) = 120.0 * M_PI / 180.0;
        _desiredPositions(2) = -120.0 * M_PI / 180.0;
    }
    else return;

    event.setAccepted();
}

void DartExample::updateGraphics() {
    /* We refresh the graphical models only once per 60Hz */
    _dartWorld->refresh();

    for(auto& object : _dartWorld->updatedShapeObjects()){
        MaterialData mat;
        mat._ambientColor = object->shapeData().get().material.ambientColor();
        mat._diffuseColor = object->shapeData().get().material.diffuseColor();
        mat._specularColor = object->shapeData().get().material.specularColor();
        mat._shininess = object->shapeData().get().material.shininess();

        if (mat._shininess < 1e-4f)
            mat._shininess = 80.f;

        Mesh* mesh = object->shapeData().get().mesh;

        auto it = _coloredObjects.insert(std::make_pair(object, nullptr));
        if(it.second){
            auto coloredObj = new ColoredObject(mesh, mat, static_cast<Object3D*>(&(object->object())), &_drawables);
            if(object->shapeNode()->getShape()->getType() == dart::dynamics::SoftMeshShape::getStaticType())
                coloredObj->setSoftBody();
            it.first->second = coloredObj;
        }
        else {
            it.first->second->setMesh(mesh).setMaterial(mat);
        }
    }

    _dartWorld->clearUpdatedShapeObjects();
}

void DartExample::updateManipulator(dart::dynamics::SkeletonPtr skel) {
    Eigen::VectorXd q = skel->getPositions();
    Eigen::VectorXd dq = skel->getVelocities();

    q += dq * skel->getTimeStep();

    Eigen::VectorXd q_err = _desiredPositions - q;
    Eigen::VectorXd dq_err = -dq;

    const Eigen::MatrixXd& M = skel->getMassMatrix();
    const Eigen::VectorXd& Cg = skel->getCoriolisAndGravityForces();

    Eigen::VectorXd forces = M * (_pGain * q_err + _dGain * dq_err) + Cg;

    skel->setForces(forces);
}

dart::dynamics::SkeletonPtr DartExample::createNewDomino(Eigen::Vector6d position, double angle, double& totalAngle)
{
    const double default_distance = default_domino_height / 2.0;
    /* Create a new domino */
    dart::dynamics::SkeletonPtr newDomino = _dominoSkel->clone();
    newDomino->setName("domino #" + std::to_string(_dominoId++));

    /* Compute the position for the new domino */
    Eigen::Vector3d dx = default_distance * Eigen::Vector3d(
            std::cos(totalAngle), std::sin(totalAngle), 0.0);

    Eigen::Vector6d x = position;
    x.tail<3>() += dx;

    /* Adjust the angle for the new domino */
    x[2] = totalAngle + angle;
    totalAngle += angle;

    newDomino->setPositions(x);

    return newDomino;
}

ColoredObject::ColoredObject(Mesh* mesh, const MaterialData& material, Object3D* parent, SceneGraph::DrawableGroup3D* group):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _mesh{mesh}, _shader{ViewerResourceManager::instance().get<Shaders::Phong>("color")}, _material(material) {}

void ColoredObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader->setAmbientColor(_material._ambientColor)
        .setDiffuseColor(_material._diffuseColor)
        .setSpecularColor(_material._specularColor)
        .setShininess(_material._shininess)
        .setLightPosition(camera.cameraMatrix().transformPoint({0.f, 2.f, 3.f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix());

    if(_isSoftBody)
        Renderer::disable(Renderer::Feature::FaceCulling);
    _mesh->draw(*_shader);
    if(_isSoftBody)
        Renderer::enable(Renderer::Feature::FaceCulling);
}


ColoredObject& ColoredObject::setMesh(Mesh* mesh){
    _mesh = mesh;
    return *this;
}
ColoredObject& ColoredObject::setMaterial(const MaterialData& material){
    _material = material;
    return *this;
}
ColoredObject& ColoredObject::setSoftBody(bool softBody){
    _isSoftBody = softBody;
    return *this;
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::DartExample)
