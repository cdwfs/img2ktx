#include <Compressonator.h>

#include "build_version.h"

#pragma warning(push, 3)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(disable : 4702)  // unreachable code
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBI_MALLOC(sz) malloc(sz)
#define STBI_REALLOC(p, newsz) realloc(p, newsz)
#define STBI_FREE(p) free(p)
#include <stb_image_resize.h>
#pragma warning(pop)

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

enum {
    // clang-format off
    // For glFormat
    IMG2KTX_GL_RED                           = 0x1903,
    IMG2KTX_GL_RG                            = 0x8227,
    IMG2KTX_GL_RGB                           = 0x1907,
    IMG2KTX_GL_RGBA                          = 0x1908,

    // For glType
    IMG2KTX_GL_UNSIGNED_BYTE                 = 0x1401,
    
    // For glInternalFormat
    IMG2KTX_GL_RGBA8                                  = 0x8058,
    IMG2KTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT           = 0x83F0, // BC1 (no alpha)
    IMG2KTX_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT          = 0x83F1, // BC1 (alpha)
    IMG2KTX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT          = 0x83F3, // BC3
    IMG2KTX_GL_COMPRESSED_RGBA_BPTC_UNORM_ARB         = 0x8E8C, // BC7
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_4x4_KHR           = 0x93B0,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_5x4_KHR           = 0x93B1,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_5x5_KHR           = 0x93B2,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_6x5_KHR           = 0x93B3,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_6x6_KHR           = 0x93B4,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_8x5_KHR           = 0x93B5,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_8x6_KHR           = 0x93B6,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_8x8_KHR           = 0x93B7,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x5_KHR          = 0x93B8,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x6_KHR          = 0x93B9,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x8_KHR          = 0x93BA,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x10_KHR         = 0x93BB,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_12x10_KHR         = 0x93BC,
    IMG2KTX_GL_COMPRESSED_RGBA_ASTC_12x12_KHR         = 0x93BD,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR   = 0x93D0,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR   = 0x93D1,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR   = 0x93D2,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR   = 0x93D3,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR   = 0x93D4,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR   = 0x93D5,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR   = 0x93D6,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR   = 0x93D7,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR  = 0x93D8,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR  = 0x93D9,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR  = 0x93DA,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR = 0x93DB,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR = 0x93DC,
    IMG2KTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR = 0x93DD,
    // clang-format on
};

struct CompressedFormatInfo {
    const char *name;
    CMP_FORMAT cmp_format;
    uint32_t internal_format;
    uint32_t base_format;
    uint32_t gl_format;  // channel count, effectively (RGB, RGBA, etc). For compressed formats, format=0.
    uint32_t gl_type;  // type of each channel. For compressed formats, type=0.
    uint32_t gl_type_size;  // size in bytes of gl_type for endianness conversion. for compressed types, size=1.
    uint32_t block_dim_x;
    uint32_t block_dim_y;
    uint32_t block_bytes;
};
const CompressedFormatInfo g_formats[] = {
        // clang-format off
    { "RGBA",      CMP_FORMAT_RGBA_8888, IMG2KTX_GL_RGBA8,                            IMG2KTX_GL_RGBA,  IMG2KTX_GL_RGBA, IMG2KTX_GL_UNSIGNED_BYTE, 1,  1,  1,  4 },
    { "BC1",       CMP_FORMAT_BC1,       IMG2KTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT,     IMG2KTX_GL_RGB,   0,               0,                        1,  4,  4,  8 },
    { "BC1a",      CMP_FORMAT_BC1,       IMG2KTX_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,    IMG2KTX_GL_RGBA,  0,               0,                        1,  4,  4, 16 },
    { "BC3",       CMP_FORMAT_BC3,       IMG2KTX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,    IMG2KTX_GL_RGBA,  0,               0,                        1,  4,  4, 16 },
    { "BC7",       CMP_FORMAT_BC7,       IMG2KTX_GL_COMPRESSED_RGBA_BPTC_UNORM_ARB,   IMG2KTX_GL_RGBA,  0,               0,                        1,  4,  4, 16 },
    { "ASTC4x4",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_4x4_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  4,  4, 16 },
    { "ASTC5x4",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_5x4_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  5,  4, 16 },
    { "ASTC5x5",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_5x5_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  5,  5, 16 },
    { "ASTC6x5",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_6x5_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  6,  5, 16 },
    { "ASTC6x6",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_6x6_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  6,  6, 16 },
    { "ASTC8x5",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_8x5_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  8,  5, 16 },
    { "ASTC8x6",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_8x6_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  8,  6, 16 },
    { "ASTC8x8",   CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_8x8_KHR,     IMG2KTX_GL_RGBA,  0,               0,                        1,  8,  8, 16 },
    { "ASTC10x5",  CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x5_KHR,    IMG2KTX_GL_RGBA,  0,               0,                        1, 10,  5, 16 },
    { "ASTC10x6",  CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x6_KHR,    IMG2KTX_GL_RGBA,  0,               0,                        1, 10,  6, 16 },
    { "ASTC10x8",  CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x8_KHR,    IMG2KTX_GL_RGBA,  0,               0,                        1, 10,  8, 16 },
    { "ASTC10x10", CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_10x10_KHR,   IMG2KTX_GL_RGBA,  0,               0,                        1, 10, 10, 16 },
    { "ASTC12x10", CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_12x10_KHR,   IMG2KTX_GL_RGBA,  0,               0,                        1, 12, 10, 16 },
    { "ASTC12x12", CMP_FORMAT_ASTC,      IMG2KTX_GL_COMPRESSED_RGBA_ASTC_12x12_KHR,   IMG2KTX_GL_RGBA,  0,               0,                        1, 12, 12, 16 },
        // clang-format on
};
const size_t g_format_count = sizeof(g_formats) / sizeof(g_formats[0]);

