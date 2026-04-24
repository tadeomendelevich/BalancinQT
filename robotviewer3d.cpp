#include "robotviewer3d.h"

#include <QVBoxLayout>
#include <QCoreApplication>
#include <QQuaternion>
#include <QVector3D>
#include <QColor>
#include <QSizePolicy>

#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QOrbitCameraController>

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QBoundingVolume>

RobotViewer3D::RobotViewer3D(QWidget *parent)
    : QWidget(parent)
    , m_view(new Qt3DExtras::Qt3DWindow())
    , m_robotTransform(nullptr)
{
    m_view->defaultFrameGraph()->setClearColor(QColor(13, 26, 8));

    auto *root = new Qt3DCore::QEntity();

    // --- Cámara ---
    Qt3DRender::QCamera *cam = m_view->camera();
    cam->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.1f, 2000.0f);
    cam->setPosition(QVector3D(300.0f, 80.0f, 300.0f));
    cam->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    cam->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));

    auto *camCtrl = new Qt3DExtras::QOrbitCameraController(root);
    camCtrl->setCamera(cam);
    camCtrl->setLinearSpeed(300.0f);
    camCtrl->setLookSpeed(180.0f);

    // --- Luz principal (frontal-superior) ---
    auto *lightEntity1 = new Qt3DCore::QEntity(root);
    auto *light1 = new Qt3DRender::QPointLight(lightEntity1);
    light1->setColor(Qt::white);
    light1->setIntensity(1.5f);
    auto *lightT1 = new Qt3DCore::QTransform(lightEntity1);
    lightT1->setTranslation(QVector3D(200.0f, 400.0f, 300.0f));
    lightEntity1->addComponent(light1);
    lightEntity1->addComponent(lightT1);

    // --- Luz de relleno (trasera) ---
    auto *lightEntity2 = new Qt3DCore::QEntity(root);
    auto *light2 = new Qt3DRender::QPointLight(lightEntity2);
    light2->setColor(QColor(100, 180, 255));
    light2->setIntensity(0.8f);
    auto *lightT2 = new Qt3DCore::QTransform(lightEntity2);
    lightT2->setTranslation(QVector3D(-200.0f, 100.0f, -300.0f));
    lightEntity2->addComponent(light2);
    lightEntity2->addComponent(lightT2);

    // --- Entidad base: escala + orientación estática del modelo ---
    auto *robotBaseEntity = new Qt3DCore::QEntity(root);
    auto *baseTransform = new Qt3DCore::QTransform(robotBaseEntity);
    baseTransform->setScale(12.0f);
    // Fusion exporta Z-up: rotar -90° en X para que el robot quede parado en Qt3D (Y-up)
    baseTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), -90.0f));
    robotBaseEntity->addComponent(baseTransform);

    // --- Entidad robot: solo rotación dinámica de pitch ---
    auto *robotEntity = new Qt3DCore::QEntity(robotBaseEntity);
    m_robotTransform = new Qt3DCore::QTransform(robotEntity);
    robotEntity->addComponent(m_robotTransform);

    auto *mesh = new Qt3DRender::QMesh(robotEntity);
    mesh->setSource(QUrl::fromLocalFile(
        QCoreApplication::applicationDirPath() + "/AutoMicro.obj"));
    robotEntity->addComponent(mesh);

    auto *material = new Qt3DExtras::QPhongMaterial(robotEntity);
    material->setDiffuse(QColor(180, 180, 200));
    robotEntity->addComponent(material);

    // Auto-centrado: desplaza el modelo al origen una vez que se calcula el bounding box
    auto *bv = new Qt3DCore::QBoundingVolume(robotEntity);
    robotEntity->addComponent(bv);
    QObject::connect(bv, &Qt3DCore::QBoundingVolume::implicitMaxPointChanged, bv,
        [bv, this]() {
            QVector3D center = (bv->implicitMinPoint() + bv->implicitMaxPoint()) / 2.0f;
            m_robotTransform->setTranslation(-center);
        }, Qt::SingleShotConnection);

    m_view->setRootEntity(root);

    // Embeber la ventana Qt3D en este widget — sin mínimo fijo para no afectar otros tabs
    QWidget *container = QWidget::createWindowContainer(m_view, this);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(container);
}

void RobotViewer3D::setPitch(float degrees)
{
    if (!m_robotTransform)
        return;
    m_robotTransform->setRotation(
        QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), degrees));
}
