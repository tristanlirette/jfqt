#pragma once
#include <QMainWindow>
class QPushButton;
class QLabel;
class QVBoxLayout;
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
private:
    JftProcess  *m_process;
    QPushButton *m_backButton;
    QLabel      *m_breadcrumb;
    QWidget     *m_listWidget;
    QVBoxLayout *m_listLayout;
};