struct ImageMipLevel {
    std::vector<uint8_t> bytes;
};
struct ImagePixels {
    stbi_uc *packed;  // base mip level only, tightly packed
    std::vector<ImageMipLevel> input_mips;  // padded
    std::vector<ImageMipLevel> output_mips;
};

struct KtxHeader {
    uint8_t identifier[12];
    uint32_t endianness;
    uint32_t glType;
    uint32_t glTypeSize;
    uint32_t glFormat;
    uint32_t glInternalFormat;
    uint32_t glBaseInternalFormat;
    uint32_t pixelWidth;
    uint32_t pixelHeight;
    uint32_t pixelDepth;
    uint32_t numberOfArrayElements;
    uint32_t numberOfFaces;
    uint32_t numberOfMipmapLevels;
    uint32_t bytesOfKeyValueData;
};

void PrintVersion() { fprintf(stdout, "img2ktx %s\n", img2ktx_build_version); }

void PrintUsage(char *argv[]) {
    PrintVersion();
    fprintf(stdout, "Usage: %s [options] [input]\n", argv[0]);
    fprintf(stdout, R"options(options:
  -o [out.ktx]      Output file [required]
  -f [format]       Output format [required]
  -r [width height] Resize input to width x height before conversion.
                    Provided dimensions must both be >= 1.
  -m                Enable mipmap generation
  -c                Enable cubemap output. Each set of six input images will be
                    treated as one cubemap. Face order is +X -X +Y -Y +Z -Z.
  -q                Enable quiet mode (suppress non-error console output)
  -h                Displays this help message
  -v                Displays version information\)options");
    fprintf(stdout, "formats:\n  ");
    for (int i = 0; i < g_format_count; ++i) {
        fprintf(stdout, "%s ", g_formats[i].name);
    }
    fprintf(stdout, "\n");
}

#define qprintf(msg, ...)           \
    if (!quiet_mode) {              \
        printf((msg), __VA_ARGS__); \
    }

