#include <lua.hpp>
#include <random>
#include <unistd.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define clamp(x) (min(max(x, 0), 255))
#define mixed(x, y) clamp(x * mix + y * (1.0f - mix))

using namespace std;

/*==================== Pixel structure ====================*/
struct Pixel_RGB {
    unsigned char r, g, b;
};



Pixel_RGB* read_txr(const char* path, int* w, int* h) {
    // open texture file
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror("Failed to open texture file");
        return nullptr;
    }

    // read texture size
    unsigned char logW, logH;
    fread(&logW, sizeof(unsigned char), 1, file);
    fread(&logH, sizeof(unsigned char), 1, file);
    *w = 1 << logW;
    *h = 1 << logH;
    // read texture data
    Pixel_RGB *texture = (Pixel_RGB*)malloc(sizeof(Pixel_RGB) * *w * *h);
    if (!texture) {
        perror("Failed to allocate memory for texture");
        fclose(file);
        return nullptr;
    }
    fread(texture, sizeof(Pixel_RGB), *w * *h, file);
    fclose(file);
    return texture;
}
struct Pixel_RGBA {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

std::uniform_real_distribution<float> U_RAND(-0.5f, 0.5f);
std::uniform_int_distribution<int> channel_RAND(0, 2);
std::uniform_int_distribution<int> comb_RAND(0, 5);
std::uniform_int_distribution<int> I_RAND(0, 2048 * 2048 - 1);

int USE_CHANNEL = 3;
int USE_COMB = 0;

/*==================== Error diffusion NOISY dithering ====================*/

// Noise selecting function
void noises(std::default_random_engine& mt, float* channel, int noise_type, Pixel_RGB* texture, int* t_index) {
    float tmp = U_RAND(mt);

    if (noise_type == 1){
        // Uniformly on [-0.5, 0.5)
        // White noise (all channel use same value)
        channel[0] = tmp;
        channel[1] = tmp;
        channel[2] = tmp;
        return;
    } else if (noise_type == 2) {
        // Uniformly on [-0.5, 0.5)
        // White noise (all channel use different value)
        channel[0] = U_RAND(mt);
        channel[1] = U_RAND(mt);
        channel[2] = U_RAND(mt);
        return;
    } else if (noise_type == 3 || noise_type == 5) {
        // use texture (all channel use same value)
        float threshold = 0.0f;
        if (USE_CHANNEL == 0) {
            threshold = texture[t_index[0]].r;
        } else if (USE_CHANNEL == 1) {
            threshold = texture[t_index[0]].g;
        } else {
            threshold = texture[t_index[0]].b;
        }
        channel[0] = threshold / 255.0f - 0.5f;
        channel[1] = threshold / 255.0f - 0.5f;
        channel[2] = threshold / 255.0f - 0.5f;
        return;
    } else if (noise_type == 4 || noise_type == 6) {
        // use texture (all channel use different value)
        float thresholds[3];
        if (USE_COMB == 0) {
            thresholds[0] = texture[t_index[0]].r;
            thresholds[1] = texture[t_index[1]].g;
            thresholds[2] = texture[t_index[2]].b;
        } else if (USE_COMB == 1) {
            thresholds[0] = texture[t_index[0]].r;
            thresholds[1] = texture[t_index[1]].b;
            thresholds[2] = texture[t_index[2]].g;
        } else if (USE_COMB == 2) {
            thresholds[0] = texture[t_index[0]].g;
            thresholds[1] = texture[t_index[1]].r;
            thresholds[2] = texture[t_index[2]].b;
        } else if (USE_COMB == 3) {
            thresholds[0] = texture[t_index[0]].g;
            thresholds[1] = texture[t_index[1]].b;
            thresholds[2] = texture[t_index[2]].r;
        } else if (USE_COMB == 4) {
            thresholds[0] = texture[t_index[0]].b;
            thresholds[1] = texture[t_index[1]].r;
            thresholds[2] = texture[t_index[2]].g;
        } else {
            thresholds[0] = texture[t_index[0]].b;
            thresholds[1] = texture[t_index[1]].g;
            thresholds[2] = texture[t_index[2]].r;
        }
        channel[0] = thresholds[0] / 255.0f - 0.5f;
        channel[1] = thresholds[1] / 255.0f - 0.5f;
        channel[2] = thresholds[2] / 255.0f - 0.5f;
        return;
    } else {
        // constant
        channel[0] = 0.0f;
        channel[1] = 0.0f;
        channel[2] = 0.0f;
        return;
    }
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

    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 9));
    int t_w, t_h;
    Pixel_RGB *texture = nullptr;
    if (noise_type >= 3) {
        texture = read_txr(texture_path, &t_w, &t_h);
        if (!texture) {
            perror("Failed to open texture file");
            return 1;
        }
    }

    float* errors;
    errors = (float*)malloc(sizeof(float) * 3 * w * h);
    for (int i = 0; i < 3 * w * h; i++) {
        errors[i] = 0.0f;
    }

    std::default_random_engine mt(seed);
    USE_CHANNEL = channel_RAND(mt);
    USE_COMB = comb_RAND(mt);
    int t_offset[3] = { 0, 0, 0 };
    if (noise_type >= 3) {
        t_offset[0] = I_RAND(mt) % (t_w * t_h);
        t_offset[1] = I_RAND(mt) % (t_w * t_h);
        t_offset[2] = I_RAND(mt) % (t_w * t_h);
    }
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            int t_index[3] = { 0, 0, 0 };
            if (noise_type >= 3) {
                t_index[0] = (x % t_w + t_w * (y % t_h) + t_offset[0]) % (t_w * t_h);
                t_index[1] = (x % t_w + t_w * (y % t_h) + t_offset[1]) % (t_w * t_h);
                t_index[2] = (x % t_w + t_w * (y % t_h) + t_offset[2]) % (t_w * t_h);
            }
            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 225.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 225.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 225.0f + errors[2 + 3 * index];

            float noise[3] = { 0.0f, 0.0f, 0.0f };
            noises(mt, noise, noise_type, texture, t_index);

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
    free(texture);
    return 0;
}

