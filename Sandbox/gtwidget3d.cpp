#include "gtwidget3d.h"

#include <QMouseEvent>
#include <QTimer>
#include <QOpenGLTexture>
#include <QLabel>
#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>

#include "GraphicsToolsModule/gtmeshquad2D.h"
#include "GraphicsToolsModule/gtmeshcircle2D.h"
#include "GraphicsToolsModule/gtmeshsurface.h"
#include "GraphicsToolsModule/Objects/gtmaterial.h"
#include "GraphicsToolsModule/gttexture2D.h"
#include "GraphicsToolsModule/gtframebufferobject.h"
#include "GraphicsToolsModule/gtshadowmaptechnique.h"
#include "GraphicsToolsModule/gtframetexture.h"
#include "GraphicsToolsModule/gtplayercontrollercamera.h"
#include "GraphicsToolsModule/gtcamera.h"

#include "GraphicsToolsModule/Objects/gtmaterialparametermatrix.h"
#include "GraphicsToolsModule/Objects/gtmaterialparametershadow.h"
#include "GraphicsToolsModule/Objects/gtmaterialparametertexture.h"
#include "GraphicsToolsModule/Objects/gtmaterialparameterframetexture.h"

#include "ComputeGraphModule/computenodethreadsafe.h"
#include "ComputeGraphModule/computegraphbase.h"
#include "ComputeGraphModule/computenodevolcanorecognition.h"
#include "ComputeGraphModule/computegraphcore.h"
#include "ComputeGraphModule/inputframestream.h"

#include <opencv2/opencv.hpp>

#include "Shared/profile_utils.h"

GtWidget3D::GtWidget3D(QWidget *parent)
    : QOpenGLWidget(parent)
    , camera_controller(new GtPlayerControllerCamera)
    , camera(nullptr)
    , fps_board(nullptr)
    , lft_board(nullptr)
    , compute_board(nullptr)
    , fps_counter(new FPSCounter)
    , vulcans(nullptr)
    , logger(nullptr)
    , shadow_mapping(false)
{
    QSurfaceFormat surface_format;
//    surface_format.setSamples(4);
    this->setFormat(surface_format);

    QTimer* framer = new QTimer(this);
    connect(framer, SIGNAL(timeout()), this, SLOT(update()));
    framer->start(20);

    setFocusPolicy(Qt::ClickFocus);
    setFocus();
}

GtWidget3D::~GtWidget3D()
{

}

void GtWidget3D::setName(const QString& name)
{
    output_node = new GtComputeNodeThreadSafe(name);
}

void GtWidget3D::setLogger(QOpenGLDebugLogger* logger)
{
    this->logger = logger;

    QSurfaceFormat format = this->format();
    format.setOption(QSurfaceFormat::DebugContext);
    setFormat(format);
}

void GtWidget3D::setShadowMapTechnique(bool flag)
{
    shadow_mapping = flag;
}

void GtWidget3D::setVolcans(ComputeNodeVolcanoRecognition* volcans)
{
    this->vulcans = volcans;
}

GtComputeNodeBase* GtWidget3D::getOutputNode() const
{
    return output_node;
}

void GtWidget3D::setCamera(GtCamera* camera)
{
    this->camera = camera;
    reinterpret_cast<GtPlayerControllerCamera*>(this->camera_controller.data())->setCamera(camera);
}

