template <typename T>
void SignalSlot::connect(const std::string &signal, T *instance, void (T::*method)(const Args &))
{
    if (!hasSignal(signal))
    {
        std::cerr << "Signal '" << signal << "' not found" << std::endl;
        return;
    }

    auto func = [instance, method](const Args &args)
    {
        (instance->*method)(args);
    };

    auto connection = std::make_shared<FunctionConnection>(std::move(func));
    m_signals[signal].push_back(connection);
}

// Pour les méthodes membres sans arguments
template <typename T>
void SignalSlot::connect(const std::string &signal, T *instance, void (T::*method)())
{
    if (!hasSignal(signal))
    {
        std::cerr << "Signal '" << signal << "' not found" << std::endl;
        return;
    }

    auto func = [instance, method]()
    {
        (instance->*method)();
    };

    auto connection = std::make_shared<SimpleFunctionConnection>(std::move(func));
    m_signals[signal].push_back(connection);
}