// Copyright (c) 2020 - present, Roland Munguia
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

//#pragma once

#include <string>
#include "imConsole.h"
#include "imgui_internal.h"
#include <cstring>
#include <limits>
#include <fstream>

// The following three functions (InputTextCallback_UserData, InputTextCallback, InputText) are obtained from misc/cpp/imgui_stdlib.h
// Which are licensed under MIT License (https://github.com/ocornut/imgui/blob/master/LICENSE.txt)
namespace ImGui
{
    struct InputTextCallback_UserData
    {
        std::string *Str;
        ImGuiInputTextCallback ChainCallback;
        void *ChainCallbackUserData;
    };

    static int InputTextCallback(ImGuiInputTextCallbackData *data)
    {
        auto *user_data = (InputTextCallback_UserData *) data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            // Resize string callback
            // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
            std::string *str = user_data->Str;
            IM_ASSERT(data->Buf == str->c_str());
            str->resize(data->BufTextLen);
            data->Buf = (char *) str->c_str();
        }
        else if (user_data->ChainCallback)
        {
            // Forward to user callback, if any
            data->UserData = user_data->ChainCallbackUserData;
            return user_data->ChainCallback(data);
        }
        return 0;
    }

    bool InputText(const char *label, std::string *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *user_data)
    {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData cb_user_data;
        cb_user_data.Str = str;
        cb_user_data.ChainCallback = callback;
        cb_user_data.ChainCallbackUserData = user_data;
        return InputText(label, (char *) str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
    }
}

ImGuiConsole::ImGuiConsole(std::string c_name, size_t inputBufferSize) : m_ConsoleName(std::move(c_name))
{
    // Set input buffer size.
    m_Buffer.resize(inputBufferSize);
    m_HistoryIndex = std::numeric_limits<size_t>::min();

    // Specify custom data to be store/loaded from imgui.ini
    InitIniSettings();

    // Set Console ImGui default settings
    if (!m_LoadedFromIni)
    {
        DefaultSettings();
    }

    // Custom functions.
    RegisterConsoleCommands();

    test();
}

void ImGuiConsole::test() {
    std::ifstream file("lorem.txt");
    if (!file) { return; }

    std::string line;
    while (std::getline(file, line)) {
        Item item;
        item.str = line;
        lines.push_back(item);
    }

    file.close(); // Close the file
}

void ImGuiConsole::Draw()
{
    ///////////////////////////////////////////////////////////////////////////
    // Window and Settings ////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Begin Console Window.
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_WindowAlpha);
    if (!ImGui::Begin(m_ConsoleName.data(), nullptr, ImGuiWindowFlags_MenuBar))
    {
        ImGui::PopStyleVar();
        ImGui::End();
        return;
    }
    ImGui::PopStyleVar();

    ///////////////
    // Menu bar  //
    ///////////////
    MenuBar();

    ////////////////
    // Filter bar //
    ////////////////
    if (m_FilterBar)
    { FilterBar(); }

    //////////////////
    // Console Logs //
    //////////////////
    LogWindow();

    // Section off.
    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////////
    // Command-line ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    InputBar();

    ImGui::End();
}

void ImGuiConsole::InitIniSettings()
{
    ImGuiContext &g = *ImGui::GetCurrentContext();

    // Load from .ini
    if (g.Initialized && !g.SettingsLoaded && !m_LoadedFromIni)
    {
        ImGuiSettingsHandler console_ini_handler;
        console_ini_handler.TypeName = "imgui-console";
        console_ini_handler.TypeHash = ImHashStr("imgui-console");
        console_ini_handler.ClearAllFn = SettingsHandler_ClearALl;
        console_ini_handler.ApplyAllFn = SettingsHandler_ApplyAll;
        console_ini_handler.ReadInitFn = SettingsHandler_ReadInit;
        console_ini_handler.ReadOpenFn = SettingsHandler_ReadOpen;
        console_ini_handler.ReadLineFn = SettingsHandler_ReadLine;
        console_ini_handler.WriteAllFn = SettingsHandler_WriteAll;
        console_ini_handler.UserData = this;
        g.SettingsHandlers.push_back(console_ini_handler);
    }
    // else Ini settings already loaded!
}

void ImGuiConsole::DefaultSettings()
{
    // Settings
    m_AutoScroll = true;
    m_ScrollToBottom = false;
    m_ColoredOutput = true;
    m_FilterBar = true;
    m_TimeStamps = true;

    // Style
    m_WindowAlpha = 1;
    m_ColorPalette[COL_COMMAND] = ImVec4(1.f, 1.f, 1.f, 1.f);
    m_ColorPalette[COL_LOG] = ImVec4(1.f, 1.f, 1.f, 0.5f);
    m_ColorPalette[COL_WARNING] = ImVec4(1.0f, 0.87f, 0.37f, 1.f);
    m_ColorPalette[COL_ERROR] = ImVec4(1.f, 0.365f, 0.365f, 1.f);
    m_ColorPalette[COL_INFO] = ImVec4(0.46f, 0.96f, 0.46f, 1.f);
    m_ColorPalette[COL_TIMESTAMP] = ImVec4(1.f, 1.f, 1.f, 0.5f);
}

void ImGuiConsole::RegisterConsoleCommands()
{
}

void ImGuiConsole::FilterBar()
{
    m_TextFilter.Draw("Filter", ImGui::GetWindowWidth() * 0.25f);
    ImGui::Separator();
}

