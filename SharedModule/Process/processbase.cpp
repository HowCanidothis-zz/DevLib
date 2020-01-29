#include "processbase.h"

#include "processfactory.h"


ProcessBase::ProcessBase()
{

}

ProcessBase::~ProcessBase()
{

}

void ProcessBase::BeginProcess(const wchar_t* title, bool shadow)
{
    if(_processValue != nullptr) {
        _processValue->setNextProcessExpected();
    }
    _processValue = nullptr;
    _processValue.reset(shadow ? ProcessFactory::Instance().createShadowIndeterminate() : ProcessFactory::Instance().createIndeterminate());
    _processValue->init(title);
}

void ProcessBase::BeginProcess(const wchar_t* title, int stepsCount, int wantedCount, bool shadow)
{
    if(_processValue != nullptr) {
        _processValue->setNextProcessExpected();
    }
    if((stepsCount != 0) && (wantedCount != 0) && (stepsCount > wantedCount)) {
        _divider = stepsCount / wantedCount;
    } else {
        _divider = 0;
    }
    _processValue = nullptr;
    auto value = shadow ? ProcessFactory::Instance().createShadowDeterminate() : ProcessFactory::Instance().createDeterminate();
    value->init(title, stepsCount);
    _processValue.reset(value);
}

void ProcessBase::SetProcessTitle(const wchar_t* title)
{
    _processValue->setTitle(title);
}

void ProcessBase::IncreaseProcessStepsCount(int stepsCount)
{
    if(auto determinate = _processValue->AsDeterminate()) {
        determinate->increaseStepsCount(stepsCount);
    }
}

void ProcessBase::IncrementProcess()
{
    _processValue->incrementStep(_divider);
}

bool ProcessBase::IsProcessCanceled() const
{
    return _processValue->IsCanceled();
}
