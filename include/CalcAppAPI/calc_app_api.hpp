#ifndef CALCULATOR_HPP
#define CALCULATOR_HPP

#include <TetrisBMP/tetris_bmp.hpp>
#include <vector>
#include <string>
#include <SciCalc/sci_calc.hpp>
#include <MathRenderer/math_renderer.hpp>
#include <DrawGraph/draw_graph.hpp>

namespace calc_app_api {

enum STATE {
    STATE_DEFAULT = 0,
    STATE_CALC = 1,
    STATE_PLOT = 2,
    STATE_EXPAND = 3,
    STATE_TETRIS = 4,
    STATE_CHANGE = -1,
};

struct CursorInfo {
    int x;
    int y;
    int height;
};

class CalcAppAPI {
private:
    int screen_width;
    int screen_height;
    int formula_area_height;
    std::vector<math_renderer::Token> input;
    int cursor = 0;
    int display_offsetX = 0;
    int display_offsetY = 0;
    int state = STATE_DEFAULT;
    math_renderer::MathRenderer renderer;
    sci_calc::SciCalc calc;
    draw_graph::DrawGraph plot;
    CursorInfo cursor_info;
    tetris_bmp::TetrisBMP tetris;
public:
    CalcAppAPI();
    CalcAppAPI(int screen_width, int screen_height, int formula_area_height);
    int load_font_data(const char* data, size_t length);
    int process_input(const std::string& key);
    int process_input_normal(const std::string& key);
    int process_input_graph(const std::string& key);
    int process_input_tetris(const std::string& key);
    int process_draw(uint32_t* pixels, uint32_t stride_pixels, int* activateFlg);
    int process_draw_normal(uint32_t* pixels, uint32_t stride_pixels, int* activateFlg);
    int process_draw_graph(uint32_t* pixels, uint32_t stride_pixels);
    int process_draw_tetris(uint32_t* pixels, uint32_t stride_pixels);
    int process_touch_move_cursor(int x, int y);
    int process_touch_screen(int x, int y);
    int process_swipe_screen(int dx, int dy);
    CursorInfo get_cursor_info();
    int get_state();
};
}  // namespace calc

#endif
