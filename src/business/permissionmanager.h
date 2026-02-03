#pragma once
#include <QObject>
namespace NFSShareManager {
class PermissionManager : public QObject {
    Q_OBJECT
public:
    explicit PermissionManager(QObject *parent = nullptr);
};
}