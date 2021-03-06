#include "gtmeshsurface.h"
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "SharedGuiModule/decl.h"

GtMeshSurface::GtMeshSurface(qint32 width, qint32 height, qint32 sections)
    : GtMeshIndicesBase(GL_UNSIGNED_INT)
    , m_width(width)
    , m_height(height)
    , m_sections(sections)

{

}

GtMeshSurface::~GtMeshSurface()
{

}

bool GtMeshSurface::buildMesh()
{
    /*    <--w-->
     *   /_______\
     *  /|_s_|___|
     * h |___|___|
     *  \|___|___|
     *  s - count of sections at the line. The same count at the column, total sections count is s * s
     *  w - real width
     *  h - real height
    */
    qint32 sections_plus_one = m_sections + 1;
    qint32 sections_minus_one = m_sections - 1;

    m_verticesCount = pow(sections_plus_one, 2);
    qint32 indexes_without_degenerate_count = m_sections * (2 * m_sections + 2);
    m_indicesCount = indexes_without_degenerate_count + (2 * m_sections - 2);

    TexturedVertex2F* vertices = new TexturedVertex2F[m_verticesCount];
    qint32* indices = new qint32[m_indicesCount];

    //        QVector<SurfaceVertex> vp(vertices_count);
    //        QVector<qint32> vi(indices_count);
    //        vertices = vp.data();
    //        indices = vi.data();

    float h_step = float(m_width) / m_sections;
    float v_step = float(m_height) / m_sections;
    float tex_step = 1.f / m_sections;

    for(qint32 i(0); i < sections_plus_one; i++) {
        for(qint32 j(0); j < sections_plus_one; j++) {
            vertices[i * sections_plus_one + j] = TexturedVertex2F{ Point2F(h_step * j, v_step * i), Point2F(tex_step * j, 1.f - tex_step * i)};
        }
    }

    qint32* indexed_ptr = indices;

    for(qint32 j(0); j < sections_minus_one; j++) {
        for(qint32 i(0); i < sections_plus_one; i++) {
            *indexed_ptr++ = i + j * sections_plus_one;
            *indexed_ptr++ = i + (j + 1) * sections_plus_one;
        }
        *indexed_ptr++ = *(indexed_ptr - 1);
        *indexed_ptr++ = (j + 1) * sections_plus_one;
    }

    qint32 offset = sections_minus_one * sections_plus_one;
    for(qint32 i(0); i < sections_plus_one; i++) {
        *indexed_ptr++ = i + offset;
        *indexed_ptr++ = i + m_sections * sections_plus_one;
    }
    m_vbo->bind();
    m_vbo->allocate(vertices, m_verticesCount * sizeof(TexturedVertex2F));
    m_vbo->release();

    m_vboIndices->bind();
    m_vboIndices->allocate(indices, m_indicesCount * sizeof(qint32));
    m_vboIndices->release();

    delete [] vertices;
    delete [] indices;

    return true;
}

void GtMeshSurface::bindVAO(OpenGLFunctions* f)
{
    m_vbo->bind();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0,2,GL_FLOAT,false,sizeof(TexturedVertex2F),nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1,2,GL_FLOAT,false,sizeof(TexturedVertex2F),(const void*)sizeof(Point2F));
}
