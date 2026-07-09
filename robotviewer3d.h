#ifndef ROBOTVIEWER3D_H
#define ROBOTVIEWER3D_H

#include <QWidget>
#include <QVector3D>

class QLabel;
class QTimer;
class QVariantAnimation;

namespace Qt3DCore   { class QTransform; }
namespace Qt3DExtras { class Qt3DWindow; class QPhongMaterial; }

class RobotViewer3D : public QWidget
{
    Q_OBJECT
public:
    // Vistas predefinidas estilo CAD (Fusion 360): isométrica 45° y ortogonales.
    enum class ViewPreset { Isometrica, Frente, Atras, Izquierda, Derecha, Arriba };

    explicit RobotViewer3D(QWidget *parent = nullptr);

    // Solo balanceo (conserva el rumbo actual) — lo usan los paths viejos (log 10Hz, GET_ANGLE).
    void setPitch(float degrees);
    // Balanceo + rumbo (paquete de odometría): el modelo también gira como el robot real.
    void setPose(float pitchDeg, float yawDeg);
    void setView(ViewPreset preset);

private:
    void applyModelTransform();                       // pose mostrada → transform + tinte + HUD
    void animatePoseTo(float pitchDeg, float yawDeg); // interpola la pose mostrada hacia el objetivo
    void goToView(const QVector3D &pos, const QVector3D &center,
                  const QVector3D &up, bool animated = true);

    Qt3DExtras::Qt3DWindow     *m_view;
    Qt3DCore::QTransform       *m_robotTransform;
    Qt3DCore::QTransform       *m_baseTransform;
    Qt3DExtras::QPhongMaterial *m_robotMaterial = nullptr;
    QVector3D                   m_modelCenter;   // centro geométrico del mesh (pivote de la rotación)

    // Pose objetivo (última recibida) y pose mostrada (interpolada, para que el
    // dato de 2 Hz no se vea a saltos).
    float m_pitchDeg  = 0.0f;
    float m_yawDeg    = 0.0f;
    float m_dispPitch = 0.0f;
    float m_dispYaw   = 0.0f;

    QVariantAnimation *m_poseAnim   = nullptr;   // interpolación de la pose del modelo
    QVariantAnimation *m_camAnim    = nullptr;   // transición animada entre vistas
    QTimer            *m_orbitTimer = nullptr;   // órbita automática (turntable)
    QLabel            *m_hudLabel   = nullptr;   // Roll / Rumbo numéricos en la barra
};

#endif // ROBOTVIEWER3D_H
