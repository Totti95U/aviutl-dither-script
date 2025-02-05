#include <lua.hpp>
#include <random>

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

    int seed = static_cast<int>(lua_tointeger(L, 6));
    std::mt19937 mt(seed);
    std::uniform_int_distribution<int> uni(0, 255);

    // TODO: Implement multi color dithering
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            int delta = 0xff / c_num;
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

// Make function list
static luaL_Reg functions[] = {
    { "wn_dither", wn_dither },
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
