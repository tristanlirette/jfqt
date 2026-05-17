#include "line_parser.h"
#include <QRegularExpression>

QString stripAnsi(const QString &line) {
    static const QRegularExpression ansi(QStringLiteral("\x1b\\[[0-9;]*[a-zA-Z]"));
    return QString(line).remove(ansi);
}

ParsedLine parseLine(const QString &raw) {
    const QString line = stripAnsi(raw).trimmed();

    static const QRegularExpression titleRe(
        QStringLiteral("^={5}\\s+(.+?)\\s+={5}$"));
    static const QRegularExpression itemRe(
        QStringLiteral("^[DVT] (\\d+): (.+)$"));

    auto m = titleRe.match(line);
    if (m.hasMatch())
        return MenuTitle{m.captured(1)};

    m = itemRe.match(line);
    if (m.hasMatch())
        return MenuItem{m.captured(1).toInt(), m.captured(2)};

    return OtherLine{};
}
