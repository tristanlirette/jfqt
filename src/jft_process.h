#pragma once
#include <QObject>
#include <QString>
#include <QQueue>

class QSocketNotifier;

class JftProcess : public QObject {
    Q_OBJECT
public:
    explicit JftProcess(QObject *parent = nullptr);
    ~JftProcess();
    void start(const QString &jftuiPath);
    void beginSetup(const QString &url, bool ignoreSsl,
                    const QString &username, const QString &password);
    void retrySetup(const QString &username, const QString &password);
public slots:
    void sendCommand(const QString &cmd);
signals:
    void menuCleared();
    void menuTitleChanged(const QString &title);
    void itemAdded(int index, const QString &name);
    void processExited();
    void ynPromptReceived(const QString &question);
    void setupRequired();
    void loginFailed();
    void setupCredentialsInvalid(const QString &prompt);
    void promptReady();
private slots:
    void onDataReady();
    void onChildExited();
private:
    void setupSigchldPipe();
    void processLine(const QByteArray &line);
    int              m_masterFd         = -1;
    int              m_pid              = -1;
    QString          m_jftuiPath;
    bool             m_restartAfterExit = false;
    QQueue<QString>  m_setupQueue;
    QSocketNotifier *m_readNotifier     = nullptr;
    QSocketNotifier *m_sigchldNotifier  = nullptr;
    QByteArray       m_lineBuf;
};
