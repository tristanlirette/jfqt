#include "jft_process.h"
#include "line_parser.h"
#include <QSocketNotifier>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <string>

static QString debugBytes(const QByteArray &data) {
    QString out;
    for (unsigned char c : data) {
        if      (c == '\r')  out += "\\r";
        else if (c == '\n')  out += "\\n";
        else if (c == '\x1b') out += "\\x1b";
        else if (c >= 0x20 && c < 0x7f) out += QChar(c);
        else out += QString("\\x%1").arg(c, 2, 16, QChar('0'));
    }
    return out;
}

static int s_sigchldPipe[2] = {-1, -1};

static void sigchldHandler(int) {
    if (s_sigchldPipe[1] < 0) return;
    char c = 1;
    ::write(s_sigchldPipe[1], &c, 1);
}

JftProcess::JftProcess(QObject *parent) : QObject(parent) {}

JftProcess::~JftProcess() {
    if (m_masterFd >= 0) {
        const char quit[] = "q\r";
        ::write(m_masterFd, quit, sizeof(quit) - 1);
    }
    if (m_pid > 0) {
        struct timespec ts{0, 500'000'000};   // 500 ms
        ::nanosleep(&ts, nullptr);
        ::kill(m_pid, SIGTERM);
        ::waitpid(m_pid, nullptr, 0);
    }
    if (m_masterFd >= 0)    ::close(m_masterFd);
    if (s_sigchldPipe[0] >= 0) { ::close(s_sigchldPipe[0]); s_sigchldPipe[0] = -1; }
    if (s_sigchldPipe[1] >= 0) { ::close(s_sigchldPipe[1]); s_sigchldPipe[1] = -1; }
}

void JftProcess::setupSigchldPipe() {
    if (s_sigchldPipe[0] >= 0) return;   // already initialized
    ::pipe2(s_sigchldPipe, O_CLOEXEC | O_NONBLOCK);
    struct sigaction sa{};
    sa.sa_handler = sigchldHandler;
    sa.sa_flags   = SA_RESTART | SA_NOCLDSTOP;
    ::sigaction(SIGCHLD, &sa, nullptr);

    m_sigchldNotifier = new QSocketNotifier(
        s_sigchldPipe[0], QSocketNotifier::Read, this);
    connect(m_sigchldNotifier, &QSocketNotifier::activated,
            this, &JftProcess::onChildExited);
}

void JftProcess::start(const QString &jftuiPath) {
    m_jftuiPath = jftuiPath;
    m_lineBuf.clear();
    m_restartAfterExit = false;
    m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (m_masterFd < 0) return;
    ::grantpt(m_masterFd);
    ::unlockpt(m_masterFd);
    // Copy slave path — ptsname() returns a pointer to static storage
    const std::string slavePath = ::ptsname(m_masterFd);

    // Set a non-zero window size so linenoise uses TIOCGWINSZ instead of
    // falling back to the \x1b[6n cursor-position query, which would block
    // waiting for a response we never send.
    struct winsize ws{};
    ws.ws_row = 24;
    ws.ws_col = 80;
    ::ioctl(m_masterFd, TIOCSWINSZ, &ws);

    setupSigchldPipe();

    m_pid = ::fork();
    if (m_pid < 0) return;

    if (m_pid == 0) {
        // ── child process ────────────────────────────────────
        ::setsid();
        int slaveFd = ::open(slavePath.c_str(), O_RDWR);
        ::ioctl(slaveFd, TIOCSCTTY, 0);
        ::dup2(slaveFd, STDIN_FILENO);
        ::dup2(slaveFd, STDOUT_FILENO);
        ::dup2(slaveFd, STDERR_FILENO);
        int maxFd = (int)::sysconf(_SC_OPEN_MAX);
        if (maxFd < 0) maxFd = 1024;
        for (int i = 3; i < maxFd; ++i) ::close(i);
        QByteArray path = jftuiPath.toLocal8Bit();
        char *argv[] = {path.data(), nullptr};
        ::execvp(path.constData(), argv);
        ::_exit(1);
    }

    // ── parent process ──────────────────────────────────────
    m_readNotifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated,
            this, &JftProcess::onDataReady);
}

void JftProcess::retrySetup(const QString &username, const QString &password) {
    m_setupQueue.enqueue(username + "\r");
    m_setupQueue.enqueue(password + "\r");
    sendCommand("y\r");
}

void JftProcess::beginSetup(const QString &url, bool ignoreSsl,
                             const QString &username, const QString &password) {
    m_setupQueue.enqueue(url + "\r");
    m_setupQueue.enqueue(ignoreSsl ? "y\r" : "n\r");
    m_setupQueue.enqueue(username + "\r");
    m_setupQueue.enqueue(password + "\r");
    sendCommand("y\r");   // answer "Would you like to configure?"
}

