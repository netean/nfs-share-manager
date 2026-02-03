#include "test_helpers.h"
#include <QTemporaryDir>
#include <QTemporaryFile>

namespace NFSShareManager {
namespace TestUtils {

QList<QTemporaryDir*> TestHelpers::s_tempDirs;

QString TestHelpers::createTempDir()
{
    QTemporaryDir *tempDir = new QTemporaryDir();
    s_tempDirs.append(tempDir);
    return tempDir->path();
}

QString TestHelpers::createTempFile(const QString &content)
{
    QTemporaryFile *tempFile = new QTemporaryFile();
    tempFile->open();
    
    if (!content.isEmpty()) {
        tempFile->write(content.toUtf8());
    }
    
    QString path = tempFile->fileName();
    tempFile->close();
    return path;
}

void TestHelpers::cleanup()
{
    qDeleteAll(s_tempDirs);
    s_tempDirs.clear();
}

} // namespace TestUtils
} // namespace NFSShareManager