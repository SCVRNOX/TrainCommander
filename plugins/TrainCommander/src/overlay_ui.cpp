#include "overlay_ui.h"
#include "imgui/imgui.h"
#include <windows.h>
#include <chrono>
#include <ctime>
#include <string>
#include <vector>
#include <cstring>
#include "nlohmann_json.hpp"

using json = nlohmann::json;
#include <fstream>

static bool PostStringToChat(AddonAPI_t *api, const std::string &text, bool asCommander = true) {
  if (!api)
    return false;

  if (!api->GameBinds_Press || !api->GameBinds_Release ||
      !api->WndProc_SendToGameOnly)
    return false;

  // Focus the appropriate chat input without blocking: prefer async invoke
  if (asCommander) {
    if (api->GameBinds_InvokeAsync)
      api->GameBinds_InvokeAsync(GB_UiSquadBroadcastChatFocus, 50);
    else {
      api->GameBinds_Press(GB_UiSquadBroadcastChatFocus);
      api->GameBinds_Release(GB_UiSquadBroadcastChatFocus);
    }
  } else {
    if (api->GameBinds_InvokeAsync)
      api->GameBinds_InvokeAsync(GB_UiChatFocus, 50);
    else {
      api->GameBinds_Press(GB_UiChatFocus);
      api->GameBinds_Release(GB_UiChatFocus);
    }
  }

  // Build the final string. Prefix with commander chat if requested.
  std::string out = text;
  if (asCommander) {
    // Many servers accept squad chat as commander channel; use /s prefix
    out = std::string("/s ") + out;
  }

  // Convert UTF-8 std::string to UTF-16 so we send proper wide characters
  int req = MultiByteToWideChar(CP_UTF8, 0, out.c_str(), (int)out.size(), NULL, 0);
  if (req <= 0)
    return false;
  std::wstring w;
  w.resize(req);
  MultiByteToWideChar(CP_UTF8, 0, out.c_str(), (int)out.size(), &w[0], req);

  // Send each wide char as WM_CHAR without sleeping on the UI thread
  for (wchar_t wc : w) {
    if (wc == 0)
      break;
    api->WndProc_SendToGameOnly(NULL, WM_CHAR, (WPARAM)wc, 0);
  }

  // If using squad/commander mode, trigger the squad broadcast command (async when available)
  if (asCommander) {
    if (api->GameBinds_InvokeAsync)
      api->GameBinds_InvokeAsync(GB_UiSquadBroadcastChatCommand, 50);
    else {
      api->GameBinds_Press(GB_UiSquadBroadcastChatCommand);
      api->GameBinds_Release(GB_UiSquadBroadcastChatCommand);
    }
    return true;
  }

  // Fallback: send Enter to submit regular chat
  api->WndProc_SendToGameOnly(NULL, WM_KEYDOWN, VK_RETURN, 0);
  api->WndProc_SendToGameOnly(NULL, WM_KEYUP, VK_RETURN, 0);
  return true;
}

// Calc seconds until next spawn
static int SecondsUntilDailySpawn(int spawnMinuteUTC, int durationMinutes) {
  auto now = std::chrono::system_clock::now();
  time_t t = std::chrono::system_clock::to_time_t(now);
  struct tm utc = {};
  gmtime_s(&utc, &t);
  int currentSecOfDay = utc.tm_hour * 3600 + utc.tm_min * 60 + utc.tm_sec;
  int spawnSecOfDay = spawnMinuteUTC * 60;
  int diff = spawnSecOfDay - currentSecOfDay;
  if (diff > 0)
    return diff; // Future today
  if (diff > -(durationMinutes * 60))
    return diff;       // Currently active (negative)
  return diff + 86400; // Next occurrence tomorrow
}

OverlayUI::OverlayUI(AddonAPI_t *api, TrainManager *manager, EventCatalog *catalog)
    : m_API(api), m_Manager(manager), m_Catalog(catalog) {
  if (m_API && m_API->Textures_Get) {
    m_Icon = m_API->Textures_Get("TC_ICON");
  }
  // init custom messages buffer
  m_SelectedCustomMessage = -1;
  memset(m_NewMessageBuf, 0, sizeof(m_NewMessageBuf));
  LoadCustomMessages();
}

void OverlayUI::LoadCustomMessages() {
  m_CustomMessages.clear();
  if (!m_Manager) return;
  std::string dir = m_Manager->GetAddonDir();
  std::string path = dir + "\\custom_messages.json";
  std::ifstream file(path);
  if (!file.is_open()) return;
  try {
    json j;
    file >> j;
    for (const auto &it : j["messages"]) {
      m_CustomMessages.push_back(it.get<std::string>());
    }
  } catch (...) {
  }
}

