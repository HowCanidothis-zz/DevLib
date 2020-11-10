#ifndef GRAPHICSTOOLSMODULE_DECL_H
#define GRAPHICSTOOLSMODULE_DECL_H

#include <SharedModule/internal.hpp>

struct GtControllersContext
{
    class GtCamera* Camera;
    class GtRenderer* Renderer;
    class QOpenGLFramebufferObject* FrameBuffer;
    class GtDepthBuffer* DepthBuffer;
};

using GtShaderProgramPtr = SharedPointer<class GtShaderProgram>;
using GtMeshPtr = SharedPointer<class GtMesh>;
using GtMeshBufferPtr = SharedPointer<class GtMeshBuffer>;
using GtRendererPtr = SharedPointer<class GtRenderer>;
using GtRendererControllerPtr = SharedPointer<class GtRendererController>;
using GtFontPtr = SharedPointer<class GtFont>;

#endif // DECL_H
