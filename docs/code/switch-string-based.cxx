// Create a string-based Switch
Switch commandSwitch([](const ArgumentPack& args) {
    return args.get<std::string>(0);  // First argument is the command
});

// Define tasks for each command
StartProcess startTask;
StopProcess stopTask;
RestartProcess restartTask;
ShowHelp helpTask;

// Add cases to the switch
commandSwitch.case_("start", &startTask)
             .case_("stop", &stopTask)
             .case_("restart", &restartTask)
             .default_(&helpTask);

// Connect to switch signals
commandSwitch.connect("started", []() {
    std::cout << "Command processing started" << std::endl;
});

commandSwitch.connect("caseSelected", [](const ArgumentPack& args) {
    std::string command = args.get<std::string>(0);
    std::cout << "Executing command: " << command << std::endl;
});

// Execute with a command
commandSwitch.execute(ArgumentPack("restart"));  // Will execute restartTask