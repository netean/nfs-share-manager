#pragma once

#include "sharecreatedialog.h"
#include "../core/nfsshare.h"

namespace NFSShareManager {

/**
 * @brief Dialog for editing existing NFS share configurations
 * 
 * This dialog extends ShareCreateDialog to provide editing capabilities
 * for existing NFS shares, with the path field disabled and pre-populated
 * with the current share configuration.
 */
class ShareConfigDialog : public ShareCreateDialog
{
    Q_OBJECT

public:
    explicit ShareConfigDialog(const NFSShare &share, ShareManager *shareManager, QWidget *parent = nullptr);
    ~ShareConfigDialog();

    /**
     * @brief Get the original share being edited
     * @return The original NFSShare object
     */
    const NFSShare &originalShare() const;

    /**
     * @brief Check if the configuration has been modified
     * @return True if any settings have been changed
     */
    bool isModified() const;

public slots:
    /**
     * @brief Accept the dialog and update the share
     */
    void accept() override;

private slots:
    /**
     * @brief Handle configuration changes to track modifications
     */
    void onConfigurationChanged();

private:
    /**
     * @brief Load the existing share configuration into the UI
     */
    void loadShareConfiguration();

    /**
     * @brief Set up the dialog for editing mode
     */
    void setupEditMode();

    /**
     * @brief Connect signals for change tracking
     */
    void connectChangeTracking();

    NFSShare m_originalShare;    ///< The original share being edited
    bool m_isModified;          ///< Track if configuration has been modified
};

} // namespace NFSShareManager