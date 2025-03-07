class Switch : public Task {
public:
    // Type definitions for selector functions
    using StringSelectorFunction = std::function&lt;std::string(const ArgumentPack&)>;
    using IntSelectorFunction = std::function&lt;int(const ArgumentPack&)>;
    using SelectorFunction = std::variant&lt;StringSelectorFunction, IntSelectorFunction>;
    
    // Constructors
    explicit Switch(StringSelectorFunction selector);
    explicit Switch(IntSelectorFunction selector);
    
    // Case definition methods
    Switch& case_(const std::string& caseValue, Task* task);
    Switch& case_(int caseValue, Task* task);
    Switch& default_(Task* task);
    
    // Execution methods
    void execute(const ArgumentPack& args = {});
    std::future&lt;void> executeAsync(const ArgumentPack& args = {});
    
private:
    SelectorFunction m_selector;
    std::map&lt;std::string, Task*> m_stringCases;
    std::map&lt;int, Task*> m_intCases;
    Task* m_defaultTask = nullptr;
    bool m_isStringSelector;
    
    Task* findTaskToExecute(const ArgumentPack& args);
};