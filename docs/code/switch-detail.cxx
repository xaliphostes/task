Task* Switch::findTaskToExecute(const ArgumentPack& args) {
    try {
        if (m_isStringSelector) {
            std::string selectedCase = std::get&lt;StringSelectorFunction>(m_selector)(args);
            
            auto it = m_stringCases.find(selectedCase);
            if (it != m_stringCases.end()) {
                emit("caseSelected", ArgumentPack(selectedCase));
                return it->second;
            }
        } else {
            int selectedCase = std::get&lt;IntSelectorFunction>(m_selector)(args);
                       
            auto it = m_intCases.find(selectedCase);
            if (it != m_intCases.end()) {
                emit("caseSelected", ArgumentPack(selectedCase));
                return it->second;
            }
        }
        
        // No matching case found, use default
        if (m_defaultTask) {
            emit("defaultSelected");
            return m_defaultTask;
        }
        
        // No default task either
        emit("noMatchFound");
        emitString("warn", "No matching case or default task found");
        return nullptr;
    } catch (const std::exception& e) {
        emitString("error", "Exception in selector function: " + std::string(e.what()));
        return nullptr;
    }
}