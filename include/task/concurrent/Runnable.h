#pragma once
#include "../Task.h"

class Runnable : public Task
{
public:
    Runnable();
    virtual ~Runnable();
    void run();

protected:
    virtual void runImpl() = 0;
};