void GtWidget3D::initializeGL()
{
    LOGOUT;
    if(!initializeOpenGLFunctions()) {
        log.error() << "initialize functions failed";
    }

    if(logger && logger->initialize()) {
        logger->startLogging();
    }
    ResourcesSystem::registerResource("output_texture", [this]{
         GtFrameTexture* result = new GtFrameTexture(this);
         result->createOutput();
         return result;
    });
    ResourcesSystem::registerResource("shadow_map_technique",[this]{
        GtShadowMapTechnique* result = new GtShadowMapTechnique(this, SizeI(1024,1024));
        result->create();
        return result;
    });
    ResourcesSystem::registerResource("sand_tex", [this]{
        GtTexture2D* result = new GtTexture2D(this);
        result->loadImage("sand2");
        return result;
    });
    ResourcesSystem::registerResource("grass_tex", [this]{
        GtTexture2D* result = new GtTexture2D(this);
        result->loadImage("grass2");
        return result;
    });
    ResourcesSystem::registerResource("mountain_tex", [this]{
        GtTexture2D* result = new GtTexture2D(this);
        result->loadImage("mountain2");
        return result;
    });
    ResourcesSystem::registerResource("mvp", [this]{
        return new Matrix4();
    });
    ResourcesSystem::registerResource("mvp_shadow", [this]{
        return new Matrix4();
    });

    static_frame_texture = ResourcesSystem::getResource<GtFrameTexture>("output_texture");
    shadow_map_technique = ResourcesSystem::getResource<GtShadowMapTechnique>("shadow_map_technique");
    MVP = ResourcesSystem::getResource<Matrix4>("mvp");
    MVP_shadow = ResourcesSystem::getResource<Matrix4>("mvp_shadow");

    GtMeshQuad2D::instance().initialize(this);

    surface_mesh = new GtMeshSurface(3000, 2400, 320);
    surface_mesh->initialize(this);

    surface_view = new GtMaterial();
    surface_view->addMesh(surface_mesh.data());

    surface_view->addParameter(new GtMaterialParameterTexture("SandTex", "sand_tex"));
    surface_view->addParameter(new GtMaterialParameterTexture("GrassTex", "grass_tex"));
    surface_view->addParameter(new GtMaterialParameterTexture("MountainTex", "mountain_tex"));
    if(shadow_mapping) {
        surface_view->addParameter(new GtMaterialParameterShadow("ShadowMap", "shadow_map_technique"));
    }

    surface_view->addParameter(new GtMaterialParameterMatrix("MVP", "mvp"));
    surface_view->addParameter(new GtMaterialParameterMatrix("ShadowMVP", "mvp_shadow"));
    surface_view->addParameter(new GtMaterialParameterFrameTexture("HeightMap", "output_texture"));
    surface_view->addParameter(new GtMaterialParameterBase("LightDirection", [this](QOpenGLShaderProgram* program, quint32 loc, OpenGLFunctions*) {
      program->setUniformValue(loc, shadow_map_technique->data()->getCam()->getForward().normalized());
    }));

    surface_view->setShaders("",GT_SHADERS_PATH "Depth/sensor.vert", GT_SHADERS_PATH "Depth/sensor.frag");

    static bool added = false;
    if(!added) {
        surface_view->mapProperties(Observer::instance());
        added = true;
    }


    if(shadow_mapping) {

        depth_view = new GtMaterial();
        depth_view->addMesh(&GtMeshQuad2D::instance());
        gTexID texture = shadow_map_technique->data()->getDepthTexture();
        depth_view->addParameter(new GtMaterialParameterBase("TextureMap", [texture](QOpenGLShaderProgram* program, quint32 loc, OpenGLFunctions* f) {
            GtTexture2D::bindTexture(f, 0, texture);
            program->setUniformValue(loc, 0);
        }));
        depth_view->setShaders(GT_SHADERS_PATH, "screen.vert", "screen.frag");
    }

    circle_mesh = new GtMeshCircle2D();
    circle_mesh->initialize(this);

    colored_view = new GtMaterial();
    colored_view->addMesh(circle_mesh.data());
    colored_view->addParameter(new GtMaterialParameterMatrix("MVP", "mvp"));
    colored_view->addParameter(new GtMaterialParameterBase("zValue", [](QOpenGLShaderProgram* program, quint32 loc, OpenGLFunctions*) {
        program->setUniformValue(loc, 400.0f);
    }));
    colored_view->setShaders(GT_SHADERS_PATH, "colored2d.vert", "colored.frag");

    glPointSize(10.f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

//    glEnable(GL_CULL_FACE);
//    glCullFace(GL_FRONT);

    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glEnable(GL_DEPTH_TEST);
}

void GtWidget3D::resizeGL(int w, int h)
{
    Q_ASSERT(camera);

    GtFramebufferFormat fbo_format;
    fbo_format.setDepthAttachment(GtFramebufferFormat::RenderBuffer);
    fbo_format.addColorAttachment(GtFramebufferTextureFormat(GL_TEXTURE_2D, GL_RGBA8));
    auto fbo = new GtFramebufferObjectMultisampled(this, {w,h}, 4);
    fbo->create(fbo_format);
    this->fbo.reset(fbo);

    camera->resize(w,h);
}

static float getXFromCircleCoordinate(float x, const SizeF& ratio)
{
    return x * ratio.height();
}

static float getYFromCircleCoordinate(float y, const SizeF& ratio, float width)
{
    return width - y * ratio.width();
}

void GtWidget3D::paintGL()
{
    camera_controller->inputHandle();

    if(true) {
        fps_counter->bind();
        {
            qint32 w, h;
            {
                MatGuard guard = output_node->getThreadOutput();
                static_frame_texture->data()->setInput(guard.getOutput());
                static_frame_texture->data()->update();
                w = guard.getOutput()->rows;
                h = guard.getOutput()->cols;
            }
            if(vulcans) {
                QMutexLocker locker(&vulcans->Mutex);
                SizeF ratio(float(surface_mesh->getHeight()) / w, float(surface_mesh->getWidth()) / h);

                circle_mesh->resize(vulcans->Circles.size());
                auto it_mesh = circle_mesh->begin();
                for(const cv::Vec3f& circle: vulcans->Circles) {
                    Circle2D* mesh_circle = *it_mesh;
                    float rx = getXFromCircleCoordinate(circle[0], ratio);
                    float ry = getYFromCircleCoordinate(circle[1], ratio, surface_mesh->getHeight());
                    mesh_circle->position = Point2F(rx, ry);
                    mesh_circle->color = Color3F(1.f, 0.f, 1.f);
                    mesh_circle->radius = Point2F(circle[2] * ratio.height(), circle[2] * ratio.width());
                    it_mesh++;
                }
                circle_mesh->update();
            }
        }
        if(shadow_mapping) {
            shadow_map_technique->data()->bind({-11232.8f, -57.2747f, 10584.6f},{1766.52f, 1309.02f, 0.f});
            MVP->get() = shadow_map_technique->data()->getWorld();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0,0,1024,1024);

            surface_view->draw(this);
            colored_view->draw(this);
            shadow_map_technique->data()->release();

            static Matrix4 bias_matrix(
            0.5f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.5f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 1.0f
            );
            Matrix4 shadow_MVP = bias_matrix * shadow_map_technique->data()->getWorld();

            fbo->bind();
            MVP->get() = camera->getWorld();
            MVP_shadow->get() = shadow_MVP;

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glViewport(0,0,fbo->getWidth(),fbo->getHeight());

//                depth_view->draw(this);
                surface_view->draw(this);
                colored_view->draw(this);
            fbo->release();
        }
        else {
            MVP->get() = camera->getWorld();

            fbo->bind();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glViewport(0,0,fbo->getWidth(),fbo->getHeight());

                surface_view->draw(this);
                colored_view->draw(this);
            fbo->release();
        }

        qint64 frame_time = fps_counter->release();
        if(lft_board) lft_board->setText(Timer::text("lft:", frame_time));
        if(fps_board) fps_board->setText("fps: " + QString::number(fps_counter->findMeanFPS(), 'f', 10));
        if(compute_board) compute_board->setText("cps: " + QString::number(ComputeGraphCore::instance()->getComputeTime(), 'f', 10));
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo->getID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebufferObject());
    glBlitFramebuffer(0,0,fbo->getWidth(),fbo->getHeight(),0,0,this->width(), this->height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void GtWidget3D::mouseMoveEvent(QMouseEvent* event)
{
    camera_controller->mouseMoveEvent(event);
}

void GtWidget3D::mousePressEvent(QMouseEvent* event)
{
    camera_controller->mousePressEvent(event);
}

void GtWidget3D::wheelEvent(QWheelEvent* event)
{
    camera_controller->wheelEvent(event);
}

void GtWidget3D::keyPressEvent(QKeyEvent *event)
{
    camera_controller->keyPressEvent(event);
}

void GtWidget3D::keyReleaseEvent(QKeyEvent *event)
{
    camera_controller->keyReleaseEvent(event);
}
