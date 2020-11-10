#include "gtshaderprogram.h"

#include <QDirIterator>
#include <QOpenGLShaderProgram>
#include "GraphicsToolsModule/gtrenderer.h"

void GtSharedShaderManager::Initialize(const QString& sharedShadersPath)
{
    Q_ASSERT(m_loadedShaders.isEmpty());
    QDirIterator it(sharedShadersPath, {"*.shader"});

    auto loadFile = [this](const QString& filePath, const QString& fileName) {
        QFile file(filePath);
        if(file.open(QFile::ReadOnly)) {
            m_loadedShaders.insert(Name(fileName), file.readAll());
        }
    };

    while (it.hasNext()) {
        it.next();
        auto filePath = it.filePath();
        auto fileName = it.fileName();
        loadFile(filePath, fileName);
        if(!filePath.startsWith(":")) {
            m_observer.AddFileObserver(filePath, [loadFile, filePath, fileName]{
                loadFile(filePath, fileName);
            });
        }
    }
}

const QByteArray& GtSharedShaderManager::Extract(const Name& fileName) const
{
    static QByteArray result;
    auto foundIt = m_loadedShaders.find(fileName);
    if(foundIt != m_loadedShaders.end()) {
        return foundIt.value();
    }
    return result;
}

QByteArray GtSharedShaderManager::Merge(const QByteArray& shader) const
{
    QByteArray result;
    QRegExp regExp(R"(#include \"([^\"]+)\")");
    qint32 pos = 0;
    qint32 lastPos = 0;
    while((pos = regExp.indexIn(shader, pos)) != -1) {
        result.append(shader.begin() + lastPos, pos - lastPos);
        result += Extract(Name(regExp.cap(1)));
        pos += regExp.matchedLength();
        lastPos = pos;
    }
    if(lastPos != 0) {
        result.append(shader.begin() + lastPos, std::distance(shader.begin(), shader.end()));
    } else {
        return shader;
    }
    return result;
}

GtSharedShaderManager& GtSharedShaderManager::GetInstance()
{
    static GtSharedShaderManager result;
    return result;
}

GtShaderProgram::GtShaderProgram(GtRenderer* renderer)
    : m_renderer(renderer)
    , m_isValid(false)
{

}

GtShaderProgram& GtShaderProgram::AddShader(ShaderType type, const QString& file)
{
    m_shaders.Append(new Shader(file, type));
    return *this;
}

void GtShaderProgram::SetShaders(const QString& path, const QString& vert_file, const QString& frag_file)
{
    m_shadersPath = path;
    AddShader(Vertex, vert_file);
    AddShader(Fragment, frag_file);
}

void GtShaderProgram::SetShaders(const QString& path, const QString& vertFile, const QString& geomFile, const QString& fragFile)
{
    m_shadersPath = path;
    AddShader(Vertex, vertFile);
    AddShader(Geometry, geomFile);
    AddShader(Fragment, fragFile);
}

QByteArray GtShaderProgram::extractShader(const QString& fileName) const
{
    QFile file(fileName);
    if(file.open(QFile::ReadOnly)) {
        auto shader = file.readAll();
        return GtSharedShaderManager::GetInstance().Merge(shader);
    }
    return QByteArray();
}

bool GtShaderProgram::Bind()
{
    m_mutex.lock();
    if(IsValid()) {
        m_shaderProgram->bind();
        return true;
    }
    return false;
}

void GtShaderProgram::Release()
{
    m_shaderProgram->release();
    m_mutex.unlock();
}

void GtShaderProgram::Update()
{
    QMutexLocker locker(&m_mutex);
    m_isValid = false;
    m_shaderProgram = new QOpenGLShaderProgram();
    {
        if(m_shadersPath == ":/") {
            for(Shader* shader : m_shaders) {
                QOpenGLShader* shaderObject = new QOpenGLShader((QOpenGLShader::ShaderTypeBit)shader->Type, m_shaderProgram.data());
                if(shaderObject->compileSourceCode(extractShader(m_shadersPath + shader->File))) {
                    m_shaderProgram->addShader(shaderObject);
                }
            }
        } else {
            DirBinder dir(m_shadersPath);
            m_shadersWatcher = new QtObserver(1000);
            for(Shader* shader : m_shaders) {
                QOpenGLShader* shader_object = new QOpenGLShader((QOpenGLShader::ShaderTypeBit)shader->Type, m_shaderProgram.data());
                m_shadersWatcher->AddFileObserver(m_shadersPath, shader->File, [this]{
                    m_renderer->Asynch([this]{
                        Update();
                    });
                });
                if(shader_object->compileSourceCode(extractShader(shader->File))) {
                    m_shaderProgram->addShader(shader_object);
                }
            }
        }
    }
    if(!m_shaderProgram->link()) {
        qCCritical(LC_SYSTEM) << "unable to link program" << m_shaderProgram->log();
    } else {
        m_isValid = true;
    }
    OnUpdated();
}
