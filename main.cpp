//------------------------------------------------------------------------------
//  Simple C99 cimgui+sokol starter project for Win32, Linux and macOS.
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include <imgui.h>
#include "sokol_imgui.h"

static struct
{
    sg_pass_action pass_action;
} state;

static void init(void)
{
    sg_desc desc = { .context = sapp_sgcontext() };
    simgui_desc_t simgui_desc = { 0 };
    sg_setup(&desc);
    simgui_setup(&simgui_desc);

    // initial clear color
    state.pass_action.colors[0] = {
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = { 1.0f, 1.0f, 1.0f, 1.0f }
    };
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
    ImGui::SetNextWindowPos({ 10.0f, 10.0f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ 400.0f, 100.0f }, ImGuiCond_Once);
    if (ImGui::Begin("Hello ImGui!", nullptr, ImGuiWindowFlags_None))
    {
        ImGui::ColorEdit3("Background", &state.pass_action.colors[0].clear_value.r, ImGuiColorEditFlags_None);
    }
    ImGui::End();
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
