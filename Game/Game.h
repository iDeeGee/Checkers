#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        // Замер времени игры
        auto start = chrono::steady_clock::now();

        // Проверка повтор или новая игра
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            board.start_draw();
        }
        is_replay = false;

        // Инициализация переменных
        int turn_num = -1;
        bool is_quit = false;
        const int Max_turns = config("Game", "MaxNumTurns");

        // Основной игровой цикл
        while (++turn_num < Max_turns)
        {
            beat_series = 0;
            logic.find_turns(turn_num % 2);
            if (logic.turns.empty())
                break;
            // Считывание настроек 
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Обработка хода (действий) пользователя и бота.
                auto resp = player_turn(turn_num % 2);
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Проверка отмены хода
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                bot_turn(turn_num % 2);
        }
        // Время окончания игры
        auto end = chrono::steady_clock::now();
        // Запись в log.txt время игры. 
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        if (is_replay)
            return play();
        if (is_quit)
            return 0;
        // Определение результатов игры и ожидание действий пользователя.
        int res = 2;
        if (turn_num == Max_turns)
        {
            res = 0;
        }
        else if (turn_num % 2)
        {
            res = 1;
        }
        board.show_final(res);
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res;
    }

  private:
    // Функция для выполнения хода бота
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now(); // Засекаем время начала хода

        auto delay_ms = config("Bot", "BotDelayMS"); // Задержка для имитации "раздумий" бота

        // Создаем отдельный поток для задержки, чтобы не блокировать основной поток
        thread th(SDL_Delay, delay_ms);

        // Поиск лучшего хода для бота
        auto turns = logic.find_best_turns(color);
        th.join(); // Ожидаем завершения потока с задержкой

        bool is_first = true;

        // Выполнение ходов
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms); // Задержка между ходами в серии
            }
            is_first = false;
            beat_series += (turn.xb != -1); // Увеличиваем счетчик серии взятий, если был взят фигуру
            board.move_piece(turn, beat_series); // Выполняем ход на доске
        }

        auto end = chrono::steady_clock::now(); // Засекаем время окончания хода

        // Логируем время выполнения хода
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // Подсветка доступных для хода клеток
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);
        move_pos pos = {-1, -1, -1, -1};
        POS_T x = -1, y = -1;
        // Обработка выбора клетки 
        while (true)
        {
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            // Проверка на корректность выбора клетки
            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second})
                {
                    pos = turn;
                    break;
                }
            }

            // Обработка некорректного выбора клетки
            if (pos.x != -1)
                break;
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }

        // Выполнение хода 
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);
        if (pos.xb == -1)
            return Response::OK;
        // Продолжить серию взятий.  
        beat_series = 1;
        
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // Обработка продолжения взятий
            while (true)
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }
        // возврат результата
        return Response::OK;
    }
    
  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
