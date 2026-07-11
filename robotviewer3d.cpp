#include "robotviewer3d.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QCoreApplication>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <QSizePolicy>
#include <QtMath>

#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QPhongAlphaMaterial>
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

// Tríada de ejes en el origen (convención CAD: X rojo, Y verde, Z azul).
static void addAxisLine(Qt3DCore::QEntity *root, QVector3D axis, QColor color, float length)
{
    auto *e   = new Qt3DCore::QEntity(root);
    auto *box = new Qt3DExtras::QCuboidMesh(e);
    auto *t   = new Qt3DCore::QTransform(e);
    auto *mat = new Qt3DExtras::QPhongMaterial(e);

    const float thick = 1.2f;
    box->setXExtent(axis.x() != 0.0f ? length : thick);
    box->setYExtent(axis.y() != 0.0f ? length : thick);
    box->setZExtent(axis.z() != 0.0f ? length : thick);
    t->setTranslation(axis * (length / 2.0f) + QVector3D(0.0f, 0.6f, 0.0f));

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
    , m_baseTransform(nullptr)
{
    m_view->defaultFrameGraph()->setClearColor(QColor(10, 18, 6));

    auto *root = new Qt3DCore::QEntity();

    // --- Cámara ---
    Qt3DRender::QCamera *cam = m_view->camera();
    cam->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.1f, 3000.0f);
    cam->setPosition(QVector3D(250.0f, 160.0f, 350.0f));
    cam->setViewCenter(QVector3D(0.0f, 80.0f, 0.0f));
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
    floorTransform->setTranslation(QVector3D(0.0f, 0.0f, 0.0f)); // piso fijo en Y=0

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

    // --- Tríada de ejes en el origen (X rojo, Y verde, Z azul, estilo CAD) ---
    addAxisLine(root, QVector3D(1, 0, 0), QColor(200,  60,  60), 70.0f);
    addAxisLine(root, QVector3D(0, 1, 0), QColor( 60, 180,  60), 70.0f);
    addAxisLine(root, QVector3D(0, 0, 1), QColor( 70, 110, 220), 70.0f);

    // --- Obstáculo frente al robot (sensores ADC 5/6/8) ---
    // Pared roja semitransparente apoyada en el piso; posición/visibilidad las
    // maneja applyModelTransform() según pose + setFrontObstacle().
    {
        m_obstacleEntity = new Qt3DCore::QEntity(root);
        auto *wall = new Qt3DExtras::QCuboidMesh(m_obstacleEntity);
        wall->setXExtent(4.0f);     // fina, de frente al robot (a yaw 0 el frente es +X)
        wall->setYExtent(60.0f);    // alto
        wall->setZExtent(140.0f);   // ancho
        m_obstacleTransform = new Qt3DCore::QTransform(m_obstacleEntity);
        m_obstacleMaterial  = new Qt3DExtras::QPhongAlphaMaterial(m_obstacleEntity);
        m_obstacleMaterial->setAmbient(QColor(120, 20, 20));
        m_obstacleMaterial->setDiffuse(QColor(220, 50, 50));
        m_obstacleMaterial->setSpecular(QColor(60, 20, 20));
        m_obstacleMaterial->setAlpha(0.45f);
        m_obstacleEntity->addComponent(wall);
        m_obstacleEntity->addComponent(m_obstacleTransform);
        m_obstacleEntity->addComponent(m_obstacleMaterial);
        m_obstacleEntity->setEnabled(false);   // oculto hasta que haya detección
    }

    // --- Entidad base: escala + orientación estática del modelo ---
    auto *robotBaseEntity = new Qt3DCore::QEntity(root);
    m_baseTransform = new Qt3DCore::QTransform(robotBaseEntity);
    m_baseTransform->setScale(12.0f);
    m_baseTransform->setRotation(
        QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), -90.0f));
    // Altura inicial hasta que carga el mesh; después applyModelTransform() la
    // recalcula en cada pose para que el modelo siempre apoye en el piso (Y=0).
    m_baseTransform->setTranslation(QVector3D(0.0f, 25.0f, 0.0f));
    robotBaseEntity->addComponent(m_baseTransform);

    // --- Entidad de pose dinámica (pitch de balanceo + yaw de odometría) ---
    auto *robotEntity = new Qt3DCore::QEntity(robotBaseEntity);
    m_robotTransform  = new Qt3DCore::QTransform(robotEntity);
    robotEntity->addComponent(m_robotTransform);

    auto *mesh = new Qt3DRender::QMesh(robotEntity);
    mesh->setSource(QUrl::fromLocalFile(
        QCoreApplication::applicationDirPath() + "/AutoMicro.obj"));
    robotEntity->addComponent(mesh);

    // Material metálico plateado (guardado: el tinte por inclinación lo modula)
    m_robotMaterial = new Qt3DExtras::QPhongMaterial(robotEntity);
    m_robotMaterial->setAmbient(QColor(55,  65,  75));
    m_robotMaterial->setDiffuse(QColor(150, 165, 180));
    m_robotMaterial->setSpecular(QColor(220, 230, 240));
    m_robotMaterial->setShininess(90.0f);
    robotEntity->addComponent(m_robotMaterial);

    // Auto-centrado: guarda el centro geométrico del mesh como pivote de rotación.
    // Se usa conexión manual (no SingleShot) para ignorar disparos con bounds
    // todavía en cero (antes de que el mesh termine de cargar).
    auto *bv   = new Qt3DCore::QBoundingVolume(robotEntity);
    auto *conn = new QMetaObject::Connection();
    robotEntity->addComponent(bv);
    *conn = QObject::connect(bv, &Qt3DCore::QBoundingVolume::implicitMaxPointChanged,
        [bv, this, conn](){
            QVector3D minPt = bv->implicitMinPoint();
            QVector3D maxPt = bv->implicitMaxPoint();
            if (minPt == maxPt) return; // mesh aún no cargado, esperar

            m_modelCenter = (minPt + maxPt) / 2.0f;
            m_modelMin    = minPt;   // bbox local: base del apoyo en el piso
            m_modelMax    = maxPt;
            applyModelTransform();

            QObject::disconnect(*conn);
            delete conn;
        });

    m_view->setRootEntity(root);

    // --- Animaciones y órbita automática ---
    // Pose del modelo: interpola de la pose mostrada a la última recibida (el push
    // de odometría llega a 2 Hz — sin esto el modelo saltaba de pose en pose).
    m_poseAnim = new QVariantAnimation(this);
    m_poseAnim->setDuration(240);
    m_poseAnim->setEasingCurve(QEasingCurve::OutQuad);

    // Cámara: transición suave entre vistas predefinidas (estilo Fusion 360).
    m_camAnim = new QVariantAnimation(this);
    m_camAnim->setDuration(450);
    m_camAnim->setEasingCurve(QEasingCurve::InOutCubic);

    // Turntable: órbita automática lenta alrededor del robot.
    m_orbitTimer = new QTimer(this);
    m_orbitTimer->setInterval(30);
    connect(m_orbitTimer, &QTimer::timeout, this, [this]() {
        Qt3DRender::QCamera *c = m_view->camera();
        QVector3D center = c->viewCenter();
        QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 0.35f);
        c->setPosition(center + r.rotatedVector(c->position() - center));
    });

    // --- Layout: barra de vistas + HUD arriba, visor abajo ---
    QWidget *container = QWidget::createWindowContainer(m_view, this);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    {
        auto *btnRow = new QHBoxLayout();
        btnRow->setContentsMargins(4, 4, 4, 4);
        btnRow->setSpacing(4);

        const struct { const char *label; ViewPreset v; } presets[] = {
            { "ISO",    ViewPreset::Isometrica },
            { "Frente", ViewPreset::Frente     },
            { "Atrás",  ViewPreset::Atras      },
            { "Izq.",   ViewPreset::Izquierda  },
            { "Der.",   ViewPreset::Derecha    },
            { "Arriba", ViewPreset::Arriba     },
        };
        for (const auto &p : presets) {
            auto *b = new QPushButton(QString::fromUtf8(p.label), this);
            b->setFixedHeight(24);
            b->setFocusPolicy(Qt::NoFocus); // que no robe el teclado del orbit controller
            ViewPreset v = p.v;
            connect(b, &QPushButton::clicked, this, [this, v]() { setView(v); });
            btnRow->addWidget(b);
        }

        // Órbita automática (toggle): la cámara gira sola alrededor del robot.
        auto *btnOrbit = new QPushButton(QString::fromUtf8("Órbita auto"), this);
        btnOrbit->setFixedHeight(24);
        btnOrbit->setCheckable(true);
        btnOrbit->setFocusPolicy(Qt::NoFocus);
        connect(btnOrbit, &QPushButton::toggled, this, [this](bool on) {
            if (on) m_orbitTimer->start(); else m_orbitTimer->stop();
        });
        btnRow->addWidget(btnOrbit);

        btnRow->addStretch();

        // HUD numérico: balanceo y rumbo actuales, siempre a la vista.
        m_hudLabel = new QLabel(QString::fromUtf8("Roll: --.-°   Rumbo: ---°"), this);
        QFont hf = m_hudLabel->font();
        hf.setBold(true);
        hf.setFamily("Courier");
        m_hudLabel->setFont(hf);
        btnRow->addWidget(m_hudLabel);

        layout->addLayout(btnRow);
    }

    layout->addWidget(container);
}

