#include "property_test_base.h"
#include <QDebug>

namespace NFSShareManager {
namespace PropertyTesting {

PropertyTestBase::PropertyTestBase(QObject *parent)
    : QObject(parent)
    , m_generator(*QRandomGenerator::global())
    , m_currentIteration(0)
{
}

void PropertyTestBase::runProperty(std::function<bool()> property, 
                                  int iterations, 
                                  const QString &description)
{
    m_currentDescription = description;
    
    for (int i = 0; i < iterations; ++i) {
        m_currentIteration = i + 1;
        
        if (!property()) {
            QString message = QString("Property failed on iteration %1").arg(m_currentIteration);
            if (!description.isEmpty()) {
                message += QString(": %1").arg(description);
            }
            QFAIL(qPrintable(message));
        }
    }
    
    // Property passed all iterations
    if (!description.isEmpty()) {
        qDebug() << QString("Property passed %1 iterations: %2").arg(iterations).arg(description);
    }
}

void PropertyTestBase::runPropertyWithCounterexample(std::function<bool()> property,
                                                    int iterations,
                                                    const QString &description,
                                                    std::function<void()> onCounterexample)
{
    m_currentDescription = description;
    
    for (int i = 0; i < iterations; ++i) {
        m_currentIteration = i + 1;
        
        if (!property()) {
            // Call counterexample handler to capture the failing case
            onCounterexample();
            
            QString message = QString("Property failed on iteration %1").arg(m_currentIteration);
            if (!description.isEmpty()) {
                message += QString(": %1").arg(description);
            }
            QFAIL(qPrintable(message));
        }
    }
    
    // Property passed all iterations
    if (!description.isEmpty()) {
        qDebug() << QString("Property passed %1 iterations: %2").arg(iterations).arg(description);
    }
}

QRandomGenerator &PropertyTestBase::generator()
{
    return m_generator;
}

void PropertyTestBase::setSeed(quint32 seed)
{
    m_generator.seed(seed);
}

} // namespace PropertyTesting
} // namespace NFSShareManager