#include <lua.hpp>
#include <random>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define clamp(x) (min(max(x, 0), 255))
#define mixed(x, y) clamp(x * mix + y * (1.0f - mix))

using namespace std;

struct Pixel_RGBA {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

// White noise dithering
int wn_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float mix = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int seed = static_cast<int>(lua_tointeger(L, 6));
    std::default_random_engine mt(seed);
    std::uniform_real_distribution<float> u_rand(0.0f, 1.0f);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            float r = pixels[index].r / 225.0f;
            float g = pixels[index].g / 225.0f;
            float b = pixels[index].b / 225.0f;

            // channel r
            int r_num = std::floor(r * c_num);
            if (r * c_num - r_num < u_rand(mt)) {
                pixels[index].r = mixed(r_num * 0xff / c_num, pixels[index].r);
            } else {
                pixels[index].r = mixed((r_num + 1) * 0xff / c_num, pixels[index].r);
            }

            // channel g
            int g_num = std::floor(g * c_num);
            if (g * c_num - g_num < u_rand(mt)) {
                pixels[index].g = mixed(g_num * 0xff / c_num, pixels[index].g);
            } else {
                pixels[index].g = mixed((g_num + 1) * 0xff / c_num, pixels[index].g);
            }

            // channel b
            int b_num = std::floor(b * c_num);
            if (b * c_num - b_num < u_rand(mt)) {
                pixels[index].b = mixed(b_num * 0xff / c_num, pixels[index].b);
            } else {
                pixels[index].b = mixed((b_num + 1) * 0xff / c_num, pixels[index].b);
            }
        }
    }
    
    return 0;
}

int floyd_steinberg_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float mix = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    float* errors;
    errors = (float*)malloc(sizeof(float) * 3 * w * h);
    for (int i = 0; i < 3 * w * h; i++) {
        errors[i] = 0.0f;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 225.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 225.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 225.0f + errors[2 + 3 * index];

            // channel r
            int new_r = 0;
            int r_num = std::floor(r * c_num);
            if (r * c_num - r_num < 0.5f) {
                new_r = r_num * 0xff / c_num;
            } else {
                new_r = (r_num + 1) * 0xff / c_num;
            }
            error[0] = r - new_r / 225.0f;

            // channel g
            int new_g = 0;
            int g_num = std::floor(g * c_num);
            if (g * c_num - g_num < 0.5f) {
                new_g = g_num * 0xff / c_num;
            } else {
                new_g = (g_num + 1) * 0xff / c_num;
            }
            error[1] = g - new_g / 225.0f;

            // channel b
            int new_b = 0;
            int b_num = std::floor(b * c_num);
            if (b * c_num - b_num < 0.5f) {
                new_b = b_num * 0xff / c_num;
            } else {
                new_b = (b_num + 1) * 0xff / c_num;
            }
            error[2] = b - new_b / 225.0f;

            pixels[index].r = mixed(new_r, pixels[index].r);
            pixels[index].g = mixed(new_g, pixels[index].g);
            pixels[index].b = mixed(new_b, pixels[index].b);

            // error diffusion
            // x + 1, y
            if (x + 1 < w) {
                errors[0 + 3 * (index + 1)] += error[0] * 7.0f / 16.0f;
                errors[1 + 3 * (index + 1)] += error[1] * 7.0f / 16.0f;
                errors[2 + 3 * (index + 1)] += error[2] * 7.0f / 16.0f;
            }
            // x - 1, y + 1
            if (x - 1 >= 0 && y + 1 < h) {
                errors[0 + 3 * (index - 1 + w)] += error[0] * 3.0f / 16.0f;
                errors[1 + 3 * (index - 1 + w)] += error[1] * 3.0f / 16.0f;
                errors[2 + 3 * (index - 1 + w)] += error[2] * 3.0f / 16.0f;
            }
            // x, y + 1
            if (y + 1 < h) {
                errors[0 + 3 * (index + w)] += error[0] * 5.0f / 16.0f;
                errors[1 + 3 * (index + w)] += error[1] * 5.0f / 16.0f;
                errors[2 + 3 * (index + w)] += error[2] * 5.0f / 16.0f;
            }
            // x + 1, y + 1
            if (x + 1 < w && y + 1 < h) {
                errors[0 + 3 * (index + 1 + w)] += error[0] * 1.0f / 16.0f;
                errors[1 + 3 * (index + 1 + w)] += error[1] * 1.0f / 16.0f;
                errors[2 + 3 * (index + 1 + w)] += error[2] * 1.0f / 16.0f;
            }
        }
    }

    free(errors);

    return 0;
}

// Make function list
static luaL_Reg functions[] = {
    { "wn_dither", wn_dither },
    { "floyd_steinberg_dither", floyd_steinberg_dither },
    { nullptr, nullptr }
};

// Register function and export
extern "C" {
    // `AUL_DLL_Sample` is a call name from Lua
    // the detail is `http://milkpot.sakura.ne.jp/lua/lua51_manual_ja.html#pdf-require`
    __declspec(dllexport) int luaopen_DITHERINGS(lua_State *L) {
        luaL_register(L, "DITHERINGS", functions);
        return 1;
    }
}