// Custom texture dithering (uniformly distributed color palette version)
int custom_texture_diffusion_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int diffusion_type = static_cast<int>(lua_tointeger(L, 6));

    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 7));
    int t_w, t_h;
    Pixel_RGB *texture = nullptr;
    texture = read_txr(texture_path, &t_w, &t_h);
    if (!texture || t_w * t_h == 0) {
        perror("Failed to open texture file");
        return 1;
    }

    int t_x = static_cast<int>(lua_tointeger(L, 8));
    int t_y = static_cast<int>(lua_tointeger(L, 9));
    float t_scale = static_cast<float>(lua_tonumber(L, 10));
    int center_x = static_cast<int>(lua_tointeger(L, 11)) + w / 2;
    int center_y = static_cast<int>(lua_tointeger(L, 12)) + h / 2;

    if (t_scale == 0.0f) {
        perror("Invalid scale");
        return 1;
    }

    float* errors;
    errors = (float*)malloc(sizeof(float) * 3 * w * h);
    for (int i = 0; i < 3 * w * h; i++) {
        errors[i] = 0.0f;
    }
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            float scaled_x = (x - center_x) / t_scale + center_x;
            float scaled_y = (y - center_y) / t_scale + center_y;
            int t_index = static_cast<int>(scaled_x - t_x) % t_w + t_w * (static_cast<int>(scaled_y - t_y) % t_h);
            
            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 225.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 225.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 225.0f + errors[2 + 3 * index];

            // channel r
            int r_num = std::floor(r * c_num);
            if (r * c_num - r_num < noise_amp * texture[t_index].r / 255.0f) {
                pixels[index].r = clamp(r_num * 0xff / c_num);
            } else {
                pixels[index].r = clamp((r_num + 1) * 0xff / c_num);
            }
            error[0] = r - pixels[index].r / 225.0f;

            // channel g
            int g_num = std::floor(g * c_num);
            if (g * c_num - g_num < noise_amp * texture[t_index].g / 255.0f) {
                pixels[index].g = clamp(g_num * 0xff / c_num);
            } else {
                pixels[index].g = clamp((g_num + 1) * 0xff / c_num);
            }
            error[1] = g - pixels[index].g / 225.0f;

            // channel b
            int b_num = std::floor(b * c_num);
            if (b * c_num - b_num < noise_amp * texture[t_index].b / 255.0f) {
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
    free(texture);
    return 0;
}

int texture_debug(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int seed = static_cast<int>(lua_tointeger(L, 6));
    int noise_type = static_cast<int>(lua_tointeger(L, 7));
    int diffusion_type = static_cast<int>(lua_tointeger(L, 8));

    // open texture file
    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 9));
    int t_w, t_h;
    Pixel_RGB *texture = read_txr(texture_path, &t_w, &t_h);
    if (!texture) {
        return 1;
    }

    float* errors;
    errors = (float*)malloc(sizeof(float) * 3 * w * h);
    for (int i = 0; i < 3 * w * h; i++) {
        errors[i] = 0.0f;
    }

    std::default_random_engine mt(seed);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            int t_index = x % t_w + t_w * (y % t_h);
            if (x < t_w && y < t_h) {
                pixels[index].r = texture[t_index].r;
                pixels[index].g = texture[t_index].g;
                pixels[index].b = texture[t_index].b;
            }
        }
    }

    free(errors);
    free(texture);
    return 0;
}

// Make function list
static luaL_Reg functions[] = {
    { "uniform_palette_dither", noisy_error_diffusion_dither },
    { "uniform_palette_custom_dither", custom_texture_diffusion_dither },
    { "texture_debug", texture_debug },
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