// Pose mostrada → transform del modelo (rotación alrededor del CENTRO del mesh),
// tinte del material por inclinación y HUD.
//
// Ejes locales del mesh (la base ya lo rota -90° en X): el pitch de balanceo es
// alrededor del Y local (como venía siendo) y el yaw de rumbo alrededor del Z
// local, que tras la base apunta al Y del mundo (vertical). Orden yaw∘pitch:
// primero se inclina sobre su eje de ruedas, después gira todo el conjunto.
void RobotViewer3D::applyModelTransform()
{
    if (!m_robotTransform)
        return;

    // Tres ejes (orden yaw∘pitch∘lat): primero banking alrededor del eje de
    // avance (X local), después balanceo sobre el eje de ruedas (Y local),
    // después el rumbo (Z local = vertical del mundo tras la base de -90°X).
    QQuaternion q =
        QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 0.0f, 1.0f), m_dispYaw) *
        QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), m_dispPitch) *
        QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), m_dispLat);

    // QTransform aplica T·R·S: rotar también la traslación de centrado
    // (p' = R·(p - c)) deja el pivote exactamente en el centro del modelo.
    m_robotTransform->setRotation(q);
    m_robotTransform->setTranslation(-q.rotatedVector(m_modelCenter));

    // Apoyo en el piso (2026-07-10): la altura de la base ya no es fija — se
    // calcula el punto más bajo del bounding box del modelo YA rotado y se
    // corrige la traslación de la base para que ese punto quede en Y=0. Con
    // altura fija (25), a ±90° o volcado el modelo atravesaba el piso.
    // 2026-07-10 bis: la composición base·hijo ahora usa las MATRICES REALES
    // de Qt3D (`matrix()` de ambos QTransform, ya con la rotación/centrado del
    // hijo aplicados arriba) en vez de derivarla a mano con quaterniones —
    // la versión manual dejaba pasar el piso con ángulos negativos (error de
    // convención en la composición). Como la traslación de la base entra
    // lineal en la Y del mundo, corregir con el delta (-minY) es exacto.
    // Se usan las 8 esquinas del bbox: cota conservadora (el mesh puede quedar
    // apenas por encima del piso en poses intermedias, nunca por debajo).
    if (m_baseTransform && m_modelMin != m_modelMax) {
        const QMatrix4x4 M = m_baseTransform->matrix() * m_robotTransform->matrix();
        float minY = 0.0f;
        bool  first = true;
        for (int i = 0; i < 8; ++i) {
            const QVector3D corner((i & 1) ? m_modelMax.x() : m_modelMin.x(),
                                   (i & 2) ? m_modelMax.y() : m_modelMin.y(),
                                   (i & 4) ? m_modelMax.z() : m_modelMin.z());
            const float wy = M.map(corner).y();
            if (first || wy < minY) { minY = wy; first = false; }
        }
        // +0.6 de margen: la grilla del piso llega a Y≈0.5 — evita z-fighting
        // entre las ruedas y las líneas de la grilla.
        const QVector3D t = m_baseTransform->translation();
        m_baseTransform->setTranslation(QVector3D(t.x(), t.y() - minY + 0.6f, t.z()));
    }

    // Tinte por inclinación: plateado en equilibrio → rojo cerca del ángulo de
    // caída. Es feedback inmediato de "qué tan al límite está" sin mirar números.
    if (m_robotMaterial) {
        float a = qMin(qAbs(m_dispPitch) / 20.0f, 1.0f);  // 0° plateado, ≥20° rojo pleno
        auto lerp = [a](int from, int to) { return int(from + a * (to - from)); };
        m_robotMaterial->setDiffuse(QColor(lerp(150, 229), lerp(165, 72), lerp(180, 77)));
        m_robotMaterial->setAmbient(QColor(lerp(55, 90),  lerp(65, 30),  lerp(75, 32)));
    }

    if (m_hudLabel) {
        m_hudLabel->setText(QString::fromUtf8("Roll: %1°   Lat: %2°   Rumbo: %3°")
                                .arg((double)m_dispPitch, 5, 'f', 1)
                                .arg((double)m_dispLat,   5, 'f', 1)
                                .arg((double)m_dispYaw,   4, 'f', 0));
    }

    // Obstáculo frente al robot: visible solo si hay detección Y el roll está en
    // [-90°, +30°] (fuera de ese rango los sensores no miran al frente / el robot
    // está volcado y el dato no representa "algo adelante").
    if (m_obstacleEntity && m_obstacleTransform) {
        const bool visible = m_obsPresent &&
                             m_dispPitch >= -90.0f && m_dispPitch <= 30.0f;
        m_obstacleEntity->setEnabled(visible);
        if (visible) {
            // Media longitud del robot en escena: la pared arranca delante del
            // cuerpo, no del centro. La distancia ADC (0.06–0.36 m, mapeo crudo
            // NO calibrado — es para visualizar, no para medir) se estira a
            // escena con 400 unidades/m.
            const float scale = m_baseTransform ? m_baseTransform->scale() : 12.0f;
            const QVector3D ext = (m_modelMax - m_modelMin) * 0.5f * scale;
            const float halfR = qMax(ext.x(), qMax(ext.y(), ext.z()));
            const float sceneDist = halfR + 8.0f + (m_obsDistM - 0.06f) * 400.0f;

            // Frente del robot en el mundo: +X a yaw 0, girado por el rumbo
            // mostrado (yaw local Z == mundo +Y, mismo signo). Si la pared
            // aparece DETRÁS del modelo, negar dir (misma incógnita de signo
            // que ODOM_THETA_SIGN).
            const float yawRad = qDegreesToRadians(m_dispYaw);
            const QVector3D dir(qCos(yawRad), 0.0f, -qSin(yawRad));
            m_obstacleTransform->setTranslation(
                QVector3D(dir.x() * sceneDist, 30.0f, dir.z() * sceneDist));
            m_obstacleTransform->setRotation(
                QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), m_dispYaw));

            if (m_obstacleMaterial) {
                // Más cerca = más opaca (feedback de proximidad sin números)
                float prox = 1.0f - qBound(0.0f, (m_obsDistM - 0.06f) / 0.30f, 1.0f);
                m_obstacleMaterial->setAlpha(0.25f + 0.45f * prox);
            }
        }
    }
}