void JftProcess::sendCommand(const QString &cmd) {
    if (m_masterFd < 0) return;
    QByteArray data = cmd.toUtf8();
    qDebug().noquote() << "[send]" << debugBytes(data);
    ssize_t written = ::write(m_masterFd, data.constData(), data.size());
    if (written < 0)
        qDebug() << "[send] write error:" << ::strerror(errno);
}

void JftProcess::onDataReady() {
    char buf[4096];
    ssize_t n = ::read(m_masterFd, buf, sizeof(buf));
    if (n <= 0) return;
    qDebug().noquote() << "[recv]" << debugBytes(QByteArray(buf, n));
    m_lineBuf.append(buf, (int)n);
    int pos;
    while ((pos = m_lineBuf.indexOf('\n')) >= 0) {
        QByteArray line = m_lineBuf.left(pos);
        m_lineBuf.remove(0, pos + 1);
        processLine(line);
    }
    // jftui's input prompt ("> ") never ends with \n; detect and process it
    // immediately so promptReady() fires without waiting for the next command.
    if (::stripAnsi(QString::fromUtf8(m_lineBuf)).trimmed() == ">") {
        processLine(m_lineBuf);
        m_lineBuf.clear();
    }
}

void JftProcess::processLine(const QByteArray &raw) {
    auto result = ::parseLine(QString::fromUtf8(raw));
    if (auto *title = std::get_if<MenuTitle>(&result)) {
        qDebug() << "[line] title:" << title->title;
        emit menuTitleChanged(title->title);
        emit menuCleared();
    } else if (auto *item = std::get_if<MenuItem>(&result)) {
        qDebug() << "[line] item" << item->index << item->name;
        emit itemAdded(item->index, item->name);
    } else {
        qDebug().noquote() << "[line] other:" << debugBytes(raw);
        QString text = ::stripAnsi(QString::fromUtf8(raw)).trimmed();
        if (text == ">") {
            emit promptReady();
        } else if (text.contains("could not login", Qt::CaseInsensitive)) {
            m_setupQueue.clear();
            emit loginFailed();
        } else if (text.contains("configure jftui", Qt::CaseInsensitive)) {
            emit setupRequired();
        } else if (!m_setupQueue.isEmpty() && text.contains("Please enter the encoded URL", Qt::CaseInsensitive)) {
            sendCommand(m_setupQueue.dequeue());
        } else if (!m_setupQueue.isEmpty() && text.contains("ignore hostname validation", Qt::CaseInsensitive)) {
            sendCommand(m_setupQueue.dequeue());
        } else if (!m_setupQueue.isEmpty() && text.contains("Please enter your username", Qt::CaseInsensitive)) {
            sendCommand(m_setupQueue.dequeue());
        } else if (!m_setupQueue.isEmpty() && text.contains("Please enter your password", Qt::CaseInsensitive)) {
            sendCommand(m_setupQueue.dequeue());
        } else if (text.contains("Would you like to try again", Qt::CaseInsensitive)) {
            emit setupCredentialsInvalid(text);
        } else if (text.contains("Login successful", Qt::CaseInsensitive)) {
            m_restartAfterExit = true;
        } else if (text.contains("[y/n", Qt::CaseInsensitive)) {
            emit ynPromptReceived(text);
        }
    }
}

void JftProcess::onChildExited() {
    char c;
    ::read(s_sigchldPipe[0], &c, 1);
    int status;
    if (::waitpid(m_pid, &status, WNOHANG) > 0) {
        m_pid = -1;
        m_readNotifier->setEnabled(false);
        // Drain any bytes still buffered in the PTY before evaluating the restart flag.
        // "Login successful" may arrive at the same time as SIGCHLD; POSIX guarantees
        // the data remains readable after child exit.
        // Set non-blocking so the drain loop exits immediately when buffer is empty
        int fdFlags = ::fcntl(m_masterFd, F_GETFL);
        ::fcntl(m_masterFd, F_SETFL, fdFlags | O_NONBLOCK);
        char buf[4096];
        ssize_t n;
        while ((n = ::read(m_masterFd, buf, sizeof(buf))) > 0) {
            qDebug().noquote() << "[recv]" << debugBytes(QByteArray(buf, n));
            m_lineBuf.append(buf, (int)n);
            int pos;
            while ((pos = m_lineBuf.indexOf('\n')) >= 0) {
                QByteArray line = m_lineBuf.left(pos);
                m_lineBuf.remove(0, pos + 1);
                processLine(line);
            }
        }
        if (!m_lineBuf.isEmpty()) {
            processLine(m_lineBuf);
            m_lineBuf.clear();
        }
        if (m_restartAfterExit) {
            m_restartAfterExit = false;
            ::close(m_masterFd);
            m_masterFd = -1;
            delete m_readNotifier;
            m_readNotifier = nullptr;
            start(m_jftuiPath);
            return;
        }
        emit processExited();
    }
}
