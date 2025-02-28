template <typename T, typename... Args>
std::shared_ptr<T> TaskQueue::createAndEnqueue(TaskPriority priority,
                                               const std::string &description,
                                               Args &&...args) {
    static_assert(std::is_base_of<Runnable, T>::value,
                  "T must be derived from Runnable");

    auto task = std::make_shared<T>(std::forward<Args>(args)...);
    if (enqueue(task, priority, description)) {
        return task;
    }
    return nullptr;
}
