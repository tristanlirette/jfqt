#pragma once
#include <QMainWindow>
#include <QStringList>
class QToolButton;
class QMenu;
class QVBoxLayout;
class QHBoxLayout;
class QCloseEvent;
class JftProcess;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(JftProcess *process, QWidget *parent = nullptr);
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void onMenuCleared();
    void onMenuTitleChanged(const QString &title);
    void onItemAdded(int index, const QString &name);
    void onProcessExited();
    void onYnPrompt(const QString &question);
    void onSetupRequired();
    void onLoginFailed();
    void onSetupCredentialsInvalid(const QString &prompt);
    void onPromptReady();
private:
    void resetFilters();
    void rebuildBreadcrumb();
    JftProcess  *m_process;
    QToolButton *m_filterButton;
    QMenu       *m_filterMenu;
    QWidget     *m_breadcrumbWidget;
    QHBoxLayout *m_breadcrumbLayout;
    QWidget     *m_listWidget;
    QVBoxLayout *m_listLayout;
    QStringList  m_titleStack;
    bool         m_awaitingPush  = false;
    int          m_pendingBacks  = 0;
};
