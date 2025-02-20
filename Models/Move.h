#pragma once
#include <stdlib.h>

typedef int8_t POS_T; // для хранения координат 8 бит, для  экономии памяти

// Хранение информации о ходе.
struct move_pos
{
    POS_T x, y;             // from, начальная позиция
    POS_T x2, y2;           // to, конечная позиция
    POS_T xb = -1, yb = -1; // beaten, позиция взятой фигуры (-1 по умолчанию)

    // Структура для хода без взятия фигуры.
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    // Структура для хода с взятием фигуры + координаты взятой фигуры.
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Сравнение объектов типа move_pos на равенство.
    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other);
    }
};
