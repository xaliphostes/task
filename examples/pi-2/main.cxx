#include <atomic>
#include <iomanip>
#include <random>
#include <task/Chronometer.h>
#include <task/Logger.h>
#include <task/concurrent/Runnable.h>
#include <task/concurrent/ThreadPool.h>

class PiWorker : public Runnable {
  public:
    PiWorker(size_t points, int seed, std::atomic<double> &result)
        : m_points(points), m_seed(seed), m_result(result) {
        // Create a progress signal if not already created by the parent class
        if (!hasSignal("progress_update")) {
            createSignal("progress_update");
        }
    }

  protected:
    void runImpl() override {
        std::mt19937 gen(m_seed);
        std::uniform_real_distribution<> dis(-1.0, 1.0);

        size_t insideCircle = 0;
        size_t progressStep = m_points / 10; // Report progress every 10%

        for (size_t i = 0; i < m_points; ++i) {
            // Check if stop was requested
            if (stopRequested()) {
                emit("warn",
                     ArgumentPack("Worker calculation stopped by request"));
                return;
            }

            double x = dis(gen);
            double y = dis(gen);

            if (x * x + y * y <= 1.0) {
                ++insideCircle;
            }

            // Report progress periodically
            if (progressStep > 0 && i % progressStep == 0) {
                float progress =
                    static_cast<float>(i) / static_cast<float>(m_points);
                reportProgress(progress);

                // Also emit detailed progress info
                emit("progress_update", ArgumentPack(i, m_points, progress));
            }
        }

        // Ajoute le résultat partiel au résultat global
        m_result.store(m_result.load() + static_cast<double>(insideCircle));

        // Report completion
        emit("log", ArgumentPack("Worker completed with " +
                                 std::to_string(insideCircle) +
                                 " points inside circle out of " +
                                 std::to_string(m_points)));
    }

  private:
    size_t m_points;
    int m_seed;
    std::atomic<double> &m_result;
};

class PiCalculator : public Task {
  public:
    PiCalculator() {
        // Create signals for the calculator
        createSignal("calculation_started");
        createSignal("calculation_finished");
        createSignal("result");
    }

    double calculate(size_t totalPoints = 1000000) {
        // Create thread pool
        ThreadPool pool;
        std::atomic<double> result{0.0};

        // Connect pool signals to this calculator for forwarding
        pool.connect("log",
                     [this](const ArgumentPack &args) { emit("log", args); });

        pool.connect("warn",
                     [this](const ArgumentPack &args) { emit("warn", args); });

        pool.connect("error",
                     [this](const ArgumentPack &args) { emit("error", args); });

        // Détermine le nombre de points par thread
        size_t numThreads = pool.maxThreadCount();

        emit("log",
             ArgumentPack("Using " + std::to_string(numThreads) + " cores"));

        size_t pointsPerThread = totalPoints / numThreads;

        // Crée et ajoute les workers au pool
        for (size_t i = 0; i < numThreads; ++i) {
            size_t points =
                (i == numThreads - 1)
                    ? pointsPerThread +
                          (totalPoints %
                           numThreads) // Ajoute le reste au dernier thread
                    : pointsPerThread;

            auto worker = pool.createAndAdd<PiWorker>(points, i, result);

            // Connect worker signals to this calculator
            worker->connect(
                "log", [this](const ArgumentPack &args) { emit("log", args); });

            worker->connect("progress_update", [this, i, numThreads](
                                                   const ArgumentPack &args) {
                // Forward detailed progress per worker
                emit("worker_progress",
                     ArgumentPack(i, numThreads, args.get<size_t>(0),
                                  args.get<size_t>(1), args.get<float>(2)));
            });
        }

        // Signal start
        emit("calculation_started");

        // Lance les calculs
        auto future = pool.run();

        // Attend la fin des calculs
        future.wait();

        // Calcule le pi final
        double pi = (result / static_cast<double>(totalPoints)) * 4.0;

        emit("result", ArgumentPack(pi));

        // Signal completion
        emit("calculation_finished");

        return pi;
    }
};

// Exemple d'utilisation
int main() {
    auto logger = std::make_shared<Logger>("π:");
    auto chrono = std::make_shared<Chronometer>();
    auto calculator = std::make_shared<PiCalculator>();
    u_int64_t nbPts = 1e10;

    // Connect logger to calculator
    logger->connectAllSignalsTo(calculator.get());

    // Connect chronometer to calculator events
    calculator->connect("calculation_started", [chrono]() { chrono->start(); });

    calculator->connect("calculation_finished", [chrono]() {
        double elapsed = chrono->stop() / 1000.0; // Conversion en secondes
        std::cout << "Calculation took " << elapsed << " seconds" << std::endl;
    });

    // Connect to receive the result
    double pi_result = 0.0;
    calculator->connect("result", [&pi_result](const ArgumentPack &args) {
        pi_result = args.get<double>(0);
    });

    std::cout << "Calculating Pi with " << nbPts << " points..." << std::endl;

    // Do the calculation
    double pi = calculator->calculate(nbPts);

    // Affichage du résultat avec une précision élevée
    std::cout << std::setprecision(15) << "π ≈ " << pi << std::endl;
    std::cout << "Real π = 3.141592653589793..." << std::endl;
}