void RobotViewer3D::setFrontObstacle(bool present, float distMeters)
{
    m_obsPresent = present;
    m_obsDistM   = distMeters;
    applyModelTransform();   // re-evalúa visibilidad/posición con la pose actual
}

void RobotViewer3D::animatePoseTo(float pitchDeg, float yawDeg, float latDeg)
{
    m_pitchDeg = pitchDeg;
    m_yawDeg   = yawDeg;
    m_latDeg   = latDeg;

    const float p0 = m_dispPitch;
    const float y0 = m_dispYaw;
    const float l0 = m_dispLat;

    // Yaw por el camino corto (no dar la vuelta larga al cruzar ±180°).
    float dy = yawDeg - y0;
    while (dy >  180.0f) dy -= 360.0f;
    while (dy < -180.0f) dy += 360.0f;
    const float dp = pitchDeg - p0;
    const float dl = latDeg   - l0;

    m_poseAnim->stop();
    m_poseAnim->disconnect();
    m_poseAnim->setStartValue(0.0f);
    m_poseAnim->setEndValue(1.0f);
    connect(m_poseAnim, &QVariantAnimation::valueChanged, this,
        [this, p0, y0, l0, dp, dy, dl](const QVariant &v) {
            float t = v.toFloat();
            m_dispPitch = p0 + dp * t;
            m_dispYaw   = y0 + dy * t;
            m_dispLat   = l0 + dl * t;
            if (m_dispYaw >  180.0f) m_dispYaw -= 360.0f;
            if (m_dispYaw < -180.0f) m_dispYaw += 360.0f;
            applyModelTransform();
        });
    m_poseAnim->start();
}

