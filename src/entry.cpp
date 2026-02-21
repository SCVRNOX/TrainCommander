#include <fstream>
#include <string>

#include "imgui/imgui.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"

#include "base64.h"
#include "editor_ui.h"
#include "event_catalog.h"
#include "event_ui.h"
#include "overlay_ui.h"
#include "premium_icon.h"
#include "train_manager.h"

// Prototypes
void AddonLoad(AddonAPI_t *aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();

/* Globals */
AddonDefinition_t AddonDef = {};
HMODULE hSelf = nullptr;
AddonAPI_t *APIDefs = nullptr;
NexusLinkData_t *NexusLink = nullptr;
Mumble::Data *MumbleLink = nullptr;

TrainManager *g_Manager = nullptr;
EventCatalog *g_Catalog = nullptr;
EditorUI *g_EditorUI = nullptr;
OverlayUI *g_OverlayUI = nullptr;
EventUI *g_EventUI = nullptr;

// Main entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    hSelf = hModule;
    break;
  case DLL_PROCESS_DETACH:
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}

// Export for Nexus
extern "C" __declspec(dllexport) AddonDefinition_t *GetAddonDef() {
  AddonDef.Signature = 1005128; // Unique ID
  AddonDef.APIVersion = NEXUS_API_VERSION;
  AddonDef.Name = "Train Commander";
  AddonDef.Version.Major = 1;
  AddonDef.Version.Minor = 0;
  AddonDef.Version.Build = 0;
  AddonDef.Version.Revision = 0;
  AddonDef.Author = "SCVRNOX";
  AddonDef.Description = "Manage and track farm trains internally.";
  AddonDef.Load = AddonLoad;
  AddonDef.Unload = AddonUnload;
  AddonDef.Flags = AF_None;
  AddonDef.Provider = UP_GitHub;
  AddonDef.UpdateLink = "https://github.com/SCVRNOX/TrainCommander";

  return &AddonDef;
}

// Setup the addon
void AddonLoad(AddonAPI_t *aApi) {
  APIDefs = aApi; // store the api

  ImGui::SetCurrentContext((ImGuiContext *)APIDefs->ImguiContext);
  ImGui::SetAllocatorFunctions((void *(*)(size_t, void *))APIDefs->ImguiMalloc,
                               (void (*)(void *, void *))APIDefs->ImguiFree);

  NexusLink = (NexusLinkData_t *)APIDefs->DataLink_Get("DL_NEXUS_LINK");
  MumbleLink = (Mumble::Data *)APIDefs->DataLink_Get("DL_MUMBLE_LINK");

  // Setup Addon Directory
  std::string addonDir = APIDefs->Paths_GetAddonDirectory("TrainCommander");

  g_Manager = new TrainManager(addonDir);
  g_Manager->LoadTrains();

  g_Catalog = new EventCatalog(addonDir);

  g_EventUI = new EventUI(APIDefs, g_Manager, g_Catalog);
  g_EditorUI = new EditorUI(APIDefs, g_Manager, g_EventUI);
  g_OverlayUI = new OverlayUI(APIDefs, g_Manager, g_Catalog);

  // Register keybind for settings
  APIDefs->InputBinds_RegisterWithString(
      "KB_TRAIN_EDITOR",
      [](const char *id, bool isRelease) {
        if (!isRelease && g_EditorUI) {
          g_EditorUI->Toggle();
        }
      },
      "SHIFT+ALT+T");

  // Setup QuickAccess Icon
  const char *iconId = "TC_ICON";
  std::string rawIcon = Base64::Decode(PREMIUM_ICON_B64);
  if (APIDefs->Textures_GetOrCreateFromMemory) {
    APIDefs->Textures_GetOrCreateFromMemory(iconId, (void *)rawIcon.data(),
                                            rawIcon.size());
  }

  // Cleanup old file-based icon if it exists
  std::string iconPath = addonDir + "\\icon.png";
  if (GetFileAttributesA(iconPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
    DeleteFileA(iconPath.c_str());
  }

  // Load fallback memory texture
  std::string base64RedPng = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAAD"
                             "UlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";
  std::string rawRedPng = Base64::Decode(base64RedPng);
  if (APIDefs->Textures_GetOrCreateFromMemory) {
    APIDefs->Textures_GetOrCreateFromMemory(
        "TC_ICON_RED", (void *)rawRedPng.data(), rawRedPng.size());
  }

  // Register QuickAccess Button
  if (APIDefs->QuickAccess_Add) {
    APIDefs->QuickAccess_Add("TC_SHORTCUT", iconId, iconId, "KB_TRAIN_EDITOR",
                             "Train Commander");
  }

  // Register Renderer
  APIDefs->GUI_Register(RT_Render, AddonRender);
  APIDefs->GUI_Register(RT_OptionsRender, AddonOptions);

  APIDefs->Log(LOGL_INFO, "TrainCommander", "Addon loaded successfully.");
}

// Cleanup on unload
void AddonUnload() {
  APIDefs->GUI_Deregister(AddonRender);
  APIDefs->GUI_Deregister(AddonOptions);

  if (APIDefs->QuickAccess_Remove) {
    APIDefs->QuickAccess_Remove("TC_SHORTCUT");
  }

  if (g_Manager) {
    g_Manager->SaveTrains();
    delete g_Manager;
    g_Manager = nullptr;
  }

  if (g_EditorUI) {
    delete g_EditorUI;
    g_EditorUI = nullptr;
  }

  if (g_OverlayUI) {
    delete g_OverlayUI;
    g_OverlayUI = nullptr;
  }

  if (g_EventUI) {
    delete g_EventUI;
    g_EventUI = nullptr;
  }

  if (g_Catalog) {
    delete g_Catalog;
    g_Catalog = nullptr;
  }

  APIDefs->Log(LOGL_INFO, "TrainCommander", "Addon unloaded.");
}

// Render loop
void AddonRender() {
  if (g_EditorUI) {
    g_EditorUI->Render();
  }

  if (g_OverlayUI) { // Let Overlay UI decide if it has something to show
    g_OverlayUI->Render();
  }

  if (g_EventUI) {
    g_EventUI->Render();
  }
}

// Addon options in Nexus menu
void AddonOptions() {
  ImGui::Text("Train Commander Options");
  ImGui::Separator();
  if (ImGui::Button("Open Train Editor")) {
    if (g_EditorUI) {
      g_EditorUI->Show();
    }
  }
}
