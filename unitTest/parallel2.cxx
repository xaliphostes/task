/*
 * Copyright (c) 2024-now fmaerten@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <task/ParallelAlgorithm.h>
#include <task/Logger.h>

// Pour une utilisation plus avancée avec des jobs personnalisés
struct CustomJob
{
    int id;
    std::string name;
    std::vector<double> data;
};

class AdvancedParallelAlgorithm : public ParallelAlgorithm
{
public:
    void doJob(const std::any &job) override
    {
        try
        {
            const auto &customJob = std::any_cast<CustomJob>(job);

            emit("log", Args{
                            std::string("Processing custom job: ") + customJob.name});

            // Traitement des données
            double sum = 0;
            for (double value : customJob.data)
            {
                sum += value;
                // Vérifie si on doit arrêter
                if (stopRequested())
                {
                    emit("warn", Args{
                                     std::string("Job cancelled: ") + customJob.name});
                    return;
                }
            }

            emit("log", Args{
                            std::string("Job completed: ") + customJob.name +
                            ", Result: " + std::to_string(sum)});
        }
        catch (const std::bad_any_cast &e)
        {
            throw std::runtime_error("Invalid custom job format");
        }
    }
};

int main()
{
    // Utilisation avancée avec jobs personnalisés
    auto advancedAlgo = std::make_shared<AdvancedParallelAlgorithm>();

    auto logger = std::make_shared<Logger>();
    logger->connectAllSignalsTo(advancedAlgo.get());

    // Création d'un job personnalisé
    CustomJob job{
        .id = 1,
        .name = "Complex calculation",
        .data = {1.0, 2.0, 3.0, 4.0}};

    advancedAlgo->addJob(job);
    advancedAlgo->run();
}
