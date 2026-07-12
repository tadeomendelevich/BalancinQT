#ifndef ROBOTVIEWER3D_H
#define ROBOTVIEWER3D_H

#include <QWidget>
#include <QVector3D>

class QLabel;
class QTimer;
class QVariantAnimation;

namespace Qt3DCore   { class QTransform; class QEntity; }
namespace Qt3DExtras { class Qt3DWindow; class QPhongMaterial; class QPhongAlphaMaterial; }

class RobotViewer3D : public QWidget
{
    Q_OBJECT
public:
    // Vistas predefinidas estilo CAD (Fusion 360): isométrica 45° y ortogonales.
    enum class ViewPreset { Isometrica, Frente, Atras, Izquierda, Derecha, Arriba };

    explicit RobotViewer3D(QWidget *parent = nullptr);

    // Solo balanceo (conserva el rumbo actual) — lo usan los paths viejos (log 10Hz, GET_ANGLE).
    void setPitch(float degrees);
    // Balanceo + rumbo + banking lateral (paquete de odometría): el modelo se
    // mueve en los TRES ejes como el robot real (latDeg opcional, default 0).
    void setPose(float pitchDeg, float yawDeg, float latDeg = 0.0f);
    // Obstáculo visto por los sensores de objeto (ADC 5/6/8): pared roja frente al
    // modelo. Solo se muestra con roll entre -90° y +30° (la condición vive acá,
    // evaluada sobre la pose MOSTRADA para que acompañe la interpolación).
    void setFrontObstacle(bool present, float distMeters);
    void setView(ViewPreset preset);

private:
    void loadModelBounds(const QString &objPath);     // bbox del mesh leído del .obj (síncrono)
    void applyModelTransform();                       // pose mostrada → transform + tinte + HUD
    void animatePoseTo(float pitchDeg, float yawDeg,
                       float latDeg);                 // interpola la pose mostrada hacia el objetivo
    void goToView(const QVector3D &pos, const QVector3D &center,
                  const QVector3D &up, bool animated = true);

    Qt3DExtras::Qt3DWindow     *m_view;
    Qt3DCore::QTransform       *m_robotTransform;
    Qt3DCore::QTransform       *m_baseTransform;
    Qt3DExtras::QPhongMaterial *m_robotMaterial = nullptr;
    QVector3D                   m_modelCenter;   // centro geométrico del mesh (pivote de la rotación)
    QVector3D                   m_modelMin;      // bounding box del mesh (coords locales) —
    QVector3D                   m_modelMax;      // para apoyar el modelo en el piso en toda pose

    // Obstáculo frente al robot (pared roja semitransparente)
    Qt3DCore::QEntity               *m_obstacleEntity    = nullptr;
    Qt3DCore::QTransform            *m_obstacleTransform = nullptr;
    Qt3DExtras::QPhongAlphaMaterial *m_obstacleMaterial  = nullptr;
    bool  m_obsPresent = false;
    float m_obsDistM   = 0.36f;   // distancia estimada (m), mapeo crudo ADC→m del llamador

    // Pose objetivo (última recibida) y pose mostrada (interpolada, para que el
    // dato de 2 Hz no se vea a saltos).
    float m_pitchDeg  = 0.0f;
    float m_yawDeg    = 0.0f;
    float m_latDeg    = 0.0f;   // banking lateral (tercer eje)
    float m_dispPitch = 0.0f;
    float m_dispYaw   = 0.0f;
    float m_dispLat   = 0.0f;

    QVariantAnimation *m_poseAnim   = nullptr;   // interpolación de la pose del modelo
    QVariantAnimation *m_camAnim    = nullptr;   // transición animada entre vistas
    QTimer            *m_orbitTimer = nullptr;   // órbita automática (turntable)
    QLabel            *m_hudLabel   = nullptr;   // Roll / Rumbo numéricos en la barra
};

#endif // ROBOTVIEWER3D_H
