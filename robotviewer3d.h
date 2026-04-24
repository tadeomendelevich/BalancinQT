#ifndef ROBOTVIEWER3D_H
#define ROBOTVIEWER3D_H

#include <QWidget>

namespace Qt3DCore  { class QTransform; }
namespace Qt3DExtras { class Qt3DWindow; }

class RobotViewer3D : public QWidget
{
    Q_OBJECT
public:
    explicit RobotViewer3D(QWidget *parent = nullptr);
    void setPitch(float degrees);

private:
    Qt3DExtras::Qt3DWindow *m_view;
    Qt3DCore::QTransform   *m_robotTransform;
};

#endif // ROBOTVIEWER3D_H
