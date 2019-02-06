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

#include <dart/dynamics/BallJoint.hpp>
#include <dart/dynamics/BodyNode.hpp>
#include <dart/dynamics/BoxShape.hpp>
#include <dart/dynamics/CylinderShape.hpp>
#include <dart/dynamics/EllipsoidShape.hpp>
#include <dart/dynamics/FreeJoint.hpp>
#include <dart/dynamics/MeshShape.hpp>
#include <dart/dynamics/RevoluteJoint.hpp>
#include <dart/dynamics/SoftBodyNode.hpp>
#include <dart/dynamics/Skeleton.hpp>
#include <dart/dynamics/WeldJoint.hpp>
#include <dart/simulation/World.hpp>

#include <Corrade/Utility/Directory.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.hpp>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/Trade/PhongMaterialData.h>

#include <Magnum/DartIntegration/ConvertShapeNode.h>
#include <Magnum/DartIntegration/World.h>

namespace Magnum { namespace Examples {

namespace {
constexpr Double DefaultHeight = 1.0; // m
constexpr Double DefaultWidth = 0.2; // m
constexpr Double DefaultDepth = 0.2; // m

constexpr Double DefaultRestPosition = 0.0;

constexpr Double DefaultStiffness = 0.0;

constexpr Double DefaultDamping = 5.0;

void setGeometry(const dart::dynamics::BodyNodePtr& bn) {
    /* Create a BoxShape to be used for both visualization and collision checking */
    std::shared_ptr<dart::dynamics::BoxShape> box(new dart::dynamics::BoxShape(
        Eigen::Vector3d(DefaultWidth, DefaultDepth, DefaultHeight)));

    /* Create a shape node for visualization and collision checking */
    auto shapeNode = bn->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);
    shapeNode->getVisualAspect()->setColor(dart::Color::Blue());

    /* Set the location of the shape node */
    Eigen::Isometry3d box_tf(Eigen::Isometry3d::Identity());
    Eigen::Vector3d center = Eigen::Vector3d(0, 0, DefaultHeight/2.0);
    box_tf.translation() = center;
    shapeNode->setRelativeTransform(box_tf);

