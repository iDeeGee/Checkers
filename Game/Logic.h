#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color) // Вектор для хранения возможных ходов
    {
        next_move.clear();
        next_best_state.clear();

        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        vector<move_pos> res;
        int state = 0;

        do {
            res.push_back(next_move[state]);
            state = next_best_state[state];
        } while (state != -1 && next_move[state].x != -1);

        return res;
        
    }

private:
    // Функция для выполнения хода на доске (без изменения оригинальной доски)
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) // Если ход включает взятие фигуры
            mtx[turn.xb][turn.yb] = 0; // Удаляем взятую фигуру

        // Превращение в дамку, если фигура дошла до последней линии
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2; // Превращаем фигуру в дамку
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещаем фигуру на новую позицию
        mtx[turn.x][turn.y] = 0; // Очищаем старую позицию
        return mtx; // Возвращаем новое состояние доски
    }

    // Функция для вычисления оценки текущего состояния доски
    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const
    {
        // Счетчики для белых и черных фигур и дамок
        double w = 0, wq = 0, b = 0, bq = 0; 
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // Считаем белые фигуры
                wq += (mtx[i][j] == 3); // Считаем белые дамки
                b += (mtx[i][j] == 2); // Считаем черные фигуры
                bq += (mtx[i][j] == 4); // Считаем черные дамки
                if (scoring_mode == "NumberAndPotential") // Если используется "NumberAndPotential"
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // Учитываем потенциал белых фигур
                    b += 0.05 * (mtx[i][j] == 2) * (i); // Учитываем потенциал черных фигур
                }
            }
        }
        if (!first_bot_color) // Если бот играет за черных, меняем местами счетчики
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0) // Если белых фигур не осталось, возвращаем максимальное значение
            return INF;
        if (b + bq == 0) // Если черных фигур не осталось, возвращаем минимальное значение
            return 0;
        int q_coef = 4; // Коэффициент для дамок
        if (scoring_mode == "NumberAndPotential") // Если используется "NumberAndPotential"
        {
            q_coef = 5; // Увеличиваем коэффициент для дамок
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // Возвращаем значение
    }

    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
                                double alpha = -1)
    {
        next_move.emplace_back(-1, -1, -1, -1);
        next_best_state.push_back(-1);
        if (state != 0) {
            find_turns(x, y, mtx);
        }
        
        auto now_turns = turns;
        auto now_have_beats = have_beats;

        if (!now_have_beats && state != 0) {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }
        double best_score = -1;
        for (auto turn : now_turns) {
            size_t new_state = next_move.size();
            double score;
            if (now_have_beats) {
                find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);
            } 
            else {
                find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            // Нашли лучшую оптиму
            if (score > best_score) {
                best_score = score;
                next_move[state] = turn;
                next_best_state[state] = (now_have_beats ? new_state : -1);
            }
        }

        return best_score;
        
        // now_have_beats && state == 0;
        // !now_have_beats && state == 0;
        // now_have_beats && state != 0;
        // !now_have_beats && state != 0;

    }

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
                               double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        if (depth == Max_depth) {
            return calc_score(mtx, (depth % 2 == color));
        }

        if (x != -1) {
            find_turns(x, y, mtx);
        }
        else {
            find_turns(color, mtx);
        }
        auto now_turns = turns;
        auto now_have_beats = have_beats;

        if (!now_have_beats && x != -1) {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty()) {
            return (depth % 2 ? 0 : INF);
        }

        double min_score = INF + 1;
        double max_score = -1;
        for (auto turn : now_turns) {
            double score;
            if (now_have_beats) {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }
            else {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }

            min_score = min(min_score, score);
            max_score = max(max_score, score);
            // alpha-beta cutter
            if (depth % 2) {
                alpha = max(alpha, max_score);
            }
            else {
                beta = min(beta, min_score);
            }
            if (optimization != "O0" && alpha > beta) {
                break;
            }
            if (optimization == "O2" && alpha == beta) {
                return (depth % 2 ? max_score + 1 : min_score - 1);
            }
        }
        
        return (depth % 2 ? max_score : min_score);
        
    }

public:
    // Поиск всех возможных ходов для фигуры определенного цвета..
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board()); // Вызов основной функции с текущим состоянием доски
    }

    // Поиск всех возможных ходов для фигуры на конкретной клетке
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board()); // Вызов основной функции с текущим состоянием доски
    }

private:
    // Основная функция для поиска ходов для фигуры определенного цвета
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns; // Вектор для хранения найденных ходов
        bool have_beats_before = false; // Флаг, указывающий есть ли взятия в текущем ходе
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color) // Проверка, что фигура принадлежит противнику
                {
                    find_turns(i, j, mtx); // Поиск ходов для фигуры
                    if (have_beats && !have_beats_before) // Если найдены взятия и до этого их не было
                    {
                        have_beats_before = true;
                        res_turns.clear(); // Очищаем предыдущие ходы
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    // Поиск ходов для фигуры на конкретной клетке
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
    vector<move_pos> turns;
    bool have_beats;
    int Max_depth; // Максимальная глубина рекурсии для поиска лучшего хода

  private:
    default_random_engine rand_eng; // Генератор случайных чисел для перемешивания ходов
    string scoring_mode; // Режим оценки ("NumberAndPotential")
    string optimization; // Уровень оптимизации (O0,O1)
    vector<move_pos> next_move; // Вектор для хранения следующего хода в цепочке
    vector<int> next_best_state; // Вектор для хранения следующего состояния в цепочке
    Board *board; // Указатель на объект доски
    Config *config; // Указатель на объект конфигурации
};