int main(int argc, char *argv[]) {
    std::vector<char *> input_filenames;
    const char *output_filename = nullptr;
    const char *output_format_name = nullptr;
    bool generate_mipmaps = false;
    bool output_as_cubemap = false;
    bool quiet_mode = false;
    bool base_resize_enable = false;
    int base_resize_width = 0, base_resize_height = 0;
    for (int a = 1; a < argc; ++a) {
        if (strcmp("-o", argv[a]) == 0 && a + 1 < argc) {
            output_filename = argv[++a];
        } else if (strcmp("-f", argv[a]) == 0 && a + 1 < argc) {
            output_format_name = argv[++a];
        } else if (strcmp("-r", argv[a]) == 0 && a + 2 < argc) {
            base_resize_enable = true;
            base_resize_width = (int)strtol(argv[++a], nullptr, 10);
            base_resize_height = (int)strtol(argv[++a], nullptr, 10);
        } else if (strcmp("-m", argv[a]) == 0) {
            generate_mipmaps = true;
        } else if (strcmp("-c", argv[a]) == 0) {
            output_as_cubemap = true;
        } else if (strcmp("-q", argv[a]) == 0) {
            quiet_mode = true;
        } else if (strcmp("-h", argv[a]) == 0) {
            PrintUsage(argv);
            return 0;
        } else if (strcmp("-v", argv[a]) == 0) {
            PrintVersion();
            return 0;
        } else {
            // All remaining params are input filenames
            input_filenames.insert(input_filenames.end(), argv + a, argv + argc);
            break;
        }
    }
    if (!output_filename || !output_format_name || input_filenames.empty()) {
        PrintUsage(argv);
        return -1;
    }
    if (output_as_cubemap && (input_filenames.size() % 6) != 0) {
        fprintf(stderr, "Error: when generating cubemaps, six images are required per cube.\n");
        return -1;
    }
    if (base_resize_enable && (base_resize_width < 1 || base_resize_height < 1)) {
        fprintf(stderr, "Error: resize width (%d) and height (%d) must both be >= 1.\n", base_resize_width,
                base_resize_height);
        return -1;
    }

    // Look up the output format info
    const CompressedFormatInfo *format_info = nullptr;
    for (int f = 0; f < g_format_count; ++f) {
        if (strcmp(g_formats[f].name, output_format_name) == 0) {
            format_info = g_formats + f;
            break;
        }
    }
    if (format_info == nullptr) {
        fprintf(stderr, "Error: format %s not supported\n", output_format_name);
        return 1;
    }
    const int bytes_per_block = format_info->block_bytes;
    const int block_dim_x = format_info->block_dim_x;
    const int block_dim_y = format_info->block_dim_y;
    const int block_dim_z = 1;

    // Load the input file(s)
    int base_width = 0, base_height = 0;
    int input_components = 4;  // force all input to 32-bit RGBA
    int original_components = 0;
    std::vector<ImagePixels> images(input_filenames.size());
    images[0].packed = stbi_load(input_filenames[0], &base_width, &base_height, &original_components, input_components);
    if (!images[0].packed) {
        fprintf(stderr, "Error loading input '%s'\n", input_filenames[0]);
        return 2;
    }
    if (output_as_cubemap && base_width != base_height) {
        fprintf(stderr, "Error: when generating cubemaps, input width/height must be equal.\n");
        fprintf(stderr, "  %s: %d x %d\n", input_filenames[0], base_width, base_height);
        return 4;
    }
    qprintf("Loaded %s -- width=%d height=%d comp=%d\n", input_filenames[0], base_width, base_height,
            original_components);
    // Subsequent files must match dimensions of the first
    for (size_t i = 1; i < input_filenames.size(); ++i) {
        int bw = 0, bh = 0, oc = 0;
        images[i].packed = stbi_load(input_filenames[i], &bw, &bh, &oc, input_components);
        if (!images[i].packed) {
            fprintf(stderr, "Error loading input '%s'\n", input_filenames[i]);
            return 2;
        }
        if (bw != base_width || bh != base_height) {
            fprintf(stderr, "Error: input image dimensions do not match.\n");
            fprintf(stderr, "  %s: %d x %d\n", input_filenames[0], base_width, base_height);
            fprintf(stderr, "  %s: %d x %d\n", input_filenames[i], bw, bh);
            return 3;
        }
        qprintf("Loaded %s -- width=%d height=%d comp=%d\n", input_filenames[i], bw, bh, oc);
    }
    // Optionally, resize the input images
    if (base_resize_enable) {
        for (int layer = 0; layer < (int)images.size(); ++layer) {
            auto &img = images[layer];
            stbi_uc *resized_packed = (stbi_uc *)STBI_MALLOC(base_resize_width * base_resize_height * input_components);
            stbir_resize_uint8(img.packed, base_width, base_height, base_width * input_components, resized_packed,
                    base_resize_width, base_resize_height, base_resize_width * input_components, input_components);
            stbi_image_free(img.packed);
            img.packed = resized_packed;
        }
        qprintf("Resized inputs (old: width=%d height=%d, new: width=%d height=%d\n", base_width, base_height,
                base_resize_width, base_resize_height);
        base_width = base_resize_width;
        base_height = base_resize_height;
    }

    // Determine mip chain properties
    int mip_levels = 1;
    if (generate_mipmaps) {
        int mip_w = base_width, mip_h = base_height;
        while (mip_w > 1 || mip_h > 1) {
            mip_levels += 1;
            mip_w = std::max(1, mip_w / 2);
            mip_h = std::max(1, mip_h / 2);
        }
    }
    std::vector<uint32_t> output_mip_sizes(mip_levels);

    CMP_CompressOptions options = {};
    options.dwSize = sizeof(CMP_CompressOptions);
    options.fquality = 0.05f;
    options.dwnumThreads = 8;
    if (std::string(format_info->name) == "BC1a") {
        options.bDXT1UseAlpha = 1;
    }

    // Generate the input mipmap chain(s). At every level, the input
    // width and height must be padded up to a multiple of the output
    // block dimensions.
    for (int layer = 0; layer < (int)images.size(); ++layer) {
        int mip_width = base_width;
        int mip_height = base_height;
        int mip_pitch_x = ((mip_width + block_dim_x - 1) / block_dim_x) * block_dim_x;
        int mip_pitch_y = ((mip_height + block_dim_y - 1) / block_dim_y) * block_dim_y;

        auto &img = images[layer];
        img.input_mips.resize(mip_levels);
        img.output_mips.resize(mip_levels);
        // Populate padded input mip level 0
        img.input_mips[0].bytes.resize(mip_pitch_x * mip_pitch_y * input_components);
        // memset(img.input_mips[0].bytes.data(), 0, mip_pitch_x * mip_pitch_y * input_components);
        for (int y = 0; y < base_height; ++y) {
            memcpy(img.input_mips[0].bytes.data() + y * mip_width * input_components,
                    img.packed + y * base_width * input_components, base_width * input_components);
        }
        // Technically STBI_FREE() should suffice in both cases, but just to be safe...
        if (base_resize_enable) {
            STBI_FREE(img.packed);
        } else {
            stbi_image_free(img.packed);
        }
        img.packed = nullptr;
        // Generate additional mips, if necessary
        for (int mip = 1; mip < mip_levels; ++mip) {
            int src_width = mip_width;
            int src_height = mip_height;
            int src_pitch_x = mip_pitch_x;
            // int src_pitch_y = mip_pitch_y;
            mip_width = std::max(1, mip_width / 2);
            mip_height = std::max(1, mip_height / 2);
            mip_pitch_x = ((mip_width + block_dim_x - 1) / block_dim_x) * block_dim_x;
            mip_pitch_y = ((mip_height + block_dim_y - 1) / block_dim_y) * block_dim_y;
            img.input_mips[mip].bytes.resize(mip_pitch_x * mip_pitch_y * input_components);
            // memset(img.input_mips[mip].bytes.data(), 0, mip_pitch_x * mip_pitch_y * num_components);
            // printf("mip %u: width=%d height=%d\n", i, mip_width, mip_height);
            stbir_resize_uint8(img.input_mips[mip - 1].bytes.data(), src_width, src_height,
                    src_pitch_x * input_components, img.input_mips[mip].bytes.data(), mip_width, mip_height,
                    mip_pitch_x * input_components, input_components);
        }

        // Generate output mip chain
        mip_width = base_width;
        mip_height = base_height;
        for (int mip = 0; mip < mip_levels; ++mip) {
            CMP_Texture src_texture = {};
            src_texture.dwSize = sizeof(CMP_Texture);
            src_texture.dwWidth = ((mip_width + block_dim_x - 1) / block_dim_x) * block_dim_x;
            src_texture.dwHeight = ((mip_height + block_dim_y - 1) / block_dim_y) * block_dim_y;
            src_texture.dwPitch = src_texture.dwWidth * input_components;
            src_texture.format = CMP_FORMAT_RGBA_8888;
            src_texture.nBlockWidth = 1;
            src_texture.nBlockHeight = 1;
            src_texture.nBlockDepth = 1;
            src_texture.dwDataSize = CMP_CalculateBufferSize(&src_texture);
            src_texture.pData = img.input_mips[mip].bytes.data();
            qprintf("compressing mip %u layer %d: width=%d height=%d pitch_x=%d pitch_y=%d\n", mip, layer, mip_width,
                    mip_height, src_texture.dwWidth, src_texture.dwHeight);

            CMP_Texture dst_texture = {};
            dst_texture.dwSize = sizeof(CMP_Texture);
            dst_texture.dwWidth = src_texture.dwWidth;
            dst_texture.dwHeight = src_texture.dwHeight;
            dst_texture.dwPitch = 0;
            dst_texture.nBlockWidth = (CMP_BYTE)block_dim_x;
            dst_texture.nBlockHeight = (CMP_BYTE)block_dim_y;
            dst_texture.nBlockDepth = (CMP_BYTE)block_dim_z;
            dst_texture.format = format_info->cmp_format;
            dst_texture.dwDataSize = CMP_CalculateBufferSize(&dst_texture);
            output_mip_sizes[mip] = dst_texture.dwDataSize;
            img.output_mips[mip].bytes.resize(output_mip_sizes[mip]);
            dst_texture.pData = (CMP_BYTE *)img.output_mips[mip].bytes.data();

            CMP_ERROR cmp_status;
            cmp_status = CMP_ConvertTexture(&src_texture, &dst_texture, &options, nullptr);
            if (cmp_status != CMP_OK) {
                std::printf("Compression returned an error %d\n", cmp_status);
                return cmp_status;
            }

            mip_width = std::max(1, mip_width / 2);
            mip_height = std::max(1, mip_height / 2);
        }
    }

    // Output to KTX
    KtxHeader header = {};
    const uint8_t ktx_magic_id[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    memcpy(header.identifier, ktx_magic_id, 12);
    header.endianness = 0x04030201;
    header.glType = format_info->gl_type;
    header.glTypeSize = format_info->gl_type_size;
    header.glFormat = format_info->gl_format;
    header.glInternalFormat = format_info->internal_format;
    header.glBaseInternalFormat = format_info->base_format;
    header.pixelWidth = base_width;
    header.pixelHeight = base_height;
    header.pixelDepth = 0;  // must be 0 for 2D/cubemap textures
    uint32_t real_array_element_count = (uint32_t)(images.size() / (output_as_cubemap ? 6 : 1));
    // KTX spec says this field must be 0 for non-array textures
    header.numberOfArrayElements = (real_array_element_count > 1) ? real_array_element_count : 0;
    header.numberOfFaces = output_as_cubemap ? 6 : 1;
    header.numberOfMipmapLevels = mip_levels;
    header.bytesOfKeyValueData = 0;
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Error opening output '%s'\n", output_filename);
        return 3;
    }
    fwrite(&header, 1, sizeof(KtxHeader), output_file);
    uint32_t zero = 0;
    for (int mip = 0; mip < mip_levels; ++mip) {
        uint32_t image_size = (output_as_cubemap && header.numberOfArrayElements == 0)
                // Non-array cubemaps store the unpadded size of one face.
                ? output_mip_sizes[mip]
                // All others store the size of all elems/faces/slices for the whole mip.
                : output_mip_sizes[mip] * real_array_element_count;
        // printf("mip=%u offset=%u image_size=%u\n", mip, (uint32_t)ftell(output_file), image_size);
        fwrite(&image_size, 1, sizeof(uint32_t), output_file);
        for (uint32_t array_elem = 0; array_elem < real_array_element_count; ++array_elem) {
            for (uint32_t face = 0; face < header.numberOfFaces; ++face) {
                const auto &img = images[array_elem * header.numberOfFaces + face];
                // printf("mip=%u array=%u face=%u offset=%u size=%u\n", mip, array_elem, face,
                //        (uint32_t)ftell(output_file), output_mip_sizes[mip]);
                fwrite(img.output_mips[mip].bytes.data(), 1, output_mip_sizes[mip], output_file);
                if (output_as_cubemap && header.numberOfArrayElements == 0) {
                    uint32_t cube_padding = (4 - (ftell(output_file) % 4)) % 4;
                    fwrite(&zero, 1, cube_padding, output_file);
                }
            }
        }
        uint32_t mip_padding = (4 - (ftell(output_file) % 4)) % 4;
        fwrite(&zero, 1, mip_padding, output_file);
    }
    size_t output_file_size = ftell(output_file);
    fclose(output_file);
    qprintf("Wrote %s (format=%s, mips=%u, layers=%u, faces=%u, size=%u)\n", output_filename, output_format_name,
            mip_levels, real_array_element_count, header.numberOfFaces, (uint32_t)output_file_size);

    return 0;
}
