#include "gtmeshquad2D.h"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

GtMeshQuad2D::GtMeshQuad2D(OpenGLFunctions* f)
{
    Initialize(f);
}

GtMeshQuad2D* GtMeshQuad2D::Instance(OpenGLFunctions* f)
{
    static GtMeshQuad2D res(f); return &res;
}

bool GtMeshQuad2D::buildMesh()
{
    TexturedVertex2F vertices[4] {
        Point2F(-1.f, -1.f), Point2F(0.f, 0.f),
        Point2F(-1.f, 1.f), Point2F(0.f, 1.f),
        Point2F(1.f, -1.f), Point2F(1.f, 0.f),
        Point2F(1.f, 1.f), Point2F(1.f, 1.f),
    };

    m_verticesCount = sizeof(vertices) / sizeof(TexturedVertex2F);

    m_vbo->bind();
    m_vbo->allocate(vertices, m_verticesCount * sizeof(TexturedVertex2F));
    m_vbo->release();

    return true;
}

void GtMeshQuad2D::bindVAO(OpenGLFunctions* f)
{
    m_vbo->bind();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0,2,GL_FLOAT,false,sizeof(TexturedVertex2F),nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1,2,GL_FLOAT,false,sizeof(TexturedVertex2F),(const void*)sizeof(Point2F));
}
