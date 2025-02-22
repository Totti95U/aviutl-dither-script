#include <lua.hpp>
#include <random>
#include <bits/stdc++.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define clamp(x) (min(max(x, 0), 255))

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
    int delta = 0xff / c_num;

    int seed = static_cast<int>(lua_tointeger(L, 6));
    std::mt19937 mt(seed);
    std::uniform_int_distribution<int> uni(0, 255);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            // Channel R
            int i = pixels[index].r / delta;
            if (c_num * (pixels[index].r % delta) < uni(mt)) {
                pixels[index].r = clamp(mix * delta * i + (1 - mix) * pixels[index].r);
            } else {
                pixels[index].r = clamp(mix * delta * (i + 1) + (1 - mix) * pixels[index].r);
            }
            // Channel G
            i = pixels[index].g / delta;
            if (c_num * (pixels[index].g % delta) < uni(mt)) {
                pixels[index].g = clamp(mix * delta * i + (1 - mix) * pixels[index].g);
            } else {
                pixels[index].g = clamp(mix * delta * (i + 1) + (1 - mix) * pixels[index].g);
            }
            // Channel B
            i = pixels[index].b / delta;
            if (c_num * (pixels[index].b % delta) < uni(mt)) {
                pixels[index].b = clamp(mix * delta * i + (1 - mix) * pixels[index].b);
            } else {
                pixels[index].b = clamp(mix * delta * (i + 1) + (1 - mix) * pixels[index].b);
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
    int delta = 0xff / c_num;

    std::vector<std::tuple<float, float, float>> errors(w * h);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            std::tuple<int, int, int> error;

            // Channel R
            float old_r = pixels[index].r + std::get<0>(errors[index]);
            int i = old_r / delta;
            int new_r = 0;
            if (2 * (old_r - i) < delta) {
                new_r = clamp(mix * delta * i + (1 - mix) * pixels[index].r);
            } else {
                new_r = clamp(mix * delta * (i + 1) + (1 - mix) * pixels[index].r);
            }

            // Channel G
            float old_g = pixels[index].g + std::get<1>(errors[index]);
            i = old_g / delta;
            int new_g = 0;
            if (2 * (old_g - i) < delta) {
                new_g = clamp(mix * delta * i + (1 - mix) * pixels[index].g);
            } else {
                new_g = clamp(mix * delta * (i + 1) + (1 - mix) * pixels[index].g);
            }

            // Channel B
            float old_b = pixels[index].b + std::get<2>(errors[index]);
            i = old_b / delta;
            int new_b = 0;
            if (2 * (old_b - i) < delta) {
                new_b = clamp(mix * delta * i + (1 - mix) * pixels[index].b);
            } else {
                new_b = clamp(mix * delta * (i + 1) + (1 - mix) * pixels[index].b);
            }

            // Error diffusion
            error = std::make_tuple(
                old_r - new_r,
                old_g - new_g,
                old_b - new_b
            );

            // x + 1, y
            errors[index + 1] = std::make_tuple(
                std::get<0>(errors[index + 1]) + std::get<0>(error) * 7 / 16,
                std::get<1>(errors[index + 1]) + std::get<1>(error) * 7 / 16,
                std::get<2>(errors[index + 1]) + std::get<2>(error) * 7 / 16
            );
            // x - 1, y + 1
            errors[index + w - 1] = std::make_tuple(
                std::get<0>(errors[index + w - 1]) + std::get<0>(error) * 3 / 16,
                std::get<1>(errors[index + w - 1]) + std::get<1>(error) * 3 / 16,
                std::get<2>(errors[index + w - 1]) + std::get<2>(error) * 3 / 16
            );
            // x, y + 1
            errors[index + w] = std::make_tuple(
                std::get<0>(errors[index + w]) + std::get<0>(error) * 5 / 16,
                std::get<1>(errors[index + w]) + std::get<1>(error) * 5 / 16,
                std::get<2>(errors[index + w]) + std::get<2>(error) * 5 / 16
            );
            // x + 1, y + 1
            errors[index + w + 1] = std::make_tuple(
                std::get<0>(errors[index + w + 1]) + std::get<0>(error) * 1 / 16,
                std::get<1>(errors[index + w + 1]) + std::get<1>(error) * 1 / 16,
                std::get<2>(errors[index + w + 1]) + std::get<2>(error) * 1 / 16
            );
        }
    }

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