void OverlayUI::SaveCustomMessages() {
  if (!m_Manager) return;
  std::string dir = m_Manager->GetAddonDir();
  CreateDirectoryA(dir.c_str(), NULL);
  std::string path = dir + "\\custom_messages.json";
  json j;
  j["messages"] = json::array();
  for (const auto &m : m_CustomMessages)
    j["messages"].push_back(m);
  std::ofstream file(path);
  if (file.is_open()) file << j.dump(2);
}

void OverlayUI::Render() {
  if (!m_Visible || !m_Manager)
    return;

  TrainTemplate *activeTrain = m_Manager->GetActiveTrain();
  if (!activeTrain || activeTrain->Steps.empty())
    return; // Nothing to show

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_AlwaysAutoResize |
                           ImGuiWindowFlags_NoTitleBar;

  // Semi-transparent bg
  ImGui::SetNextWindowBgAlpha(0.6f);

  if (ImGui::Begin("Train Commander - Overlay", &m_Visible, flags)) {
    int currentStepIdx = m_Manager->GetCurrentStepIndex();

    if (m_Icon) {
      float iconSize = m_IconHovered ? 40.0f : 32.0f;
      float offset = m_IconHovered ? -4.0f : 0.0f;

      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

      ImVec4 tint =
          m_IconHovered ? ImVec4(1.2f, 1.2f, 1.2f, 1.0f) : ImVec4(1, 1, 1, 1);
      ImGui::ImageButton(m_Icon->Resource, ImVec2(iconSize, iconSize),
                         ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0),
                         tint);
      m_IconHovered = ImGui::IsItemHovered();

      ImGui::PopStyleColor(3);
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - offset);
      ImGui::SameLine();
    }
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Train: %s",
                       activeTrain->Name.c_str());
    ImGui::Separator();

    if (currentStepIdx >= 0 && currentStepIdx < activeTrain->Steps.size()) {
      const auto &currentStep = activeTrain->Steps[currentStepIdx];
      ImGui::Text("Current: %s", currentStep.Title.c_str());

      // Countdown if scheduled
      if (currentStep.SpawnMinuteUTC >= 0) {
        int secs = SecondsUntilDailySpawn(currentStep.SpawnMinuteUTC,
                                          currentStep.DurationMinutes);
        int absSecs = secs < 0 ? -secs : secs;
        int h = absSecs / 3600;
        int m = (absSecs % 3600) / 60;
        int s = absSecs % 60;
        if (secs < 0) {
          // Event is currently running
          ImGui::TextColored(
              ImVec4(0.2f, 1.0f, 0.4f, 1.0f),
              ">>> ACTIVE now! (Started %dh %02dm %02ds ago) <<<", h, m, s);
        } else if (secs < 300) {
          // Less than 5 minutes — highlight urgently
          ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                             "Starts in %dm %02ds!", m, s);
        } else {
          ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f),
                             "Next spawn in %dh %02dm %02ds", h, m, s);
        }
      }
      if (!currentStep.Description.empty()) {
        ImGui::TextWrapped("%s", currentStep.Description.c_str());
      }

      ImGui::Spacing();

      if (!currentStep.WaypointCode.empty() ||
          !currentStep.SquadMessage.empty()) {
        std::string broadcastStr = "";

        if (!currentStep.WaypointCode.empty()) {
          broadcastStr += currentStep.WaypointCode + " ";
          if (ImGui::Button("WP")) {
            ImGui::SetClipboardText(currentStep.WaypointCode.c_str());
          }
          ImGui::SameLine();
        }

        if (!currentStep.SquadMessage.empty()) {
          broadcastStr += currentStep.SquadMessage;
          if (ImGui::Button("Msg")) {
            ImGui::SetClipboardText(currentStep.SquadMessage.c_str());
          }
          ImGui::SameLine();
        }

        if (ImGui::Button("Broadcast All (WP + Msg)")) {
          ImGui::SetClipboardText(broadcastStr.c_str());
        }
      }

      // Custom messages manager
      ImGui::Separator();
      ImGui::Text("Custom Messages");
      ImGui::InputTextMultiline("New message", m_NewMessageBuf, sizeof(m_NewMessageBuf), ImVec2(-1, 80));
      if (ImGui::Button("Add Message")) {
        std::string nm = m_NewMessageBuf;
        if (!nm.empty()) {
          m_CustomMessages.push_back(nm);
          m_SelectedCustomMessage = (int)m_CustomMessages.size() - 1;
          memset(m_NewMessageBuf, 0, sizeof(m_NewMessageBuf));
          SaveCustomMessages();
        }
      }

      if (!m_CustomMessages.empty()) {
        ImGui::Spacing();
        ImGui::Text("Saved messages:");
        ImGui::BeginChild("msg_list", ImVec2(0, 120), true);
        for (int i = 0; i < (int)m_CustomMessages.size(); ++i) {
          const std::string &cm = m_CustomMessages[i];
          if (ImGui::Selectable(cm.c_str(), m_SelectedCustomMessage == i)) {
            m_SelectedCustomMessage = i;
            // copy selected into edit buffer for quick edit
            strncpy_s(m_NewMessageBuf, sizeof(m_NewMessageBuf), cm.c_str(), _TRUNCATE);
          }
        }
        ImGui::EndChild();

        ImGui::Spacing();
        if (m_SelectedCustomMessage >= 0 && m_SelectedCustomMessage < (int)m_CustomMessages.size()) {
          if (ImGui::Button("Copy Selected")) {
            ImGui::SetClipboardText(m_CustomMessages[m_SelectedCustomMessage].c_str());
            if (m_API && m_API->GUI_SendAlert)
              m_API->GUI_SendAlert("Message copied to clipboard.");
          }
          ImGui::SameLine();
          if (ImGui::Button("Update Selected")) {
            m_CustomMessages[m_SelectedCustomMessage] = std::string(m_NewMessageBuf);
            SaveCustomMessages();
          }
          ImGui::SameLine();
          if (ImGui::Button("Delete Selected")) {
            m_CustomMessages.erase(m_CustomMessages.begin() + m_SelectedCustomMessage);
            m_SelectedCustomMessage = -1;
            memset(m_NewMessageBuf, 0, sizeof(m_NewMessageBuf));
            SaveCustomMessages();
          }
        }
      }

      if (!currentStep.Mechanics.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Mechanics:");
        ImGui::TextWrapped("%s", currentStep.Mechanics.c_str());
        if (ImGui::Button("Copy Mechanics")) {
          ImGui::SetClipboardText(currentStep.Mechanics.c_str());
        }
      }
    }

    ImGui::Separator();

    // Navigation
    if (ImGui::Button("< Prev") && currentStepIdx > 0) {
      m_Manager->PreviousStep();
    }
    ImGui::SameLine();

    std::string stepCounter = std::to_string(currentStepIdx + 1) + " / " +
                              std::to_string(activeTrain->Steps.size());
    ImGui::Text("%s", stepCounter.c_str());
    ImGui::SameLine();

    if (ImGui::Button("Next >") &&
        currentStepIdx < activeTrain->Steps.size() - 1) {
      m_Manager->NextStep();
    }

    ImGui::Spacing();

    ImGui::Spacing();

    if (currentStepIdx + 1 < activeTrain->Steps.size()) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Next: %s",
                         activeTrain->Steps[currentStepIdx + 1].Title.c_str());
    } else {
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "End of Train");
    }

    if (currentStepIdx + 1 < (int)activeTrain->Steps.size()) {
      ImGui::Separator();
      const auto &nextStep = activeTrain->Steps[currentStepIdx + 1];
      if (ImGui::Button("Copy Next Boss")) {
        // Build a message that contains only the "Next up:" part.
        std::string msg;
        if (!nextStep.SquadMessage.empty()) {
          // If SquadMessage already contains "Next up:", use that substring.
          std::string tmp = nextStep.SquadMessage;
          std::string lower = tmp;
          for (char &c : lower)
            c = (char)tolower((unsigned char)c);
          size_t pos = lower.find("next up:");
          if (pos != std::string::npos) {
            msg = tmp.substr(pos);
            while (!msg.empty() && (msg[0] == ' ' || msg[0] == '-' || msg[0] == '\t'))
              msg.erase(msg.begin());
          }
        }
        if (msg.empty()) {
          // Fallback: construct a clean "Next up: <Title> <WP>" message
          msg = std::string("Next up: ") + nextStep.Title;
          if (!nextStep.WaypointCode.empty())
            msg += " " + nextStep.WaypointCode;
        }

        // Fallback: copy to clipboard and ask the user to paste manually
        ImGui::SetClipboardText(msg.c_str());
        m_PendingClipboardMsg.clear();
        m_PendingPaste = false;
        if (m_API) {
          if (m_API->GUI_SendAlert)
            m_API->GUI_SendAlert("Message copied — press Ctrl+V in squad chat then Enter to send.");
        }
      }
    }
  }
  ImGui::End();

  // No automatic paste sequence active — clipboard fallback only
  (void)m_PendingPaste;
}
