#include "main_window.h"
#include "jft_process.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QCloseEvent>
#include <QMessageBox>
#include <QApplication>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>

MainWindow::MainWindow(JftProcess *process, QWidget *parent)
    : QMainWindow(parent), m_process(process)
{
    // ── Header row ─────────────────────────────────────────
    auto *header       = new QWidget;
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 4, 8, 4);

    m_backButton = new QPushButton("← Back");
    m_backButton->setFixedWidth(80);
    connect(m_backButton, &QPushButton::clicked, this, [this]() {
        m_process->sendCommand("..\r");
    });

    m_homeButton = new QPushButton("⌂ Home");
    m_homeButton->setFixedWidth(80);
    connect(m_homeButton, &QPushButton::clicked, this, [this]() {
        m_process->sendCommand("h\r");
    });

    m_breadcrumb = new QLabel("Connecting…");

    headerLayout->addWidget(m_backButton);
    headerLayout->addWidget(m_homeButton);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_breadcrumb);

    // ── Scrollable button list ──────────────────────────────
    m_listWidget = new QWidget;
    m_listLayout = new QVBoxLayout(m_listWidget);
    m_listLayout->setAlignment(Qt::AlignTop);
    m_listLayout->setSpacing(4);
    m_listLayout->setContentsMargins(8, 8, 8, 8);

    auto *scroll = new QScrollArea;
    scroll->setWidget(m_listWidget);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // ── Central widget ──────────────────────────────────────
    auto *central       = new QWidget;
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(header);
    centralLayout->addWidget(scroll, 1);
    setCentralWidget(central);

    // ── Signal connections ──────────────────────────────────
    connect(process, &JftProcess::menuCleared,
            this, &MainWindow::onMenuCleared);
    connect(process, &JftProcess::menuTitleChanged,
            this, &MainWindow::onMenuTitleChanged);
    connect(process, &JftProcess::itemAdded,
            this, &MainWindow::onItemAdded);
    connect(process, &JftProcess::processExited,
            this, &MainWindow::onProcessExited);
    connect(process, &JftProcess::ynPromptReceived,
            this, &MainWindow::onYnPrompt);
    connect(process, &JftProcess::setupRequired,
            this, &MainWindow::onSetupRequired);
    connect(process, &JftProcess::loginFailed,
            this, &MainWindow::onLoginFailed);
    connect(process, &JftProcess::setupCredentialsInvalid,
            this, &MainWindow::onSetupCredentialsInvalid);

    resize(640, 600);
    setWindowTitle("jfqt - GUI for jftui");
}

void MainWindow::onMenuCleared() {
    while (QLayoutItem *item = m_listLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    m_backButton->setEnabled(true);
    m_homeButton->setEnabled(true);
    m_listWidget->setEnabled(true);
}

void MainWindow::onMenuTitleChanged(const QString &title) {
    m_breadcrumb->setText(title);
}

void MainWindow::onItemAdded(int index, const QString &name) {
    auto *btn = new QPushButton(name);
    btn->setMinimumHeight(36);
    btn->setStyleSheet("text-align: left; padding-left: 8px;");
    connect(btn, &QPushButton::clicked, this, [this, index]() {
        m_backButton->setEnabled(false);
        m_homeButton->setEnabled(false);
        m_listWidget->setEnabled(false);
        m_process->sendCommand(QString::number(index) + "\r");
    });
    m_listLayout->addWidget(btn);
}

void MainWindow::onProcessExited() {
    m_backButton->setEnabled(false);
    m_homeButton->setEnabled(false);
    m_breadcrumb->setText("jftui exited");
}

void MainWindow::onYnPrompt(const QString &question) {
    QMessageBox mb(this);
    mb.setWindowTitle("jftui");
    mb.setText(question);
    auto *resume  = mb.addButton("Resume",     QMessageBox::AcceptRole);
    auto *restart = mb.addButton("Start over", QMessageBox::DestructiveRole);
    if (question.contains("/c]", Qt::CaseInsensitive))
        mb.addButton(QMessageBox::Cancel);
    mb.exec();
    QAbstractButton *clicked = mb.clickedButton();
    if (clicked == resume)
        m_process->sendCommand("y\r");
    else if (clicked == restart)
        m_process->sendCommand("n\r");
    else
        m_process->sendCommand("c\r");
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // JftProcess destructor handles "q\r" + 500ms wait + SIGTERM
    event->accept();
}

void MainWindow::onSetupRequired() {
    if (findChild<QDialog *>()) return;

    auto *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("jftui — First-run Setup");

    auto *urlEdit  = new QLineEdit("https://");
    auto *sslCheck = new QCheckBox("Ignore SSL hostname validation");
    auto *userEdit = new QLineEdit;
    auto *passEdit = new QLineEdit;
    passEdit->setEchoMode(QLineEdit::Password);

    auto *form = new QFormLayout;
    form->addRow("Server URL:", urlEdit);
    form->addRow("",            sslCheck);
    form->addRow("Username:",   userEdit);
    form->addRow("Password:",   passEdit);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    auto *okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);
    connect(urlEdit, &QLineEdit::textChanged, okButton, [okButton, urlEdit]() {
        const QString text = urlEdit->text().trimmed();
        okButton->setEnabled(text.size() > 8 && text.contains('.'));
    });

    auto *layout = new QVBoxLayout(dlg);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(dlg, &QDialog::accepted, this,
            [this, urlEdit, sslCheck, userEdit, passEdit]() {
        m_process->beginSetup(urlEdit->text(), sslCheck->isChecked(),
                              userEdit->text(), passEdit->text());
    });
    connect(dlg, &QDialog::rejected, this, [this]() {
        m_process->sendCommand("n\r");
    });

    dlg->open();
}

void MainWindow::onLoginFailed() {
    QMessageBox mb(this);
    mb.setWindowTitle("jfqt");
    mb.setText("Login failed.");
    mb.addButton("Exit", QMessageBox::AcceptRole);
    mb.exec();
    QApplication::quit();
}

void MainWindow::onSetupCredentialsInvalid(const QString &prompt) {
    QDialog dlg(this);
    dlg.setWindowTitle("jftui — Login");

    auto *msgLabel = new QLabel(prompt);
    msgLabel->setWordWrap(true);

    auto *userEdit = new QLineEdit;
    auto *passEdit = new QLineEdit;
    passEdit->setEchoMode(QLineEdit::Password);

    auto *form = new QFormLayout;
    form->addRow(msgLabel);
    form->addRow("Username:", userEdit);
    form->addRow("Password:", passEdit);

    auto *buttons = new QDialogButtonBox;
    buttons->addButton("Try Again", QDialogButtonBox::AcceptRole);
    buttons->addButton("Cancel",    QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addWidget(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        m_process->retrySetup(userEdit->text(), passEdit->text());
    } else {
        m_process->sendCommand("n\r");
    }
}
