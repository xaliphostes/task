// Create a simple state machine using Switch
class StateMachine {
    public:
        enum State { INIT, IDLE, PROCESSING, ERROR, SHUTDOWN };
        
        StateMachine() {
            m_stateSwitch = std::make_unique<Switch>([this](const ArgumentPack&) {
                return m_currentState;
            });
            
            // Define state tasks
            m_initTask = std::make_unique<InitTask>();
            m_idleTask = std::make_unique<IdleTask>();
            m_processingTask = std::make_unique<ProcessingTask>();
            m_errorTask = std::make_unique<ErrorTask>();
            m_shutdownTask = std::make_unique<ShutdownTask>();
            
            // Register state handlers
            m_stateSwitch->case_(INIT, m_initTask.get())
                         ->case_(IDLE, m_idleTask.get())
                         ->case_(PROCESSING, m_processingTask.get())
                         ->case_(ERROR, m_errorTask.get())
                         ->case_(SHUTDOWN, m_shutdownTask.get())
                         ->default_(m_errorTask.get());
                         
            // Connect state change signal
            m_initTask->connectSimple("finished", [this]() { 
                setState(IDLE); 
            });
            
            m_idleTask->connectData("newData", [this](const ArgumentPack&) {
                setState(PROCESSING);
            });
            
            m_processingTask->connectSimple("finished", [this]() {
                setState(IDLE);
            });
            
            m_processingTask->connectSimple("error", [this]() {
                setState(ERROR);
            });
        }
        
        void setState(State newState) {
            m_currentState = newState;
            m_stateSwitch->execute();
        }
        
        void start() {
            setState(INIT);
        }
        
    private:
        State m_currentState = INIT;
        std::unique_ptr<Switch> m_stateSwitch;
        
        // State tasks
        std::unique_ptr<Task> m_initTask;
        std::unique_ptr<Task> m_idleTask;
        std::unique_ptr<Task> m_processingTask;
        std::unique_ptr<Task> m_errorTask;
        std::unique_ptr<Task> m_shutdownTask;
    };