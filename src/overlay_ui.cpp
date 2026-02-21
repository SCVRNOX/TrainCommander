#include "overlay_ui.h"
#include "imgui/imgui.h"
#include <chrono>
#include <ctime>
#include <string>

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

OverlayUI::OverlayUI(AddonAPI_t *api, TrainManager *manager,
                     EventCatalog *catalog)
    : m_API(api), m_Manager(manager), m_Catalog(catalog) {
  if (m_API && m_API->Textures_Get) {
    m_Icon = m_API->Textures_Get("TC_ICON");
  }
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
        std::string msg = "";
        if (!nextStep.WaypointCode.empty())
          msg += nextStep.WaypointCode + " ";
        if (!nextStep.SquadMessage.empty())
          msg += nextStep.SquadMessage;
        else
          msg += nextStep.Title;
        ImGui::SetClipboardText(msg.c_str());
      }
    }
  }
  ImGui::End();
}
