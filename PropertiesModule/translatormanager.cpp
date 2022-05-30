#include "translatormanager.h"

TranslatorManager::TranslatorManager()
{
    OnLanguageChanged += { this, [this]{
        m_names.clear();
    } };
}

TranslatorManager& TranslatorManager::GetInstance()
{
    static TranslatorManager result;
    return result;
}

TranslatedString::TranslatedString(const FTranslationHandler& translationHandler)
    : Super(translationHandler())
    , m_translationHandler(translationHandler)
{
    TranslatorManager::GetInstance().OnLanguageChanged.Connect(this, [this, translationHandler]{
        retranslate();
    }).MakeSafe(m_connections);

    Retranslate += { this, [this]{ retranslate(); }};
}

TranslatedString::TranslatedString(const FTranslationHandler& translationHandler, const QVector<Dispatcher*>& retranslators)
    : TranslatedString(translationHandler)
{
    for(auto* retranslator : retranslators) {
        Retranslate.ConnectFrom(CONNECTION_DEBUG_LOCATION, *retranslator).MakeSafe(m_connections);
    }
}

void TranslatedString::SetTranslationHandler(const FTranslationHandler& handler)
{
    m_translationHandler = handler;
    retranslate();
}

void TranslatedString::retranslate()
{
    m_retranslate.Call([this]{
        SetValue(m_translationHandler());
    });
}
