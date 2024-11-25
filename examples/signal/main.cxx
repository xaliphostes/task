#include <iostream>
#include <task/SignalSlot.h>

class Counter : public SignalSlot {
public:
    Counter() : m_value(0) {
        createSignal("valueChanged");
        createSignal("paramsChanged");
    }

    // Getter
    int getValue() const { return m_value; }

    // Setter (slot)
    void setValue(const Args& args) {
        if (args.empty()) return;
        
        try {
            int value = std::any_cast<int>(args[0]);
            if (value != m_value) {
                m_value = value;
                emit("valueChanged", Args{m_value});
            }
        }
        catch(const std::bad_any_cast&) {
            std::cerr << "Invalid value type in setValue" << std::endl;
        }
    }

    // Slot method
    void showDouble(const Args& args) {
        std::cout << "  2*" << m_value << " = " << (2 * m_value) << std::endl;
    }

    // Slot method
    void showTriple(const Args& args) {
        std::cout << "  3*" << m_value << " = " << (3 * m_value) << std::endl;
    }

    // Slot with multiple parameters
    void setParams(const Args& args) {
        if (args.size() < 3) {
            std::cerr << "setParams needs 3 arguments" << std::endl;
            return;
        }

        try {
            int a = std::any_cast<int>(args[0]);
            int b = std::any_cast<int>(args[1]);
            int c = std::any_cast<int>(args[2]);
            std::cout << "method with 3 params: (a=" << a 
                      << ", b=" << b << ", c=" << c << ")" << std::endl;
        }
        catch(const std::bad_any_cast&) {
            std::cerr << "Invalid parameter type in setParams" << std::endl;
        }
    }

private:
    int m_value;
};

void printSeparator() {
    std::cout << "---------------------------" << std::endl;
}

/*
===============
Should display:
===============

---------------------------
Value of a changed to 12
Value of b changed to 12
  2*12 = 24
  3*12 = 36
---------------------------
Value of b changed to 48
  2*48 = 96
  3*48 = 144
---------------------------
(should not trigger anything)
---------------------------
method with 3 params: (a=1, b=2, c=3) 
 */
int main() {
    auto a = std::make_shared<Counter>();
    auto b = std::make_shared<Counter>();

    // Connexions
    a->connect("valueChanged", [](const Args& args) {
        if (!args.empty()) {
            try {
                int value = std::any_cast<int>(args[0]);
                std::cout << "Value of a changed to " << value << std::endl;
            }
            catch(const std::bad_any_cast&) {
                std::cerr << "Invalid value type in lambda" << std::endl;
            }
        }
    });

    // Connexion de a vers b
    a->connect("valueChanged", [b](const Args& args) {
        b->setValue(args);
    });

    // Connexion des slots de b
    b->connect("valueChanged", [](const Args& args) {
        if (!args.empty()) {
            try {
                int value = std::any_cast<int>(args[0]);
                std::cout << "Value of b changed to " << value << std::endl;
            }
            catch(const std::bad_any_cast&) {
                std::cerr << "Invalid value type in lambda" << std::endl;
            }
        }
    });

    b->connect("valueChanged", b.get(), &Counter::showDouble);
    b->connect("valueChanged", b.get(), &Counter::showTriple);
    b->connect("paramsChanged", b.get(), &Counter::setParams);

    // Tests
    printSeparator();
    a->emit("valueChanged", Args{12});

    printSeparator();
    b->emit("valueChanged", Args{48});

    printSeparator();
    std::cout << "(should not trigger anything)" << std::endl;
    a->emit("valueChanged", Args{12}); // Même valeur, ne devrait rien déclencher

    printSeparator();
    b->emit("paramsChanged", Args{1, 2, 3});

    printSeparator();
    return 0;
}
