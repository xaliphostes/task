// Execute switch in a background thread
std::future<void> future = switchTask.executeAsync(args);

// Do other work while switch evaluates and executes

// Wait for completion when needed
future.wait();