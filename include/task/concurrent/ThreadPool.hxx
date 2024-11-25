
template <typename T, typename... Args>
T *ThreadPool::createAndAdd(Args &&...args)
{
    auto runnable = std::make_unique<T>(std::forward<Args>(args)...);
    T *ptr = runnable.get();
    add(std::move(runnable));
    return ptr;
}
