//------------------------------------------------------------------------------
//  Simple C99 cimgui+sokol starter project for Win32, Linux and macOS.
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include <cstdio>
#include <imgui.h>
#include "sokol_imgui.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>
#include <emscripten/emscripten.h>
#include <miniz.h>
#include "image.h"

#define WINDOW_WIDTH 800.0f
#define DEFAULT_QUALITY 60
#define DEFAULT_FOLD_W 2560.0f
#define DEFAULT_FOLD_H 1440.0f
#define IMAGE_LINE 6 // allow for 4 images on the same line, tops
#define EVENT_FRAME_DRAG 5

struct ImageInfo
{
    int image_idx;
    bool display;
};

static struct
{
    sg_pass_action pass_action;
    ImVec2 fold_size;
    int quality;
    std::vector<std::string> files;
    std::vector<std::shared_ptr<Image> > images;
    std::vector<ImageInfo> image_windows;
    char sprinted[1024];
    char archive_name[1024];
    int event_frame;
} state;

EM_JS(void, download, (const char *path), {
    const pathStr = UTF8ToString(path).replace("/", "");
    console.log("Downloading:", pathStr);
    const data = window.Module["FS"].readFile("/" + pathStr);
    const blob = new Blob([data]);
    const url = window.URL.createObjectURL(blob);
    const downloadEl = document.querySelector("#download");
    downloadEl.download = pathStr;
    downloadEl.href = url;
    downloadEl.click();
});

void compress_all_images()
{
    std::vector<std::shared_ptr<CompressedInfo> > infos;
    for (int i = 0; i < state.images.size(); i++)
    {
        Image im(*state.images[i]);
        bool failed = false;
        while (im.w >= state.fold_size.x && im.h >= state.fold_size.y)
        {
            if (!im.resize(im.w / 2, im.h / 2))
            {
                failed = true;
                break;
            }
        }
        if (failed)
        {
            continue;
        }

        std::shared_ptr<CompressedInfo> cpr = im.save_compressed_memory(state.quality);
        if (cpr->ptr == 0)
        {
            continue;
        }
        infos.push_back(cpr);
    }

    // Why is this going the other way around ???
    mz_bool status = MZ_TRUE;
    mz_zip_error err;

    remove(state.archive_name);
    char archive_path[1024];
    sprintf(archive_path, "/%s", state.archive_name);

    for (int i = 0; i < infos.size(); i++)
    {
        sprintf(state.sprinted, "%s_cmpr.jpg", infos[i]->file_name.c_str());

        status = mz_zip_add_mem_to_archive_file_in_place_v2(archive_path, state.sprinted,
                                                            infos[i]->buf, infos[i]->ptr,
                                                            nullptr, 0, MZ_BEST_COMPRESSION, &err);

        if (status == MZ_FALSE)
        {
            // TODO: handle error
            std::cerr << "Cannot compress? " << mz_zip_get_error_string(err) << std::endl;
            break;
        }
    }

    if (status == MZ_FALSE)
    {
        return;
    }

    download(archive_path);
}

void compress_one_image(const std::shared_ptr<Image> &im)
{
    Image im_copy(*im);
    bool failed = false;
    while (im_copy.w >= state.fold_size.x && im_copy.h >= state.fold_size.y)
    {
        if (!im_copy.resize(im_copy.w / 2, im_copy.h / 2))
        {
            failed = true;
            break;
        }
    }
    if (failed)
    {
        return;
    }

    std::string path = "/";
    path += im_copy.file_name + "_cmpr.jpg";
    im_copy.save_compressed(path, state.quality);
    download(path.c_str());
}

extern "C"
{
    // https://stackoverflow.com/questions/61496876/how-can-i-load-a-file-from-a-html-input-into-emscriptens-memfs-file-system
    void images_selected()
    {
        // state.image_windows.clear();
        // state.images.clear();
        // state.files.clear();

        std::ifstream reader("info.txt");
        if (!reader.good())
        {
            std::cerr << "Bad reader: info.txt?" << std::endl;
        }
        std::string path;
        while (std::getline(reader, path))
        {
            state.files.push_back(path);
            std::shared_ptr<Image> img(new Image());
            if (!img->load(path))
            {
                // TODO: a better error
                std::cerr << "Cannot load: " << path << "?" << std::endl;
            }
            std::cout << "Image loaded: " << img->w << ", " << img->h << std::endl;
            state.images.push_back(img);
        }

        state.event_frame = EVENT_FRAME_DRAG;
    }

    int main(int argc, char *argv[]);
}

