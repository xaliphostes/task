#include <task/concurrent/Runnable.h>

Runnable::Runnable()
{
}

Runnable::~Runnable()
{
}

void Runnable::run()
{
    emit("started");
    runImpl();
    emit("finished");
}