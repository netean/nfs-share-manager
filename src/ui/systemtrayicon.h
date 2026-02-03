#pragma once

#include <QSystemTrayIcon>

namespace NFSShareManager {

/**
 * @brief System tray icon class (placeholder for future implementation)
 */
class SystemTrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    explicit SystemTrayIcon(QObject *parent = nullptr);
};

} // namespace NFSShareManager