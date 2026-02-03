#pragma once

#include <QString>
#include <QTemporaryDir>

namespace NFSShareManager {
namespace TestUtils {

/**
 * @brief Test helper utilities
 */
class TestHelpers
{
public:
    /**
     * @brief Create a temporary directory for testing
     * @return Path to the temporary directory
     */
    static QString createTempDir();

    /**
     * @brief Create a temporary file with content
     * @param content File content
     * @return Path to the temporary file
     */
    static QString createTempFile(const QString &content = QString());

    /**
     * @brief Clean up temporary resources
     */
    static void cleanup();

private:
    static QList<QTemporaryDir*> s_tempDirs;
};

} // namespace TestUtils
} // namespace NFSShareManager