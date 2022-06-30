#include "processbase.h"

#include "processfactory.h"

ProcessBase::ProcessBase()
    : m_interruptor(nullptr)
    , m_silentIfOneStep(false)
{

}

ProcessBase::~ProcessBase()
{
    if(m_interruptor != nullptr) {
        m_interruptor->OnInterrupted -= this;
    }
}

void ProcessBase::SetInterruptor(const Interruptor& interruptor)
{
    Q_ASSERT(m_interruptor == nullptr);
    m_interruptor = ::make_scoped<Interruptor>(interruptor);
    m_interruptor->OnInterrupted += { this, [this]{
        m_processValue->finish();
    }};
}

const QString& ProcessBase::GetTitle() const
{
    return m_processValue->GetTitle();
}

void ProcessBase::BeginProcess(const ProcessBaseIndeterminateParams& params)
{
    m_processValue = nullptr;
    m_processValue.reset(params.Shadow ? ProcessFactory::Instance().createShadowIndeterminate() : ProcessFactory::Instance().createIndeterminate());
    m_processValue->init(m_interruptor.get(), params.Title);
}

void ProcessBase::BeginProcess(const ProcessBaseDeterminateParams& params)
{
    auto stepsCount = params.StepsCount;
    auto wantedCount = params.WantedCount;

    if((stepsCount != 0) && (wantedCount != 0) && (stepsCount > wantedCount)) {
        m_divider = stepsCount / wantedCount;
    } else {
        m_divider = 0;
    }
    m_processValue = nullptr;
    auto value = params.Shadow ? ProcessFactory::Instance().createShadowDeterminate() : ProcessFactory::Instance().createDeterminate();
    value->SetDummy(m_silentIfOneStep && m_divider < 2);
    value->init(m_interruptor.get(), params.Title, stepsCount);
    m_processValue.reset(value);
}

void ProcessBase::SetProcessTitle(const QString& title)
{
    m_processValue->setTitle(title);
}

void ProcessBase::IncreaseProcessStepsCount(int stepsCount)
{
    if(auto determinate = m_processValue->AsDeterminate()) {
        determinate->increaseStepsCount(stepsCount);
    }
}

void ProcessBase::IncrementProcess()
{
    m_processValue->incrementStep(m_divider);
}

bool ProcessBase::IsProcessCanceled() const
{
    return m_processValue->IsFinished();
}
