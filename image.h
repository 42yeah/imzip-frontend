#pragma once

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"
#include <memory>
#include <string>
#include <vector>

#define DEFAULT_BUF_SIZE 1024

using CComp = unsigned char; // CComp for color component

struct RGB
{
    CComp r;
    CComp g;
    CComp b;
};

struct CompressedInfo
{
    std::string file_name;
    char *buf;
    int buf_size;
    int ptr;

    CompressedInfo();
    ~CompressedInfo();

    void write(const char *what, int n);
};

class Image
{
public:
    /**
     * Default constructor
     */
    Image();

    Image(int w, int h, int ch);

    /**
     * Load image from file.
     * Returns false upon failure.
     */
    bool load(const std::string &path);

    unsigned int at(int x, int y) const;
    void set_rgb(int x, int y, CComp r, CComp g, CComp b);
    void set_rgb(int x, int y, const RGB &rgb);
    RGB get_rgb(int x, int y) const;
    bool save(const std::string &dest) const;
    bool save_compressed(const std::string &dest, int quality) const;
    std::shared_ptr<CompressedInfo> save_compressed_memory(int quality) const;

    /**
     * Resize the image using stb_image_resize.
     */
    bool resize(int nx, int ny);

    /**
     * Copy constructor
     *
     * @param other The other image, to be copied from.
     */
    Image(const Image& other);

    /**
     * Destructor
     */
    ~Image();

    int id() const;

    int w, h, ch;
    sg_image sg_image;
    simgui_image_t imgui_image;
    float aspect;
    std::string file_name;

private:
    std::unique_ptr<CComp[]> image;
    int id_;
    bool initialized;
};

CComp ccomp(float t);

/**
 * Generates a test gradient image.
 */
std::shared_ptr<Image> generate_gradient_image(int w, int h);

