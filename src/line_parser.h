#pragma once
#include <QString>
#include <variant>

struct MenuTitle { QString title; };
struct MenuItem  { int index; QString name; };
struct OtherLine {};
using ParsedLine = std::variant<MenuTitle, MenuItem, OtherLine>;

QString    stripAnsi(const QString &line);
ParsedLine parseLine(const QString &line);
