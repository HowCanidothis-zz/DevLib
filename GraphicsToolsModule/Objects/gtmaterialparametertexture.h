#ifndef GTMATERIALTEXTURE_H
#define GTMATERIALTEXTURE_H

#include "gtmaterialparametertexturebase.h"

class GtMaterialParameterTexture : public GtMaterialParameterTextureBase
{
    typedef GtMaterialParameterTextureBase Super;
    SharedPointer<GtTextureResource> m_texture;
public:
    GtMaterialParameterTexture(const QString& m_name, const Name& m_resource);

    void MapProperties(QtObserver* observer) Q_DECL_OVERRIDE;
    // GtObjectBase interface
private:
    virtual FDelegate apply() Q_DECL_OVERRIDE;
};

#endif // GTMATERIALTEXTURE_H
