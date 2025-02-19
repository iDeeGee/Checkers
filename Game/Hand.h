#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
  public:
    Hand(Board *board) : board(board)
    {
    }
    // Метод для получения выбранной клетки и обработки событий
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // Структура для хранения событий SDL
        Response resp = Response::OK;
        int x = -1, y = -1; // Координаты курсора мыши
        int xc = -1, yc = -1; // Координаты клетки на доске
        
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                // Обработка выхода
                case SDL_QUIT:
                    resp = Response::QUIT;
                    break;
                // Вычисляются координаты клетки на доске
                case SDL_MOUSEBUTTONDOWN:
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);  

                    // Обработка специальных кнопок 
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;
                    }
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                // Обработка изменения размера окна
                case SDL_WINDOWEVENT:
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // Перерисовываем доску с новым размером
                        break;
                    }
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return {resp, xc, yc};
    }
    // Метод ожидает действий пользователя
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                break;
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return resp;
    }

  private:
    Board *board; // Указатель на объект доски
};
