// Create an integer-based Switch
Switch stateSwitch([](const ArgumentPack& args) {
    return args.get&lt;int>(0);  // First argument is the state
});

// Define tasks for each state
InitializeSystem initTask;
NormalOperation runningTask;
ErrorHandling errorTask;
SystemShutdown shutdownTask;

// Add cases to the switch
stateSwitch.case_(0, &initTask)        // INIT state
           .case_(1, &runningTask)     // RUNNING state
           .case_(2, &errorTask)       // ERROR state 
           .case_(3, &shutdownTask)    // SHUTDOWN state
           .default_(&errorTask);      // Default to error handling

// Execute asynchronously
auto future = stateSwitch.executeAsync(ArgumentPack(1)); // RUNNING state

// Do other work while task executes...

// Wait for completion if needed
future.wait();