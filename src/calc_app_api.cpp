#include <BitmapTool/bmp.hpp>
#include <CalcAppAPI/calc_app_api.hpp>
#include <MathRenderer/math_renderer.hpp>
#include <MathRenderer/math_renderer_defines.hpp>
#include <Pentomino/pentomino_defines.hpp>
#include <PentominoBMP/pentomino_bmp.hpp>
#include <cstdint>

namespace calc_app_api {

constexpr static int WIDTH = 400;
constexpr static int HEIGHT = 300;
constexpr static int FORMULA_DISPLAY_HEIGHT = 200;

CalcAppAPI::CalcAppAPI()
    : screen_width(WIDTH),
      screen_height(HEIGHT),
      formula_area_height(FORMULA_DISPLAY_HEIGHT),
      renderer(WIDTH, HEIGHT),
      plot(WIDTH, HEIGHT, STATE_PLOT, STATE_DEFAULT),
      pentomino(WIDTH, HEIGHT) {
    bmp::loadLetterGlyph();
}

CalcAppAPI::CalcAppAPI(int screen_width, int screen_height, int formula_area_height)
    : screen_width(screen_width),
      screen_height(screen_height),
      formula_area_height(formula_area_height),
      renderer(screen_width, screen_height),
      plot(screen_width, screen_height, STATE_PLOT, STATE_DEFAULT),
      pentomino(screen_width, screen_height) {
    bmp::loadLetterGlyph();
}

int CalcAppAPI::init() {
    bmp::loadLetterGlyph();
    return this->state;
}
int CalcAppAPI::process_input(const std::string& key) {
    this->got_input = true;
    if (this->state == STATE_PLOT) return process_input_graph(key);
    if (this->state == STATE_PENTOMINO) return process_input_pentomino(key);
    return process_input_normal(key);
}

int CalcAppAPI::process_input_normal(const std::string& key) {
    if (this->state == STATE_CHANGE) {
        this->state = STATE_DEFAULT;
        return this->state;
    }
    if (this->state != STATE_DEFAULT && this->state != STATE_CALC && this->state != STATE_EXPAND) return 0;
    if (this->state == STATE_CALC || this->state == STATE_EXPAND) {
        if (key != "left" && key != "right" && key != "equal" && key != "del") {
            this->cursor = 0;
            this->input.clear();
            this->display_offsetX = 0;
            this->display_offsetY = 0;
        }
        if (key == "plus" || key == "minus" || key == "times" || key == "div") {
            math_renderer::process_input(this->input, this->cursor, "a");
            math_renderer::process_input(this->input, this->cursor, "n");
            math_renderer::process_input(this->input, this->cursor, "s");
        }
    }
    this->state = math_renderer::process_input(this->input, this->cursor, key);
    if (this->state == STATE_PLOT) {
        this->plot.delete_var_name();
        this->plot.plot(this->input, "x");
        this->cursor = 0;
        this->input.clear();
        this->display_offsetX = 0;
        this->display_offsetY = 0;
        this->cursor_info.height = 0;
    }
    return this->state;
}

int CalcAppAPI::process_input_graph(const std::string& key) {
    if (this->state == STATE_CHANGE) {
        this->state = STATE_DEFAULT;
        return this->state;
    }
    if (this->state != STATE_PLOT) return this->state;
    this->state = this->plot.process_input(key);
    if (key == "0") {
        this->input = this->plot.get_graph_input();
        this->cursor = this->input.size();
        this->display_offsetX = 0;
        this->display_offsetY = 0;
    }
    return this->state;
}

int CalcAppAPI::process_input_pentomino(const std::string& key) {
    if (this->state == STATE_CHANGE) {
        this->state = STATE_DEFAULT;
        return this->state;
    }
    if (this->state != STATE_PENTOMINO) return this->state;
    if (key == "7") pentomino.process_input(pentomino::INPUT_UP);
    if (key == "1") pentomino.process_input(pentomino::INPUT_LEFT);
    if (key == "9") pentomino.process_input(pentomino::INPUT_RIGHT);
    if (key == "3") pentomino.process_input(pentomino::INPUT_DOWN);
    if (key == "4" || key == "2") pentomino.process_input(pentomino::INPUT_ROT_L);
    if (key == "8" || key == "6") pentomino.process_input(pentomino::INPUT_ROT_R);
    if (key == "5" || key == "0") pentomino.process_input(pentomino::INPUT_HOLD);

    if (key == "ac") {
        this->state = STATE_DEFAULT;
    }
    return this->state;
}

int CalcAppAPI::process_draw(uint32_t* dst, uint32_t stride_pixels, int* activateFlg) {
    if (this->state == STATE_PLOT) return process_draw_graph(dst, stride_pixels);
    if (this->state == STATE_PENTOMINO) return process_draw_pentomino(dst, stride_pixels);
    return process_draw_normal(dst, stride_pixels, activateFlg);
}

int CalcAppAPI::process_draw_normal(uint32_t* dst, uint32_t stride_pixels, int* activateFlg) {
    static uint32_t* frame = nullptr;

    if (this->state != STATE_DEFAULT && this->state != STATE_CALC && this->state != STATE_EXPAND) return this->state;
    if (dst == nullptr) return this->state;

    // 新たな入力が無い場合、前回の画面をそのまま描画する
    if (!this->got_input && frame != nullptr) {
        for (int y = 0; y < this->screen_height; y++) {
            std::memcpy(dst + y * stride_pixels, frame + y * this->screen_width, this->screen_width * sizeof(uint32_t));
        }
        return this->state;
    }
    this->got_input = false;

    this->renderer.clear();

    if (this->state == STATE_CALC) {
        int index = 0;
        std::string math_str;
        math_renderer::make_math_str(this->input, index, math_str);
        if (math_str == "we thirst for the seven wailings. we bear the koan of jericho." || math_str == "bluearchive") {
            this->state = STATE_DEFAULT;
            *activateFlg = 1;
            return this->state;
        }
        if (math_str == "we thirst for the wailings of jericho. we bear the seven koans.") {
            this->state = STATE_DEFAULT;
            *activateFlg = 2;
            return this->state;
        }
        if (math_str == "pentomino") {
            this->state = STATE_PENTOMINO;
            this->pentomino = pentomino_bmp::PentominoBMP(screen_width, screen_height);
            this->cursor_info.height = 0;
            return process_draw_pentomino(dst, stride_pixels);
        }
        sci_calc::Complex ans = this->calc.calc(math_str);
        math_str = "=" + this->calc.to_string_g(ans, 10);

        size_t br = math_str.find("\n");

        math_renderer::LayoutBox box;
        if (br == std::string::npos) {
            std::vector<math_renderer::Token> ans_tokens = math_renderer::make_tokens(math_str);
            math_renderer::LayoutBox box = math_renderer::LayoutEngine(ans_tokens).layout();
            box.shift(this->screen_width - box.width, this->screen_height - (box.ascent + box.descent));
            this->renderer.draw_tokens(box.tokens, 0, this->formula_area_height + 1, this->screen_width,
                                       this->screen_height);
            this->renderer.draw_special_objects(box.spobj);
        } else {
            std::string up_str = math_str.substr(0, br);
            std::string down_str = math_str.substr(br + 1);
            std::vector<math_renderer::Token> up_tokens = math_renderer::make_tokens(up_str);
            std::vector<math_renderer::Token> down_tokens = math_renderer::make_tokens(down_str);
            math_renderer::LayoutBox up_box = math_renderer::LayoutEngine(up_tokens).layout();
            math_renderer::LayoutBox down_box = math_renderer::LayoutEngine(down_tokens).layout();
            int x = this->screen_width - std::max(up_box.width, down_box.width);
            int y2 = this->screen_height - (down_box.ascent + down_box.descent);
            int y1 = y2 - (up_box.ascent + up_box.descent);
            up_box.shift(x, y1);
            down_box.shift(x, y2);
            this->renderer.draw_tokens(up_box.tokens, 0, this->formula_area_height + 1, this->screen_width,
                                       this->screen_height);
            this->renderer.draw_special_objects(up_box.spobj);
            this->renderer.draw_tokens(down_box.tokens, 0, this->formula_area_height + 1, this->screen_width,
                                       this->screen_height);
            this->renderer.draw_special_objects(down_box.spobj);
        }
    }
    if (this->state == STATE_EXPAND) {
        int index = 0;
        std::string math_str;
        math_renderer::make_math_str(this->input, index, math_str);
        math_str = this->calc.expand(math_str);
        std::vector<math_renderer::Token> ans_tokens = math_renderer::make_tokens(math_str);
        this->input = math_renderer::make_tokens(math_str);
        this->cursor = this->input.size();
        this->state = STATE_DEFAULT;
    }
    if (this->state == STATE_FACTOR) {
        int index = 0;
        std::string math_str;
        math_renderer::make_math_str(this->input, index, math_str);
        math_str = this->calc.factorize(math_str);
        std::vector<math_renderer::Token> ans_tokens = math_renderer::make_tokens(math_str);
        this->input = math_renderer::make_tokens(math_str);
        this->cursor = this->input.size();
        this->state = STATE_DEFAULT;
    }

    math_renderer::LayoutBox box = math_renderer::LayoutEngine(this->input).layout();
    box.shift(this->display_offsetX, this->display_offsetY);
    math_renderer::shift_for_cursor(box, this->input, this->cursor, this->screen_width, this->formula_area_height,
                                    this->display_offsetX, this->display_offsetY);

    if (this->cursor >= this->input.size()) {
        this->cursor_info.x = box.width + this->display_offsetX;
        this->cursor_info.y = std::max(box.ascent - math_renderer::ascent_for_scale(math_renderer::NORMAL_SCALE), 0) +
                              this->display_offsetY;
        this->cursor_info.height = math_renderer::NORMAL_SCALE * math_renderer::BASE_H;
    } else if (this->cursor >= 1 && this->input[this->cursor].type == math_renderer::TK_CLOSE_BRACE &&
               this->input[this->cursor - 1].type == math_renderer::TK_OPEN_BRACE) {
        this->cursor_info.x = this->input[this->cursor - 1].x;
        this->cursor_info.y = this->input[this->cursor - 1].y;
        this->cursor_info.height = math_renderer::ascent_for_letters(this->input[this->cursor - 1].m_letters,
                                                                     this->input[this->cursor - 1].scale) +
                                   math_renderer::descent_for_letters(this->input[this->cursor - 1].m_letters,
                                                                      this->input[this->cursor - 1].scale);
    } else {
        this->cursor_info.x = this->input[this->cursor].x;
        this->cursor_info.y = this->input[this->cursor].y;
        this->cursor_info.height =
            math_renderer::ascent_for_letters(this->input[this->cursor].m_letters, this->input[this->cursor].scale) +
            math_renderer::descent_for_letters(this->input[this->cursor].m_letters, this->input[this->cursor].scale);
    }

    this->renderer.draw_tokens(box.tokens, 0, 0, this->screen_width, this->formula_area_height);
    this->renderer.draw_special_objects(box.spobj, 0, 0, this->screen_width, this->formula_area_height);

    frame = this->renderer.get_draw_buffer();

    for (int y = 0; y < this->screen_height; y++) {
        std::memcpy(dst + y * stride_pixels, frame + y * this->screen_width, this->screen_width * sizeof(uint32_t));
    }

    return this->state;
}

int CalcAppAPI::process_draw_graph(uint32_t* dst, uint32_t stride_pixels) {
    static uint32_t* frame = nullptr;

    if (this->state != STATE_PLOT) return this->state;
    if (dst == nullptr) return this->state;

    // 新たな入力が無い場合、前回の画面をそのまま描画する
    if (!this->got_input && frame != nullptr) {
        for (int y = 0; y < this->screen_height; y++) {
            std::memcpy(dst + y * stride_pixels, frame + y * this->screen_width, this->screen_width * sizeof(uint32_t));
        }
        return this->state;
    }
    this->got_input = false;

    if (this->plot.refresh(this->calc)) {
        this->state = STATE_CHANGE;
        return this->state;
    };

    frame = this->plot.get_draw_buffer();

    for (int y = 0; y < this->screen_height; y++) {
        std::memcpy(dst + y * stride_pixels, frame + y * this->screen_width, this->screen_width * sizeof(uint32_t));
    }
    return this->state;
}

int CalcAppAPI::process_draw_pentomino(uint32_t* dst, uint32_t stride_pixels) {
    if (this->state != STATE_PENTOMINO) return this->state;
    if (dst == nullptr) return this->state;

    this->pentomino.update();
    this->pentomino.clear_screen();
    this->pentomino.draw_screen();
    uint32_t* frame = this->pentomino.get_screen().data();

    for (int y = 0; y < this->screen_height; y++) {
        std::memcpy(dst + y * stride_pixels, frame + y * this->screen_width, this->screen_width * sizeof(uint32_t));
    }
    return this->state;
}

int CalcAppAPI::process_touch_move_cursor(int x, int y) {
    this->got_input = true;
    math_renderer::move_cursor_touch(this->input, this->cursor, x, y);
    return this->state;
}
int CalcAppAPI::process_touch_screen(int x, int y) {
    switch (this->state) {
        case STATE_DEFAULT:
        case STATE_CALC:
        case STATE_EXPAND:
            process_touch_move_cursor(x, y);
            return this->state;
        case STATE_PLOT:
            plot.process_touch(x, y);
            return this->state;
        default:
            break;
    }
    return this->state;
}
int CalcAppAPI::process_swipe_screen(int dx, int dy) {
    switch (this->state) {
        case STATE_DEFAULT:
        case STATE_CALC:
        case STATE_EXPAND:
            return this->state;
        case STATE_PLOT:
            plot.process_swipe(dx, dy);
            return this->state;
        default:
            break;
    }
    return this->state;
}

CursorInfo CalcAppAPI::get_cursor_info() { return this->cursor_info; }

int CalcAppAPI::get_state() { return this->state; }

}  // namespace calc_app_api