void RobotViewer3D::setPitch(float degrees)
{
    animatePoseTo(degrees, m_yawDeg, m_latDeg);   // conserva rumbo y banking actuales
}

void RobotViewer3D::setPose(float pitchDeg, float yawDeg, float latDeg)
{
    animatePoseTo(pitchDeg, yawDeg, latDeg);
}

void RobotViewer3D::goToView(const QVector3D &pos, const QVector3D &center,
                             const QVector3D &up, bool animated)
{
    Qt3DRender::QCamera *cam = m_view->camera();

    if (!animated) {
        cam->setPosition(pos);
        cam->setViewCenter(center);
        cam->setUpVector(up);
        return;
    }

    const QVector3D p0 = cam->position();
    const QVector3D c0 = cam->viewCenter();
    const QVector3D u0 = cam->upVector();

    m_camAnim->stop();
    m_camAnim->disconnect();
    m_camAnim->setStartValue(0.0f);
    m_camAnim->setEndValue(1.0f);
    connect(m_camAnim, &QVariantAnimation::valueChanged, this,
        [this, p0, c0, u0, pos, center, up](const QVariant &v) {
            float t = v.toFloat();
            Qt3DRender::QCamera *c = m_view->camera();
            c->setPosition(p0 + (pos - p0) * t);
            c->setViewCenter(c0 + (center - c0) * t);
            QVector3D u = u0 + (up - u0) * t;
            if (!u.isNull()) u.normalize();
            c->setUpVector(u);
        });
    m_camAnim->start();
}

