#ifndef GTTESTWIDGET_H
#define GTTESTWIDGET_H

#include <QOpenGLWidget>
#include <qopenglfunctions_4_5_core>
#include "SharedGui/gt_decl.h"
#include "Shared/stack.h"

#include "GraphicsToolsModule/Objects/gtmaterialbase.h"

class ComputeNodeVolcanoRecognition;

class GtComputeGraph;
class GtComputeNodeBase;
class GtComputeNodeThreadSafe;
class GtFrameTexture;
class GtFramebufferObjectBase;
class GtCamera;
class GtMeshSurface;
class GtMeshCircle2D;
class GtViewBase;
class QOpenGLDebugLogger;
class GtShadowMapTechnique;
class InputFrameStream;
class QLabel;
struct Timer;

namespace cv {
    class Mat;
}

class GtWidget3D : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
    ScopedPointer<GtFramebufferObjectBase> fbo;
    ScopedPointer<GtViewBase> depth_view;
    ScopedPointer<GtViewBase> surface_view;
    ScopedPointer<GtViewBase> colored_view;
    ScopedPointer<GtMeshSurface> surface_mesh;
    ScopedPointer<GtMeshCircle2D> circle_mesh;
    ScopedPointer<class GtPlayerControllerBase> camera_controller;

    GtCamera* camera;

    QLabel* fps_board;
    QLabel* lft_board;
    QLabel* compute_board;
    ScopedPointer<struct FPSCounter> fps_counter;
    bool shadow_mapping;

    GtComputeNodeThreadSafe* output_node;
    ComputeNodeVolcanoRecognition* vulcans;
    ScopedPointer<GtFrameTextureResource> static_frame_texture;
    ScopedPointer<GtShadowMapTechniqueResource> shadow_map_technique;
    ScopedPointer<Matrix4Resource> MVP;
    ScopedPointer<Matrix4Resource> MVP_shadow;

    QOpenGLDebugLogger* logger;
public:
    GtWidget3D(QWidget *parent = 0);
    ~GtWidget3D();

    void setName(const QString& name);
    void setLogger(QOpenGLDebugLogger* logger);

    void setShadowMapTechnique(bool flag);

    void setVolcans(ComputeNodeVolcanoRecognition*);
    GtComputeNodeBase* getOutputNode() const;

    void setFpsBoard(QLabel* label) { fps_board = label; }
    void setLftBoard(QLabel* label) { lft_board = label; }
    void setCpsBoard(QLabel* label) { compute_board = label; }
    void setCamera(GtCamera* camera);

    // QOpenGLWidget interface
protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* e) Q_DECL_OVERRIDE;
};

#endif // TESTWIDGET_H