    /* Move the center of mass to the center of the object */
    bn->setLocalCOM(center);
}

dart::dynamics::BodyNode* makeRootBody(const dart::dynamics::SkeletonPtr& pendulum, std::string name) {
    dart::dynamics::BallJoint::Properties properties;
    properties.mName = name + "_joint";
    properties.mRestPositions = Eigen::Vector3d::Constant(DefaultRestPosition);
    properties.mSpringStiffnesses = Eigen::Vector3d::Constant(DefaultStiffness);
    properties.mDampingCoefficients = Eigen::Vector3d::Constant(DefaultDamping);

    dart::dynamics::BodyNodePtr bn = pendulum->createJointAndBodyNodePair<dart::dynamics::BallJoint>(
        nullptr, properties, dart::dynamics::BodyNode::AspectProperties(name)).second;

    /* Make a shape for the Joint */
    auto ball = std::make_shared<dart::dynamics::EllipsoidShape>(Math::sqrt(2.0)*
        Eigen::Vector3d(DefaultWidth, DefaultWidth, DefaultWidth));
    auto shapeNode = bn->createShapeNodeWith<dart::dynamics::VisualAspect>(ball);
    auto va = shapeNode->getVisualAspect();
    CORRADE_INTERNAL_ASSERT(va);
    va->setColor(dart::Color::Blue());

    /* Set the geometry of the Body */
    setGeometry(bn);
    return bn;
}

dart::dynamics::BodyNode* addBody(const dart::dynamics::SkeletonPtr& pendulum, dart::dynamics::BodyNode* parent, std::string name) {
    using namespace Math::Literals;

    /* Set up the properties for the Joint */
    dart::dynamics::RevoluteJoint::Properties properties;
    properties.mName = name + "_joint";
    properties.mAxis = Eigen::Vector3d::UnitY();
    properties.mT_ParentBodyToJoint.translation() = Eigen::Vector3d(0, 0, DefaultHeight);
    properties.mRestPositions[0] = DefaultRestPosition;
    properties.mSpringStiffnesses[0] = DefaultStiffness;
    properties.mDampingCoefficients[0] = DefaultDamping;

    /* Create a new BodyNode, attached to its parent by a RevoluteJoint */
    dart::dynamics::BodyNodePtr bn = pendulum->createJointAndBodyNodePair<dart::dynamics::RevoluteJoint>(
        parent, properties, dart::dynamics::BodyNode::AspectProperties(name)).second;

    /* Make a shape for the Joint */
    auto cyl = std::make_shared<dart::dynamics::CylinderShape>(DefaultWidth/2.0, DefaultDepth);

    /* Line up the cylinder with the Joint axis */
    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.linear() = dart::math::eulerXYZToMatrix(
        Eigen::Vector3d(Double(Radd(90.0_deg)), 0, 0));

    auto shapeNode = bn->createShapeNodeWith<dart::dynamics::VisualAspect>(cyl);
    shapeNode->getVisualAspect()->setColor(dart::Color::Blue());
    shapeNode->setRelativeTransform(tf);

    /* Set the geometry of the Body */
    setGeometry(bn);
    return bn;
}

/* Add a soft body with the specified Joint type to a chain */
template<class JointType> dart::dynamics::BodyNode* addSoftBody(const dart::dynamics::SkeletonPtr& chain, const std::string& name, dart::dynamics::BodyNode* parent = nullptr) {
    constexpr Double default_shape_density = 1000; // kg/m^3
    constexpr Double default_shape_height  = 0.1;  // m
    constexpr Double default_shape_width   = 0.03; // m
    constexpr Double default_skin_thickness = 1e-3; // m

    constexpr Double default_vertex_stiffness = 1000.0;
    constexpr Double default_edge_stiffness = 1.0;
    constexpr Double default_soft_damping = 5.0;

    /* Set the Joint properties */
    typename JointType::Properties joint_properties;
    joint_properties.mName = name + "_joint";
    if(parent) {
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
    Double width = default_shape_height, height = 2*default_shape_width;
    Eigen::Vector3d dims(width, width, height);

    Double mass = 2*dims[0]*dims[1] + 2*dims[0]*dims[2] + 2*dims[1]*dims[2];
    mass *= default_shape_density * default_skin_thickness;
    soft_properties = dart::dynamics::SoftBodyNodeHelper::makeBoxProperties(dims, Eigen::Isometry3d::Identity(), Eigen::Vector3i(4,4,4), mass);

    soft_properties.mKv = default_vertex_stiffness;
    soft_properties.mKe = default_edge_stiffness;
    soft_properties.mDampCoeff = default_soft_damping;

    /* Create the Joint and Body pair */
    dart::dynamics::SoftBodyNode::Properties body_properties(dart::dynamics::BodyNode::AspectProperties(name), soft_properties);
    dart::dynamics::SoftBodyNode* bn = chain->createJointAndBodyNodePair<JointType, dart::dynamics::SoftBodyNode>(parent, joint_properties, body_properties).second;

    /* Zero out the inertia for the underlying BodyNode */
    dart::dynamics::Inertia inertia;
    inertia.setMoment(1e-8*Eigen::Matrix3d::Identity());
    inertia.setMass(1e-8);
    bn->setInertia(inertia);

    return bn;
}
}

using namespace Magnum::Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class DartExample: public Platform::Application {
    public:
        explicit DartExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        Matrix4 _transformation, _projection;
        Vector2i _previousMousePosition;

        std::unique_ptr<Magnum::DartIntegration::World> _dartWorld;
        dart::simulation::WorldPtr _world;
};

DartExample::DartExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Primitives Example")}
{
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    _transformation =
        Matrix4::rotationX(30.0_degf)*Matrix4::rotationY(40.0_degf);
    _projection =
        Matrix4::perspectiveProjection(
            35.0_degf, Vector2{windowSize()}.aspectRatio(), 0.01f, 100.0f)*
        Matrix4::translation(Vector3::zAxis(-10.0f));


    /* Create an empty Skeleton with the name "pendulum" */
    std::string name = "pendulum";
    dart::dynamics::SkeletonPtr pendulum = dart::dynamics::Skeleton::create(name);

    /* Add each body to the last BodyNode in the pendulum */
    dart::dynamics::BodyNode* bn = makeRootBody(pendulum, "body1");
    bn = addBody(pendulum, bn, "body2");
    bn = addBody(pendulum, bn, "body3");
    bn = addBody(pendulum, bn, "body4");
    bn = addBody(pendulum, bn, "body5");

    /* Set the initial joint positions so that the pendulum
       starts to swing right away */
    pendulum->getDof(1)->setPosition(Double(Radd(120.0_deg)));
    pendulum->getDof(2)->setPosition(Double(Radd(20.0_deg)));
    pendulum->getDof(3)->setPosition(Double(Radd(-50.0_deg)));

    /* Create a world and add the pendulum to the world */
    _world.reset(new dart::simulation::World);
    _world->addSkeleton(pendulum);

    Scene3D scene;
    Object3D* obj = new Object3D{&scene};

    _dartWorld.reset(new Magnum::DartIntegration::World(*obj, *_world));
}

void DartExample::drawEvent() {
    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    for(int i = 0; i < 15; ++i)
        _dartWorld->step();
    // (*_world).step();

    _dartWorld->refresh();
    _dartWorld->clearUpdatedShapeObjects();

    swapBuffers();
    redraw();
}

void DartExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    _previousMousePosition = event.position();
    event.setAccepted();
}

void DartExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/
        Vector2{GL::defaultFramebuffer.viewport().size()};

    _transformation =
        Matrix4::rotationX(Rad{delta.y()})*
        _transformation*
        Matrix4::rotationY(Rad{delta.x()});

    _previousMousePosition = event.position();
    event.setAccepted();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::DartExample)
