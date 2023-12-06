// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: 2023 42yeah <email>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "image.h"
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"

#define FLIP_VERTICAL false

int id = 0;

CompressedInfo::CompressedInfo() : file_name(""), buf(nullptr), buf_size(0), ptr(0)
{
    buf = new char[DEFAULT_BUF_SIZE];
    buf_size = DEFAULT_BUF_SIZE;
    ptr = 0;
}

void CompressedInfo::write(const char *what, int n)
{
    if (n + ptr > buf_size)
    {
        buf_size *= 2;
        char *new_buf = new char[buf_size];
        memcpy(new_buf, buf, ptr);
        delete buf;
        buf = new_buf;
    }
    memcpy(&buf[ptr], what, n);
    ptr += n;
}


CompressedInfo::~CompressedInfo()
{
    if (buf != nullptr)
    {
        delete buf;
    }
}

Image::Image() : w(0), h(0), ch(4), file_name(""), image(nullptr), id_(::id++), initialized(false)
{

}

Image::Image(int w, int h, int ch) : w(w), h(h), ch(ch), file_name(""), image(nullptr), id_(::id++), initialized(false)
{
    assert((ch == 3 || ch == 4) && "Unsupported numer of channels");
    image.reset(new CComp[w * h * ch]);
    aspect = (float) w / h;
    initialized = true;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            for (int i = 0; i < ch; i++)
            {
                image[at(x, y) + i] = 0;
            }
        }
    }

    sg_image_desc desc = {
        .width = w,
        .height = h,
        .pixel_format = SG_PIXELFORMAT_RGBA8, // Hmm...
        .sample_count = 1,
    };
    desc.data.subimage[0][0] = {
        .ptr = image.get(),
        .size = (size_t) (w * h * ch)
    };
    sg_image = sg_make_image(&desc);

    sg_sampler_desc sam_desc = {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT
    };

    simgui_image_desc_t imgui_desc = {
        .image = sg_image,
        .sampler = sg_make_sampler(&sam_desc)
    };
    imgui_image = simgui_make_image(&imgui_desc);

    initialized = true;
}

unsigned int Image::at(int x, int y) const
{
    assert(initialized && "Image is not initialized");

    if (x < 0) { x = 0; }
    if (y < 0) { y = 0; }
    if (x >= w) { x = w - 1; }
    if (y >= h) { y = h - 1; }
    return (y * w + x) * ch;
}

void Image::set_rgb(int x, int y, CComp r, CComp g, CComp b)
{
    assert(initialized && "Image is not initialized");

    image[at(x, y) + 0] = r;
    image[at(x, y) + 1] = g;
    image[at(x, y) + 2] = b;

    if (ch == 4)
    {
        image[at(x, y) + 3] = 255;
    }
}

void Image::set_rgb(int x, int y, const RGB &rgb)
{
    assert(initialized && "Image is not initialized");

    set_rgb(x, y, rgb.r, rgb.g, rgb.b);
}

RGB Image::get_rgb(int x, int y) const
{
    assert(initialized && "Image is not initialized");

    RGB ret;
    ret.r = image[at(x, y) + 0];
    ret.g = image[at(x, y) + 1];
    ret.b = image[at(x, y) + 2];
    return ret;
}

Image::Image(const Image& other)
{
    w = other.w;
    h = other.h;
    ch = other.ch;
    file_name = other.file_name;
    initialized = other.initialized;
    image.reset(new CComp[w * h * ch]);
    std::memcpy(image.get(), other.image.get(), sizeof(CComp) * w * h * ch);
}

bool Image::save(const std::string &dest) const
{
    assert(initialized && "Image is not initialized");

    stbi_flip_vertically_on_write(FLIP_VERTICAL);
    int res = stbi_write_png(dest.c_str(), w, h, ch, image.get(), w * ch);

    return res != 0;
}

bool Image::save_compressed(const std::string &dest, int quality) const
{
    assert(initialized && "Image is not initialized");

    stbi_flip_vertically_on_write(FLIP_VERTICAL);
    int res = stbi_write_jpg(dest.c_str(), w, h, ch, image.get(), quality);

    return res != 0;
}

void stbi_jpg_func(void *ctx, void *data, int size)
{
    CompressedInfo *info = (CompressedInfo *) ctx;
    info->write((const char *) data, size);
}

std::shared_ptr<CompressedInfo> Image::save_compressed_memory(int quality) const
{
    assert(initialized && "Image is not initialized");

    std::shared_ptr<CompressedInfo> info(new CompressedInfo());
    info->file_name = file_name;

    stbi_flip_vertically_on_write(FLIP_VERTICAL);
    int res = stbi_write_jpg_to_func(stbi_jpg_func, (void *) info.get(), w, h, ch, image.get(), quality);

    if (res == 0)
    {
        std::cerr << "ERR! Cannot write JPG " << file_name << " to func." << std::endl;
        return std::shared_ptr<CompressedInfo>(new CompressedInfo());
    }
    return info;
}

Image::~Image()
{

}

CComp ccomp(float t)
{
    CComp c = (CComp) (t * 255.0f);
    return c;
}

std::shared_ptr<Image> generate_gradient_image(int w, int h)
{
    std::shared_ptr<Image> ret = std::make_shared<Image>(w, h, 4);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            float r = ((float) x / w);
            float g = ((float) y / h);
            ret->set_rgb(x, y, ccomp(r), ccomp(g), 0);
        }
    }
    return ret;
}

int Image::id() const
{
    return id_;
}

bool Image::load(const std::string &path)
{
    stbi_set_flip_vertically_on_load(FLIP_VERTICAL);

    unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    assert((ch == 3 || ch == 4) && "Unsupported number of channels");

    if (!data)
    {
        return false;
    }

    if (ch == 3)
    {
        ch = 4;
        image.reset(new CComp[w * h * ch]);
        int src_offset = 0, dest_offset = 0;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                src_offset = (y * w + x) * 3;
                dest_offset = (y * w + x) * 4;
                image[dest_offset + 0] = data[src_offset + 0];
                image[dest_offset + 1] = data[src_offset + 1];
                image[dest_offset + 2] = data[src_offset + 2];
                image[dest_offset + 3] = (CComp) 255;
            }
        }
        stbi_image_free(data);
        initialized = true;
    }
    else
    {
        image.reset(data);
    }

    aspect = (float) w / h;
    file_name = path;

    // ???
    if (file_name.size() == 0)
    {
        return false;
    }
    if (file_name[0] == '/')
    {
        file_name = file_name.substr(1, file_name.size());
    }

    sg_image_desc desc = {
        .width = w,
        .height = h,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    };
    desc.data.subimage[0][0] = {
        .ptr = image.get(),
        .size = (size_t) (w * h * ch)
    };
    sg_image = sg_make_image(&desc);

    sg_sampler_desc sam_desc = {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT
    };

    simgui_image_desc_t imgui_desc = {
        .image = sg_image,
        .sampler = sg_make_sampler(&sam_desc)
    };
    imgui_image = simgui_make_image(&imgui_desc);

    initialized = true;

    return true;
}

bool Image::resize(int nx, int ny)
{
    // 1. Allocate needed amount of memory
    std::unique_ptr<unsigned char[]> ptr(new unsigned char[nx * ny * ch]);
    int ok = stbir_resize_uint8(image.get(), w, h, 0, ptr.get(), nx, ny, 0, ch);

    if (ok == 0)
    {
        return false;
    }

    w = nx;
    h = ny;
    image = std::move(ptr);
    return true;
}
