//------------------------------------------------------------------------------
//  Simple C99 cimgui+sokol starter project for Win32, Linux and macOS.
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include <imgui.h>
#include "sokol_imgui.h"
#include <cmath>
#include <iostream>
#include <emscripten/emscripten.h>

static struct
{
    sg_pass_action pass_action;
    ImVec2 fold_size;
    int quality;
} state;

extern "C"
{
    // https://stackoverflow.com/questions/61496876/how-can-i-load-a-file-from-a-html-input-into-emscriptens-memfs-file-system
    void images_selected()
    {
        std::cout << "Images selected!" << std::endl;
    }

    int main(int argc, char *argv[]);
}

void imzip_ui()
{
    ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ fminf(800.0f, (float) sapp_width()), (float) sapp_height() }, ImGuiCond_Always);
    if (ImGui::Begin("Windowless", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
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

        if (ImGui::Button("Upload images ..."))
        {
            EM_ASM(
                document.querySelector("#file-input").click();
            );
            // std::cout << "Unbelievable." << std::endl;
        }
        ImGui::SameLine(); ImGui::TextWrapped("%d images selected.", 0);
    }
    ImGui::End();
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
    state.fold_size = { 2560.0f, 1440.0f };
    state.quality = 80;
}

static void frame(void)
{
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
        .window_title = "Hello Sokol + Dear ImGui"
    };
}