void RobotViewer3D::setView(ViewPreset preset)
{
    // Mismo centro de interés que la cámara inicial (el robot está en el origen,
    // elevado ~80 en Y por la escala). Distancia similar a la vista por defecto.
    const QVector3D center(0.0f, 80.0f, 0.0f);
    const float     d = 450.0f;

    QVector3D dir;                    // dirección centro→cámara (se normaliza abajo)
    QVector3D up(0.0f, 1.0f, 0.0f);
    switch (preset) {
    case ViewPreset::Isometrica: dir = QVector3D( 1.0f, 0.8f,  1.0f); break; // 45° azimut, ~38° elevación
    case ViewPreset::Frente:     dir = QVector3D( 0.0f, 0.0f,  1.0f); break;
    case ViewPreset::Atras:      dir = QVector3D( 0.0f, 0.0f, -1.0f); break;
    case ViewPreset::Izquierda:  dir = QVector3D(-1.0f, 0.0f,  0.0f); break;
    case ViewPreset::Derecha:    dir = QVector3D( 1.0f, 0.0f,  0.0f); break;
    case ViewPreset::Arriba:
        dir = QVector3D(0.0f, 1.0f, 0.0f);
        up  = QVector3D(0.0f, 0.0f, -1.0f); // mirando derecho hacia abajo, el "arriba" de pantalla es -Z
        break;
    }

    goToView(center + dir.normalized() * d, center, up, true);
}
