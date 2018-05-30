#ifndef CONTROLLERSSYSTEM_H
#define CONTROLLERSSYSTEM_H

#include <Shared/name.h>

class ControllerBase;

class ControllersSystem
{
    ControllersSystem();
public:    
    static ControllerBase* GetController(const Name& name);

private:
    friend class ControllerBase;

    static void registerController(const Name& name, ControllerBase* controller);
};

#endif // CONTROLLERSSYSTEM_H