void ImGuiConsole::LogWindow()
{
    const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollRegion##", ImVec2(0, -footerHeightToReserve), false, 0))
    {
        // Wrap items.
        ImGui::PushTextWrapPos();

        // Display items.
        for (const auto &item : lines)
        {
            // Exit if word is filtered.
            if (!m_TextFilter.PassFilter(item.str.c_str()))
                continue;

            // Items.
            if (item.colID > -1) {
                ImGui::PushStyleColor(ImGuiCol_Text, m_ColorPalette[item.colID]);
                ImGui::TextUnformatted(item.str.c_str());
                ImGui::PopStyleColor();
            }
            else ImGui::TextUnformatted(item.str.c_str());
        }

        // Stop wrapping since we are done displaying console items.
        ImGui::PopTextWrapPos();

        // Auto-scroll logs.
        if ((m_ScrollToBottom && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() || m_AutoScroll)))
            ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;

        // Loop through command string vector.
        ImGui::EndChild();
    }
}

void ImGuiConsole::InputBar()
{

}

void ImGuiConsole::MenuBar()
{

}

// From imgui_demo.cpp
void ImGuiConsole::HelpMaker(const char *desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void ImGuiConsole::SettingsHandler_ClearALl(ImGuiContext *ctx, ImGuiSettingsHandler *handler)
{
}

void ImGuiConsole::SettingsHandler_ReadInit(ImGuiContext *ctx, ImGuiSettingsHandler *handler)
{
}

void *ImGuiConsole::SettingsHandler_ReadOpen(ImGuiContext *ctx, ImGuiSettingsHandler *handler, const char *name)
{
    if (!handler->UserData)
        return nullptr;

    auto console = static_cast<ImGuiConsole *>(handler->UserData);

    if (strcmp(name, console->m_ConsoleName.c_str()) != 0)
        return nullptr;
    return (void *) 1;
}

void ImGuiConsole::SettingsHandler_ReadLine(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line)
{
    if (!handler->UserData)
        return;

    // Get console.
    auto console = static_cast<ImGuiConsole *>(handler->UserData);

    // Ensure console doesn't reset variables.
    console->m_LoadedFromIni = true;

// Disable warning regarding sscanf when using MVSC
#pragma warning( push )
#pragma warning( disable:4996 )

#define INI_CONSOLE_LOAD_COLOR(type) (std::sscanf(line, #type"=%i,%i,%i,%i", &r, &g, &b, &a) == 4) { console->m_ColorPalette[type] = ImColor(r, g, b, a); }
#define INI_CONSOLE_LOAD_FLOAT(var) (std::sscanf(line, #var"=%f", &f) == 1) { console->var = f; }
#define INI_CONSOLE_LOAD_BOOL(var) (std::sscanf(line, #var"=%i", &b) == 1) {console->var = b == 1;}

    float f;
    int r, g, b, a;

    // Window style/visuals
    if INI_CONSOLE_LOAD_COLOR(COL_COMMAND)
    else if INI_CONSOLE_LOAD_COLOR(COL_LOG)
    else if INI_CONSOLE_LOAD_COLOR(COL_WARNING)
    else if INI_CONSOLE_LOAD_COLOR(COL_ERROR)
    else if INI_CONSOLE_LOAD_COLOR(COL_INFO)
    else if INI_CONSOLE_LOAD_COLOR(COL_TIMESTAMP)
    else if INI_CONSOLE_LOAD_FLOAT(m_WindowAlpha)

        // Window settings
    else if INI_CONSOLE_LOAD_BOOL(m_AutoScroll)
    else if INI_CONSOLE_LOAD_BOOL(m_ScrollToBottom)
    else if INI_CONSOLE_LOAD_BOOL(m_ColoredOutput)
    else if INI_CONSOLE_LOAD_BOOL(m_FilterBar)
    else if INI_CONSOLE_LOAD_BOOL(m_TimeStamps)

#pragma warning( pop )
}

void ImGuiConsole::SettingsHandler_ApplyAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler)
{
    if (!handler->UserData)
        return;
}

void ImGuiConsole::SettingsHandler_WriteAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
    if (!handler->UserData)
        return;

    // Get console.
    auto console = static_cast<ImGuiConsole *>(handler->UserData);

#define INI_CONSOLE_SAVE_COLOR(type) buf->appendf(#type"=%i,%i,%i,%i\n", (int)(console->m_ColorPalette[type].x * 255),\
                                                                         (int)(console->m_ColorPalette[type].y * 255),\
                                                                         (int)(console->m_ColorPalette[type].z * 255),\
                                                                         (int)(console->m_ColorPalette[type].w * 255))

#define INI_CONSOLE_SAVE_FLOAT(var) buf->appendf(#var"=%.3f\n", console->var)
#define INI_CONSOLE_SAVE_BOOL(var) buf->appendf(#var"=%i\n", console->var)

    // Set header for CONSOLE Console.
    buf->appendf("[%s][%s]\n", handler->TypeName, console->m_ConsoleName.data());

    // Window settings.
    INI_CONSOLE_SAVE_BOOL(m_AutoScroll);
    INI_CONSOLE_SAVE_BOOL(m_ScrollToBottom);
    INI_CONSOLE_SAVE_BOOL(m_ColoredOutput);
    INI_CONSOLE_SAVE_BOOL(m_FilterBar);
    INI_CONSOLE_SAVE_BOOL(m_TimeStamps);

    // Window style/visuals
    INI_CONSOLE_SAVE_FLOAT(m_WindowAlpha);
    INI_CONSOLE_SAVE_COLOR(COL_COMMAND);
    INI_CONSOLE_SAVE_COLOR(COL_LOG);
    INI_CONSOLE_SAVE_COLOR(COL_WARNING);
    INI_CONSOLE_SAVE_COLOR(COL_ERROR);
    INI_CONSOLE_SAVE_COLOR(COL_INFO);
    INI_CONSOLE_SAVE_COLOR(COL_TIMESTAMP);

    // End saving.
    buf->append("\n");
}
