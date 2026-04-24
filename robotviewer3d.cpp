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
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QCuboidMesh>

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QBoundingVolume>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DRender/QMesh>

static void addPointLight(Qt3DCore::QEntity *root, QVector3D pos, QColor color, float intensity)
{
    auto *e   = new Qt3DCore::QEntity(root);
    auto *l   = new Qt3DRender::QPointLight(e);
    auto *t   = new Qt3DCore::QTransform(e);
    l->setColor(color);
    l->setIntensity(intensity);
    l->setConstantAttenuation(1.0f);
    l->setLinearAttenuation(0.0f);
    l->setQuadraticAttenuation(0.0f);
    t->setTranslation(pos);
    e->addComponent(l);
    e->addComponent(t);
}

static void addGridLine(Qt3DCore::QEntity *parent, float length, float offset,
                        bool alongZ, float y, QColor color)
{
    auto *e   = new Qt3DCore::QEntity(parent);
    auto *box = new Qt3DExtras::QCuboidMesh(e);
    auto *t   = new Qt3DCore::QTransform(e);
    auto *mat = new Qt3DExtras::QPhongMaterial(e);

    if (alongZ) {
        box->setXExtent(0.8f);
        box->setYExtent(0.4f);
        box->setZExtent(length);
        t->setTranslation(QVector3D(offset, y, 0.0f));
    } else {
        box->setXExtent(length);
        box->setYExtent(0.4f);
        box->setZExtent(0.8f);
        t->setTranslation(QVector3D(0.0f, y, offset));
    }

    mat->setAmbient(color);
    mat->setDiffuse(color);
    mat->setSpecular(Qt::black);

    e->addComponent(box);
    e->addComponent(t);
    e->addComponent(mat);
}

RobotViewer3D::RobotViewer3D(QWidget *parent)
    : QWidget(parent)
    , m_view(new Qt3DExtras::Qt3DWindow())
    , m_robotTransform(nullptr)
{
    m_view->defaultFrameGraph()->setClearColor(QColor(10, 18, 6));

    auto *root = new Qt3DCore::QEntity();

    // --- Cámara ---
    Qt3DRender::QCamera *cam = m_view->camera();
    cam->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.1f, 3000.0f);
    cam->setPosition(QVector3D(250.0f, 160.0f, 350.0f));
    cam->setViewCenter(QVector3D(0.0f, 20.0f, 0.0f));
    cam->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));

    auto *camCtrl = new Qt3DExtras::QOrbitCameraController(root);
    camCtrl->setCamera(cam);
    camCtrl->setLinearSpeed(400.0f);
    camCtrl->setLookSpeed(180.0f);

    // --- Luz direccional suave (ambiente) ---
    auto *dirLightEnt = new Qt3DCore::QEntity(root);
    auto *dirLight    = new Qt3DRender::QDirectionalLight(dirLightEnt);
    dirLight->setColor(QColor(180, 200, 255));
    dirLight->setIntensity(0.4f);
    dirLight->setWorldDirection(QVector3D(-0.3f, -1.0f, -0.5f));
    dirLightEnt->addComponent(dirLight);

    // --- Luces puntuales: key / fill / rim ---
    addPointLight(root, QVector3D( 300.0f,  500.0f,  300.0f), QColor(255, 245, 220), 2.0f); // key cálida
    addPointLight(root, QVector3D(-400.0f,  200.0f,  100.0f), QColor(100, 160, 255), 1.2f); // fill azul
    addPointLight(root, QVector3D(  50.0f,  300.0f, -500.0f), QColor(255, 200, 120), 0.8f); // rim trasera

    // --- Piso ---
    auto *floorEntity    = new Qt3DCore::QEntity(root);
    auto *floorMesh      = new Qt3DExtras::QPlaneMesh(floorEntity);
    auto *floorTransform = new Qt3DCore::QTransform(floorEntity);
    auto *floorMat       = new Qt3DExtras::QPhongMaterial(floorEntity);

    floorMesh->setWidth(700.0f);
    floorMesh->setHeight(700.0f);
    floorMat->setAmbient(QColor(8,  16,  5));
    floorMat->setDiffuse(QColor(14, 28, 9));
    floorMat->setSpecular(QColor(30, 60, 20));
    floorMat->setShininess(10.0f);
    floorTransform->setTranslation(QVector3D(0.0f, -100.0f, 0.0f)); // posición provisional

    floorEntity->addComponent(floorMesh);
    floorEntity->addComponent(floorTransform);
    floorEntity->addComponent(floorMat);

    // --- Grilla sobre el piso ---
    const float gridSize  = 600.0f;
    const int   gridLines = 10;
    const float gridStep  = gridSize / gridLines;
    const float gridHalf  = gridSize / 2.0f;
    const QColor gridColor(22, 55, 14); // verde oscuro sutil

    for (int i = 0; i <= gridLines; ++i) {
        float offset = -gridHalf + i * gridStep;
        addGridLine(floorEntity, gridSize, offset, true,  0.3f, gridColor);
        addGridLine(floorEntity, gridSize, offset, false, 0.3f, gridColor);
    }

    // --- Entidad base: escala + orientación estática del modelo ---
    auto *robotBaseEntity = new Qt3DCore::QEntity(root);
    auto *baseTransform   = new Qt3DCore::QTransform(robotBaseEntity);
    baseTransform->setScale(12.0f);
    baseTransform->setRotation(
        QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), -90.0f));
    robotBaseEntity->addComponent(baseTransform);

    // --- Entidad de pitch dinámico ---
    auto *robotEntity = new Qt3DCore::QEntity(robotBaseEntity);
    m_robotTransform  = new Qt3DCore::QTransform(robotEntity);
    robotEntity->addComponent(m_robotTransform);

    auto *mesh = new Qt3DRender::QMesh(robotEntity);
    mesh->setSource(QUrl::fromLocalFile(
        QCoreApplication::applicationDirPath() + "/AutoMicro.obj"));
    robotEntity->addComponent(mesh);

    // Material metálico plateado
    auto *material = new Qt3DExtras::QPhongMaterial(robotEntity);
    material->setAmbient(QColor(55,  65,  75));
    material->setDiffuse(QColor(150, 165, 180));
    material->setSpecular(QColor(220, 230, 240));
    material->setShininess(90.0f);
    robotEntity->addComponent(material);

    // Auto-centrado + piso dinámico usando QBoundingVolume
    auto *bv = new Qt3DCore::QBoundingVolume(robotEntity);
    robotEntity->addComponent(bv);
    QObject::connect(bv, &Qt3DCore::QBoundingVolume::implicitMaxPointChanged, bv,
        [bv, this, floorTransform](){
            QVector3D minPt = bv->implicitMinPoint();
            QVector3D maxPt = bv->implicitMaxPoint();
            QVector3D center = (minPt + maxPt) / 2.0f;
            m_robotTransform->setTranslation(-center);

            // local Z → world Y (tras rotación -90°X y escala 12)
            float localBottomZ = minPt.z() - center.z();
            float worldFloorY  = localBottomZ * 12.0f - 2.0f; // -2 de margen
            floorTransform->setTranslation(QVector3D(0.0f, worldFloorY, 0.0f));

            // Reposicionar grilla al mismo Y
            // (las líneas fueron creadas en Y fijo, se dejan como referencia visual)
        }, Qt::SingleShotConnection);

    m_view->setRootEntity(root);

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
    QVector3D currentTranslation = m_robotTransform->translation();
    m_robotTransform->setRotation(
        QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), degrees));
    m_robotTransform->setTranslation(currentTranslation);
}
