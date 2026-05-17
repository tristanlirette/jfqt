#include <QtTest>
#include "line_parser.h"

class TestLineParser : public QObject {
    Q_OBJECT
private slots:
    void stripAnsi_removesColorCodes();
    void stripAnsi_leavesPlainText();
    void parseLine_menuTitle();
    void parseLine_menuTitle_withWhitespace();
    void parseLine_directoryItem();
    void parseLine_videoItem_withYear();
    void parseLine_audioItem();
    void parseLine_ignoresPrompt();
    void parseLine_ignoresErrorLine();
    void parseLine_stripsAnsiThenMatches();
};

void TestLineParser::stripAnsi_removesColorCodes() {
    QCOMPARE(stripAnsi("\x1b[32mhello\x1b[0m"), QString("hello"));
}
void TestLineParser::stripAnsi_leavesPlainText() {
    QCOMPARE(stripAnsi("D 1: Movies"), QString("D 1: Movies"));
}
void TestLineParser::parseLine_menuTitle() {
    auto r = parseLine("===== Home =====");
    QVERIFY(std::holds_alternative<MenuTitle>(r));
    QCOMPARE(std::get<MenuTitle>(r).title, QString("Home"));
}
void TestLineParser::parseLine_menuTitle_withWhitespace() {
    auto r = parseLine("=====  User Views  =====");
    QVERIFY(std::holds_alternative<MenuTitle>(r));
    QCOMPARE(std::get<MenuTitle>(r).title, QString("User Views"));
}
void TestLineParser::parseLine_directoryItem() {
    auto r = parseLine("D 3: TV Shows");
    QVERIFY(std::holds_alternative<MenuItem>(r));
    QCOMPARE(std::get<MenuItem>(r).index, 3);
    QCOMPARE(std::get<MenuItem>(r).name,  QString("TV Shows"));
}
void TestLineParser::parseLine_videoItem_withYear() {
    auto r = parseLine("V 7: Inception (2010)");
    QVERIFY(std::holds_alternative<MenuItem>(r));
    QCOMPARE(std::get<MenuItem>(r).index, 7);
    QCOMPARE(std::get<MenuItem>(r).name,  QString("Inception (2010)"));
}
void TestLineParser::parseLine_audioItem() {
    auto r = parseLine("T 2: 01 - Bohemian Rhapsody");
    QVERIFY(std::holds_alternative<MenuItem>(r));
    QCOMPARE(std::get<MenuItem>(r).index, 2);
    QCOMPARE(std::get<MenuItem>(r).name,  QString("01 - Bohemian Rhapsody"));
}
void TestLineParser::parseLine_ignoresPrompt() {
    QVERIFY(std::holds_alternative<OtherLine>(parseLine("> ")));
}
void TestLineParser::parseLine_ignoresErrorLine() {
    QVERIFY(std::holds_alternative<OtherLine>(parseLine("Error: cannot open folder.")));
}
void TestLineParser::parseLine_stripsAnsiThenMatches() {
    auto r = parseLine("\x1b[1mD 5: Music\x1b[0m");
    QVERIFY(std::holds_alternative<MenuItem>(r));
    QCOMPARE(std::get<MenuItem>(r).index, 5);
    QCOMPARE(std::get<MenuItem>(r).name,  QString("Music"));
}

QTEST_MAIN(TestLineParser)
#include "test_line_parser.moc"
