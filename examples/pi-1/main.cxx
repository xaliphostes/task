#include <random>
#include <thread>
#include <iomanip>
#include <task/ParallelAlgorithm.h>
#include <task/Chronometer.h>
#include <task/Logger.h>

// Structure pour les paramètres de chaque job
struct PiJobParams
{
    size_t points; // Nombre de points à calculer par ce job
    int seed;      // Seed pour le générateur aléatoire
};

class PiCalculator : public ParallelAlgorithm
{
public:
    PiCalculator(size_t totalPoints = 1000000)
        : m_totalPoints(totalPoints), m_result(0.0)
    {
    }

    double getResult() const
    {
        return m_result;
    }

protected:
    // Implémentation du job individuel
    void doJob(const std::any &job) override
    {
        try
        {
            const auto &params = std::any_cast<PiJobParams>(job);
            double localSum = calculatePiPortion(params);

            // Mise à jour thread-safe du résultat
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_result.store(m_result.load() + localSum);

                // Émission du progrès
                m_completedPoints += params.points;
                double progress = (static_cast<double>(m_completedPoints) / m_totalPoints) * 100;

                emit("log", std::vector<std::any>{
                                std::string("Progress: ") +
                                std::to_string(static_cast<int>(progress)) + "%"});
            }
        }
        catch (const std::bad_any_cast &e)
        {
            emit("error", std::vector<std::any>{
                              std::string("Invalid job format: ") + e.what()});
        }
    }

    void exec(const std::vector<std::any> &args = {}) override
    {
        if (!m_isSetup) {
            setupJobs();
            m_isSetup = true;
        }

        m_result = 0.0;
        m_completedPoints = 0;

        // Exécution des jobs en parallèle via ParallelAlgorithm
        ParallelAlgorithm::exec(args);

        // Calcul final de pi
        m_result = m_result / static_cast<double>(m_totalPoints) * 4.0;

        emit("log", std::vector<std::any>{
                        std::string("Final π value: ") +
                        std::to_string(m_result)});
    }

private:
    double calculatePiPortion(const PiJobParams &params)
    {
        std::mt19937 gen(params.seed);
        std::uniform_real_distribution<> dis(-1.0, 1.0);

        size_t insideCircle = 0;

        for (size_t i = 0; i < params.points; ++i)
        {
            if (stopRequested())
            {
                emit("warn", std::vector<std::any>{
                                 std::string("Calculation stopped by user")});
                return 0.0;
            }

            double x = dis(gen);
            double y = dis(gen);

            if (x * x + y * y <= 1.0)
            {
                ++insideCircle;
            }
        }

        return static_cast<double>(insideCircle);
    }

    void setupJobs()
    {
        // Détermine le nombre de jobs basé sur le nombre de cœurs disponibles
        size_t numCores = std::thread::hardware_concurrency();
        size_t pointsPerJob = m_totalPoints / numCores;

        emit("log", std::vector<std::any>{
                        std::string("Using ") + std::to_string(numCores) + " cores"});

        // Crée un job pour chaque cœur
        for (size_t i = 0; i < numCores; ++i)
        {
            PiJobParams params{
                pointsPerJob,
                static_cast<int>(i) // Utilise i comme seed
            };
            addJob(params);
        }

        // Gère les points restants si la division n'est pas exacte
        size_t remainingPoints = m_totalPoints % numCores;
        if (remainingPoints > 0)
        {
            PiJobParams params{
                remainingPoints,
                static_cast<int>(numCores)};
            addJob(params);
        }
    }

    const size_t m_totalPoints;
    std::atomic<size_t> m_completedPoints{0};
    std::atomic<double> m_result{0.0};
    std::mutex m_mutex;
    bool m_isSetup{false};
};

// Exemple d'utilisation
int main()
{
    // Création des composants
    auto logger = std::make_shared<Logger>("π:");
    auto chrono = std::make_shared<Chronometer>();
    auto piCalc = std::make_shared<PiCalculator>(10000000000); // 10 billion of points

    // Configuration du logging
    logger->connectAllSignalsTo(piCalc.get());

    // Configuration du chronomètre
    piCalc->connect("started", [chrono]() { chrono->start(); });
    piCalc->connect("finished", [chrono]() {
        double elapsed = chrono->stop() / 1000.0;  // Conversion en secondes
        std::cout << "Calculation took " << elapsed << " seconds" << std::endl;
    });

    // Lancement du calcul
    auto future = piCalc->run();

    // Si on veut arrêter le calcul prématurément
    // piCalc->stop();

    // Attente de la fin du calcul
    future.wait();

    // Affichage du résultat avec une précision élevée
    std::cout << std::setprecision(15) << "π ≈ " << piCalc->getResult() << std::endl;
    std::cout << "Real π = 3.141592653589793..." << std::endl;
}