void imzip_ui()
{
    ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ fminf(WINDOW_WIDTH, (float) sapp_width()), (float) sapp_height() }, ImGuiCond_Always);
    if (ImGui::Begin("Windowless", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus
    ))
    {
        ImGui::TextWrapped("IMZIP by 42yeah");
        ImGui::Separator();
        ImGui::TextWrapped("ImZip is an online image compression utility written by me, 42yeah. It is written in WebAssembly and is therefore crazy fast. Drag & drop photos here and download their compressed versions, in real time.");

        if (ImGui::CollapsingHeader("How imzip works"))
        {
            ImGui::TextWrapped("ImZip performs easy, performant image compression by doing the following to each input images:");
            ImGui::TextWrapped("1. If the image is bigger than the 'threshold' (%.0f, %.0f), the image size is halved;", state.fold_size.x, state.fold_size.y);
            ImGui::TextWrapped("2. The image is re-encoded via the JPG encoder by stb (quality: %d)", state.quality);
        }
        if (ImGui::CollapsingHeader("Compression Configs"))
        {
            ImGui::SliderInt("JPG quality", &state.quality, 10, 100);
            ImGui::InputFloat2("Fold when size exceeds", &state.fold_size.x);
            ImGui::InputText("Archive name", state.archive_name, sizeof(state.archive_name));
        }

        if (ImGui::Button("Upload images ..."))
        {
            EM_ASM(
                if (navigator.userAgent.indexOf("iPhone OS") != -1 ||
                    (navigator.userAgent.indexOf("Intel Mac OS X") != -1) && navigator.userAgent.indexOf("Chrome") == -1) {
                    document.querySelector(".fuck-safari").classList.remove("hidden");
                }
                document.querySelector("#file-input").click();
            );
        }
        if (state.images.size() > 0)
        {
            ImGui::SameLine();
            if (ImGui::Button("Compress!"))
            {
                compress_all_images();
            }
        }
        ImGui::SameLine(); ImGui::TextWrapped("%d images selected.", (int) state.files.size());

        if (state.images.size() > 0)
        {
            ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.0f }, "GALLERY");
            if (ImGui::Button("Clear all"))
            {
                state.images.clear();
                state.files.clear();
                state.image_windows.clear();
                state.event_frame = EVENT_FRAME_DRAG;
            }

            float sum_width = 0.0f;
            int num_lines = 0;
            for (int i = 0; i < state.images.size(); i++)
            {
                sum_width += state.images[i]->w;
                if (i != 0 && i % IMAGE_LINE == 0)
                {
                    num_lines++;
                }
            }

            for (int i = 0; i < state.images.size(); i++)
            {
                sprintf(state.sprinted, "##IM %d", i);
                const std::shared_ptr<Image> &im = state.images[i];
                float w = fminf((float) WINDOW_WIDTH, sapp_widthf()) / IMAGE_LINE;
                if (ImGui::ImageButton(state.sprinted, simgui_imtextureid(im->imgui_image), { w, w / im->aspect }, { 0.0f, 0.0f }, { 1.0f, 1.0f }))
                {
                    auto found = std::find_if(state.image_windows.begin(), state.image_windows.end(), [&](const ImageInfo &info)
                    {
                        return info.image_idx == i;
                    });
                    if (found == state.image_windows.end())
                    {
                        state.image_windows.push_back({ .image_idx = i, .display = true });
                        state.event_frame = EVENT_FRAME_DRAG;
                    }
                }
                if ((i + 1) % IMAGE_LINE != 0)
                {
                    ImGui::SameLine();
                }
            }
        }
    }
    ImGui::End();

    int negative_offset = 0;
    for (auto it = state.image_windows.begin(); it != state.image_windows.end(); )
    {
        // Correct the index if image windows are deleted
        it->image_idx -= negative_offset;

        if (it->image_idx < 0 || it->image_idx >= state.images.size())
        {
            std::cout << "Erasure:" << it->image_idx << ", " << state.images.size() << std::endl;
            it = state.image_windows.erase(it);
            continue;
        }

        const std::shared_ptr<Image> &im = state.images[it->image_idx];
        sprintf(state.sprinted, "Image %s", im->file_name.c_str());
        if (ImGui::Begin(state.sprinted, &it->display))
        {
            ImGui::TextWrapped("Image dimension: %dx%d", im->w, im->h);
            ImGui::TextWrapped("Image size (in memory: %ld)", (long) (im->w * im->h) * im->ch);

            if (ImGui::Button("Download"))
            {
                compress_one_image(im);
            }

            if (ImGui::Button("Remove"))
            {
                it->display = false;
                state.images.erase(state.images.begin() + it->image_idx);
                state.files.erase(state.files.begin() + it->image_idx);
            }

            float w = fminf(sapp_widthf(), fminf(WINDOW_WIDTH, im->w * 0.5f));

            if (it->display)
                ImGui::Image(simgui_imtextureid(im->imgui_image), { w, w / im->aspect }, { 0.0f, 0.0f }, { 1.0f, 1.0f });
        }
        ImGui::End();

        if (!it->display)
        {
            it = state.image_windows.erase(it);
            state.event_frame = EVENT_FRAME_DRAG;
            negative_offset++;
            continue;
        }

        it++;
    }
}

static void init(void)
{
    sg_desc desc = { .context = sapp_sgcontext() };
    simgui_desc_t simgui_desc = { 0 };
    sg_setup(&desc);
    simgui_setup(&simgui_desc);

    // initial clear color
    state.pass_action.colors[0] = {
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = { 0.1f, 0.1f, 0.2f, 1.0f }
    };
    state.fold_size = { DEFAULT_FOLD_W, DEFAULT_FOLD_H };
    state.quality = DEFAULT_QUALITY;
    sprintf(state.archive_name, "compressed.zip");
    state.event_frame = EVENT_FRAME_DRAG;
}

static void frame(void)
{
    if (state.event_frame != 0)
    {
        state.event_frame--;
    }
    else
    {
        return;
    }

    simgui_frame_desc_t new_frame_desc = {
        .width = sapp_width(),
        .height = sapp_height(),
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    };
    simgui_new_frame(&new_frame_desc);

    /*=== UI CODE STARTS HERE ===*/
    imzip_ui();
    /*=== UI CODE ENDS HERE ===*/

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    simgui_render();
    sg_end_pass();
    sg_commit();
}

static void cleanup(void)
{
    simgui_shutdown();
    sg_shutdown();
}

static void event(const sapp_event* ev)
{
    state.event_frame = EVENT_FRAME_DRAG;
    simgui_handle_event(ev);
}

sapp_desc sokol_main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .window_title = "IMZIP by 42yeah"
    };
}
