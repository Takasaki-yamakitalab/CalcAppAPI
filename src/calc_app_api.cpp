#include <CalcAppAPI/calc_app_api.hpp>
#include <MathRenderer/math_renderer.hpp>
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
      plot(WIDTH, HEIGHT, STATE_PLOT, STATE_CHANGE) {}

CalcAppAPI::CalcAppAPI(int screen_width, int screen_height, int formula_area_height)
    : screen_width(screen_width),
      screen_height(screen_height),
      formula_area_height(formula_area_height),
      renderer(screen_width, screen_height),
      plot(screen_width, screen_height, STATE_PLOT, STATE_CHANGE) {}

int CalcAppAPI::load_font_data(const char* data, size_t length) {
    bmp::Image img = bmp::loadBMP1bit(data, length);
    std::vector<bmp::Glyph> glyph_list(0);
    for (int i = 0; i < math_renderer::LETTER_NUM; i++) {
        glyph_list.push_back(bmp::extractGlyph(img, i, math_renderer::BASE_W, math_renderer::BASE_H));
    }
    this->renderer.load_glyph_list(glyph_list);
    this->plot.load_glyph_list(glyph_list);
    this->renderer.init_letter_list();
    return this->state;
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

int CalcAppAPI::process_draw_normal(uint32_t* dst, uint32_t stride_pixels, int* activateFlg) {
    if (this->state != STATE_DEFAULT && this->state != STATE_CALC && this->state != STATE_EXPAND) return this->state;
    if (dst == nullptr) return this->state;

    this->renderer.clear();

    if (this->state == STATE_CALC) {
        int index = 0;
        std::string math_str;
        math_renderer::make_math_str(this->input, index, math_str);
        if (math_str == "we thirst for the seven wailings. we bear the koan of jericho." || math_str == "bluearchive") {
            *activateFlg = 1;
        }
        sci_calc::Complex ans = this->calc.calc(math_str);
        std::vector<math_renderer::Token> ans_tokens;
        std::string buff;
        buff = "=" + this->calc.to_string_g(ans);
        for (char c : buff) {
            if (c == '\0') break;
            ans_tokens.push_back(math_renderer::make_token(math_renderer::TK_SYMBOL, std::string(1, c)));
        }
        math_renderer::LayoutBox box = math_renderer::LayoutEngine(ans_tokens).layout();
        box.shift(this->screen_width - box.width, this->screen_height - (box.ascent + box.descent));
        this->renderer.draw_tokens(box.tokens, 0, this->formula_area_height + 1, this->screen_width,
                                   this->screen_height);
        this->renderer.draw_special_objects(box.spobj);
    }
    if (this->state == STATE_EXPAND) {
        int index = 0;
        std::string math_str;
        math_renderer::make_math_str(this->input, index, math_str);
        math_str = this->calc.expand(math_str);
        std::vector<math_renderer::Token> ans_tokens = math_renderer::make_tokens(math_str);
        this->input = math_renderer::make_tokens(math_str);
        this->cursor = this->input.size();
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

    uint32_t* frame = this->renderer.get_draw_buffer();

    for (int y = 0; y < this->screen_height; y++) {
        std::memcpy(dst + y * stride_pixels, frame + y * this->screen_width, this->screen_width * sizeof(uint32_t));
    }

    return this->state;
}

int CalcAppAPI::process_draw_graph(uint32_t* dst, uint32_t stride_pixels) {
    if (this->state != STATE_PLOT) return this->state;
    if (dst == nullptr) return this->state;

    if (this->plot.refresh(this->calc)) {
        this->state = STATE_CHANGE;
        return this->state;
    };

    uint32_t* frame = this->plot.get_draw_buffer();

    for (int y = 0; y < this->screen_height; y++) {
        std::memcpy(dst + y * stride_pixels, frame + y * this->screen_width, this->screen_width * sizeof(uint32_t));
    }
    return this->state;
}

int CalcAppAPI::process_touch_move_cursor(int x, int y) {
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
