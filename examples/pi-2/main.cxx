
#include <task/concurrent/Runnable.h>
#include <task/concurrent/ThreadPool.h>
#include <random>
#include <atomic>
#include <iomanip>


class PiWorker : public Runnable {
public:
    PiWorker(size_t points, int seed, std::atomic<double>& result)
        : m_points(points)
        , m_seed(seed)
        , m_result(result)
    {}

    void runImpl() override {
        std::mt19937 gen(m_seed);
        std::uniform_real_distribution<> dis(-1.0, 1.0);
        
        size_t insideCircle = 0;
        
        for (size_t i = 0; i < m_points; ++i) {
            double x = dis(gen);
            double y = dis(gen);
            
            if (x*x + y*y <= 1.0) {
                ++insideCircle;
            }
        }
        
        // Ajoute le résultat partiel au résultat global
        m_result.store(m_result.load() + static_cast<double>(insideCircle));
    }

private:
    size_t m_points;
    int m_seed;
    std::atomic<double>& m_result;
};

class PiCalculator {
public:
    double calculate(size_t totalPoints = 1000000) {
        std::atomic<double> result{0.0};
        ThreadPool pool;

        // Détermine le nombre de points par thread
        size_t numThreads = pool.maxThreadCount();
        std::cout << "Using " << numThreads << " cores" << std::endl ;
        size_t pointsPerThread = totalPoints / numThreads;
        
        // Crée et ajoute les workers au pool
        for (size_t i = 0; i < numThreads; ++i) {
            size_t points = (i == numThreads-1) 
                ? pointsPerThread + (totalPoints % numThreads)  // Ajoute le reste au dernier thread
                : pointsPerThread;
                
            pool.createAndAdd<PiWorker>(points, i, result);
        }

        // Lance les calculs
        pool.run();

        // Calcule le pi final
        return (result / static_cast<double>(totalPoints)) * 4.0;
    }
};

// Exemple d'utilisation
int main() {
    PiCalculator calc;
    
    std::cout << "Calculating Pi with 10M points..." << std::endl;
    double pi = calc.calculate(10000000000);
    std::cout << "π ≈ " << std::setprecision(15) << pi << std::endl;
    std::cout << "Real π = 3.141592653589793..." << std::endl;
}