#pragma once

#include <QtTest/QtTest>
#include <QRandomGenerator>
#include <functional>

namespace NFSShareManager {
namespace PropertyTesting {

/**
 * @brief Base class for property-based tests
 * 
 * This class provides infrastructure for running property-based tests
 * with randomized inputs and configurable iteration counts.
 */
class PropertyTestBase : public QObject
{
    Q_OBJECT

public:
    explicit PropertyTestBase(QObject *parent = nullptr);

protected:
    /**
     * @brief Run a property test with the specified number of iterations
     * @param property The property function to test
     * @param iterations Number of test iterations (default: 100)
     * @param description Description of the property being tested
     */
    void runProperty(std::function<bool()> property, 
                    int iterations = 100, 
                    const QString &description = QString());

    /**
     * @brief Run a property test that may generate counterexamples
     * @param property The property function to test
     * @param iterations Number of test iterations
     * @param description Description of the property being tested
     * @param onCounterexample Callback for when a counterexample is found
     */
    void runPropertyWithCounterexample(std::function<bool()> property,
                                      int iterations,
                                      const QString &description,
                                      std::function<void()> onCounterexample);

    /**
     * @brief Get the random generator instance
     * @return Reference to the random generator
     */
    QRandomGenerator &generator();

    /**
     * @brief Set the seed for reproducible test runs
     * @param seed The seed value to use
     */
    void setSeed(quint32 seed);

private:
    QRandomGenerator &m_generator;
    int m_currentIteration;
    QString m_currentDescription;
};

/**
 * @brief Macro for defining property tests
 * 
 * Usage: PROPERTY_TEST(testName, iterations, description) { ... }
 */
#define PROPERTY_TEST(name, iterations, description) \
    void name() { \
        runProperty([this]() -> bool { \
            return name##_impl(); \
        }, iterations, description); \
    } \
    bool name##_impl()

/**
 * @brief Macro for property tests with counterexample handling
 */
#define PROPERTY_TEST_WITH_COUNTEREXAMPLE(name, iterations, description, counterexample_handler) \
    void name() { \
        runPropertyWithCounterexample([this]() -> bool { \
            return name##_impl(); \
        }, iterations, description, [this]() { \
            counterexample_handler(); \
        }); \
    } \
    bool name##_impl()

} // namespace PropertyTesting
} // namespace NFSShareManager