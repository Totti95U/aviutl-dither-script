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

std::uniform_real_distribution<float> U_RAND(-0.5f, 0.5f);

/*==================== Error diffusion NOISY dithering ====================*/

// Noise selecting function
void noises(std::default_random_engine& mt, float* channel, int noise_type) {
    float tmp = U_RAND(mt);
    switch (noise_type) {
        // Uniformly on [0, 1)
        // All channels are same
        case 1:
            channel[0] = tmp;
            channel[1] = tmp;
            channel[2] = tmp;
            return;
        // Uniformly on [0.5, 1)
        // All channels are distinct
        case 2:
            channel[0] = U_RAND(mt);
            channel[1] = U_RAND(mt);
            channel[2] = U_RAND(mt);
            return;
        // TODO: implement using blue noise
        case 3:
            return;
        // TODO: implement using bayer matrix
        case 4:
            return;
        // constant
        default:
            channel[0] = 0.0f;
            channel[1] = 0.0f;
            channel[2] = 0.0f;
            return;
    }

    return;
}

void diffuse_at(float* errors, float* error, int w, int h, int x, int y, float ratio) {
    if (x < 0 || x >= w || y < 0 || y >= h) {
        return;
    } else {
        errors[0 + 3 * (x + w * y)] += error[0] * ratio;
        errors[1 + 3 * (x + w * y)] += error[1] * ratio;
        errors[2 + 3 * (x + w * y)] += error[2] * ratio;
        return;
    }
    return;
}

void diffuse_error(float* errors, float* error, int x, int y, int w, int h, int diffusion_type){
    switch (diffusion_type) {
    // Floyd-Steinberg
    case 4:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 7.0f / 16.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 3.0f / 16.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 5.0f / 16.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 1.0f / 16.0f);
        break;
    // False Floyd-Steinberg
    case 2:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 3.0f / 8.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 3.0f / 8.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 2.0f / 8.0f);
        break;
    // Jarvis, Judice, and Ninke
    case 9:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 7.0f / 48.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 5.0f / 48.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 3.0f / 48.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 5.0f / 48.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 7.0f / 48.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 5.0f / 48.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 3.0f / 48.0f);
        // x - 2, y + 2
        diffuse_at(errors, error, w, h, x - 2, y + 2, 1.0f / 48.0f);
        // x - 1, y + 2
        diffuse_at(errors, error, w, h, x - 1, y + 2, 3.0f / 48.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 5.0f / 48.0f);
        // x + 1, y + 2
        diffuse_at(errors, error, w, h, x + 1, y + 2, 3.0f / 48.0f);
        // x + 2, y + 2
        diffuse_at(errors, error, w, h, x + 2, y + 2, 1.0f / 48.0f);
        break;
    // Stucki
    case 8:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 8.0f / 42.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 4.0f / 42.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 2.0f / 42.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 4.0f / 42.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 8.0f / 42.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 4.0f / 42.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 2.0f / 42.0f);
        // x - 2, y + 2
        diffuse_at(errors, error, w, h, x - 2, y + 2, 1.0f / 42.0f);
        // x - 1, y + 2
        diffuse_at(errors, error, w, h, x - 1, y + 2, 2.0f / 42.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 4.0f / 42.0f);
        // x + 1, y + 2
        diffuse_at(errors, error, w, h, x + 1, y + 2, 2.0f / 42.0f);
        // x + 2, y + 2
        diffuse_at(errors, error, w, h, x + 2, y + 2, 1.0f / 42.0f);
        break;
    // Atkinson
    case 3:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 1.0f / 8.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 1.0f / 8.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 1.0f / 8.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 1.0f / 8.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 1.0f / 8.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 1.0f / 8.0f);
        break;
    // Burkes
    case 6:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 8.0f / 32.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 4.0f / 32.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 2.0f / 32.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 4.0f / 32.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 8.0f / 32.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 4.0f / 32.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 2.0f / 32.0f);
        break;
    // Sierra
    case 7:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 5.0f / 32.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 3.0f / 32.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 2.0f / 32.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 4.0f / 32.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 5.0f / 32.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 4.0f / 32.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 2.0f / 32.0f);
        // x - 1, y + 2
        diffuse_at(errors, error, w, h, x - 1, y + 2, 2.0f / 32.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 3.0f / 32.0f);
        // x + 1, y + 2
        diffuse_at(errors, error, w, h, x + 1, y + 2, 2.0f / 32.0f);
        break;
    // Two-row Sierra
    case 5:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 4.0f / 16.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 3.0f / 16.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 1.0f / 16.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 2.0f / 16.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 3.0f / 16.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 2.0f / 16.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 1.0f / 16.0f);
        break;
    // Sierra Lite
    case 1:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 2.0f / 4.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 1.0f / 4.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 1.0f / 4.0f);
        break;
    // no diffusion
    default:
        break;
    }

    return;
}

/*------------------- Uniformly fixed palette dithering -------------------*/
// Uniformly distributed color palette version
int noisy_error_diffusion_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int seed = static_cast<int>(lua_tointeger(L, 6));
    int noise_type = static_cast<int>(lua_tointeger(L, 7));
    int diffusion_type = static_cast<int>(lua_tointeger(L, 8));

    float* errors;
    errors = (float*)malloc(sizeof(float) * 3 * w * h);
    for (int i = 0; i < 3 * w * h; i++) {
        errors[i] = 0.0f;
    }

    std::default_random_engine mt(seed);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 225.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 225.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 225.0f + errors[2 + 3 * index];

            float noise[3] = { 0.0f, 0.0f, 0.0f };
            noises(mt, noise, noise_type);

            // channel r
            int r_num = std::floor(r * c_num);
            if (r * c_num - r_num < noise_amp * noise[0] + 0.5f) {
                pixels[index].r = clamp(r_num * 0xff / c_num);
            } else {
                pixels[index].r = clamp((r_num + 1) * 0xff / c_num);
            }
            error[0] = r - pixels[index].r / 225.0f;

            // channel g
            int g_num = std::floor(g * c_num);
            if (g * c_num - g_num < noise_amp * noise[1] + 0.5f) {
                pixels[index].g = clamp(g_num * 0xff / c_num);
            } else {
                pixels[index].g = clamp((g_num + 1) * 0xff / c_num);
            }
            error[1] = g - pixels[index].g / 225.0f;

            // channel b
            int b_num = std::floor(b * c_num);
            if (b * c_num - b_num < noise_amp * noise[2] + 0.5f) {
                pixels[index].b = clamp(b_num * 0xff / c_num);
            } else {
                pixels[index].b = clamp((b_num + 1) * 0xff / c_num);
            }
            error[2] = b - pixels[index].b / 225.0f;

            // error diffusion
            diffuse_error(errors, error, x, y, w, h, diffusion_type);
        }
    }

    free(errors);
    return 0;
}

// Make function list
static luaL_Reg functions[] = {
    { "uniform_palette_dither", noisy_error_diffusion_dither },
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
