#define _CRT_SECURE_NO_WARNINGS
#include "editor_ui.h"
#include "event_ui.h"
#include "imgui/imgui.h"
#include <string>

EditorUI::EditorUI(AddonAPI_t *api, TrainManager *manager, EventUI *eventUI)
    : m_API(api), m_Manager(manager), m_EventUI(eventUI) {
  if (m_API && m_API->Textures_Get) {
    m_Icon = m_API->Textures_Get("TC_ICON");
  }
}

void EditorUI::Render() {
  if (!m_Visible || !m_Manager)
    return;

  auto &trains = m_Manager->GetTrains();

  ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Train Commander Editor", &m_Visible)) {
    if (ImGui::Button("Open Event Catalog (Live Schedule)", ImVec2(0, 30))) {
      if (m_EventUI)
        m_EventUI->Show();
    }
    ImGui::Separator();

    ImGui::Columns(2, "EditorColumns", true);
    ImGui::SetColumnWidth(0, 250);

    ImGui::Separator();

    for (int i = 0; i < (int)trains.size(); ++i) {
      bool isSelected = (m_SelectedTrainIndex == i);
      char label[256];
      snprintf(label, sizeof(label), "%s##%d", trains[i].Name.c_str(), i);
      if (ImGui::Selectable(label, isSelected)) {
        m_SelectedTrainIndex = i;
        m_SelectedStepIndex = -1;
      }
    }

    ImGui::Separator();
    if (ImGui::Button("+ Create New Train", ImVec2(-1, 0))) {
      TrainTemplate newTrain;
      newTrain.Name = "New Train";
      newTrain.Author = "Me";
      trains.push_back(newTrain);
      m_SelectedTrainIndex = trains.size() - 1;
    }
    if (ImGui::Button("Paste from Clipboard", ImVec2(-1, 0))) {
      const char *clip = ImGui::GetClipboardText();
      if (clip) {
        if (m_Manager->ImportFromClipboard(clip)) {
          m_SelectedTrainIndex = trains.size() - 1;
        }
      }
    }

    ImGui::Spacing();
    if (ImGui::Button("Save Trains API")) {
      m_Manager->SaveTrains();
      ImGui::OpenPopup("Save Success");
    }
    if (ImGui::BeginPopupModal("Save Success", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("All trains saved to addons directory!");
      if (ImGui::Button("OK", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::NextColumn();

    // Details View
    if (m_SelectedTrainIndex >= 0 &&
        m_SelectedTrainIndex < static_cast<int>(trains.size())) {
      auto &currTrain = trains[m_SelectedTrainIndex];

      if (ImGui::Button("Set Active", ImVec2(120, 0))) {
        m_Manager->SetActiveTrain(m_SelectedTrainIndex);
      }
      ImGui::SameLine();
      if (ImGui::Button("Export to Clipboard", ImVec2(150, 0))) {
        std::string b64 = m_Manager->ExportToClipboard(m_SelectedTrainIndex);
        ImGui::SetClipboardText(b64.c_str());
        ImGui::OpenPopup("Export Success");
      }
      if (ImGui::BeginPopupModal("Export Success", NULL,
                                 ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(
            "Train copied to clipboard! (Share with others via Ctrl+V)");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button("Delete Train", ImVec2(100, 0))) {
        trains.erase(trains.begin() + m_SelectedTrainIndex);
        if (m_Manager->GetActiveTrain() == &currTrain)
          m_Manager->SetActiveTrain(-1);
        m_SelectedTrainIndex = -1;

        ImGui::Columns(1);
        ImGui::End();
        return;
      }

      ImGui::Spacing();

      // Train Metadata
      char nameBuf[256];
      strncpy(nameBuf, currTrain.Name.c_str(), sizeof(nameBuf));
      if (ImGui::InputText("Train Name", nameBuf, sizeof(nameBuf)))
        currTrain.Name = nameBuf;

      char authorBuf[256];
      strncpy(authorBuf, currTrain.Author.c_str(), sizeof(authorBuf));
      if (ImGui::InputText("Author", authorBuf, sizeof(authorBuf)))
        currTrain.Author = authorBuf;

      ImGui::Separator();
      ImGui::Text("Steps:");

      // Step List
      ImGui::BeginChild("StepList", ImVec2(0, 200), true);
      int moveFrom = -1, moveTo = -1;
      for (int j = 0; j < (int)currTrain.Steps.size(); ++j) {
        bool isStepSelected = (m_SelectedStepIndex == j);

        // Up button
        ImGui::PushID(j);
        if (j == 0)
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
        if (ImGui::SmallButton("Up") && j > 0) {
          moveFrom = j;
          moveTo = j - 1;
        }
        if (j == 0)
          ImGui::PopStyleVar();
        ImGui::SameLine();
        // Down button
        bool isLast = (j == (int)currTrain.Steps.size() - 1);
        if (isLast)
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
        if (ImGui::SmallButton("Down") && !isLast) {
          moveFrom = j;
          moveTo = j + 1;
        }
        if (isLast)
          ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::PopID();

        std::string label = std::to_string(j + 1) + ". " +
                            currTrain.Steps[j].Title + "##step" +
                            std::to_string(j);
        if (ImGui::Selectable(label.c_str(), isStepSelected)) {
          m_SelectedStepIndex = j;
        }
      }
      if (moveFrom >= 0 && moveTo >= 0 &&
          moveTo < (int)currTrain.Steps.size()) {
        std::swap(currTrain.Steps[moveFrom], currTrain.Steps[moveTo]);
        if (m_SelectedStepIndex == moveFrom)
          m_SelectedStepIndex = moveTo;
        else if (m_SelectedStepIndex == moveTo)
          m_SelectedStepIndex = moveFrom;
      }
      ImGui::EndChild();

      if (ImGui::Button("Add Step", ImVec2(120, 0))) {
        TrainStep step;
        step.Title = "New Step";
        currTrain.Steps.push_back(step);
        m_SelectedStepIndex = currTrain.Steps.size() - 1;
      }
      ImGui::SameLine();
      if (m_SelectedStepIndex >= 0 &&
          ImGui::Button("Delete Step", ImVec2(120, 0))) {
        currTrain.Steps.erase(currTrain.Steps.begin() + m_SelectedStepIndex);
        m_SelectedStepIndex = -1;
      }

      ImGui::Separator();

      // Step Details
      if (m_SelectedStepIndex >= 0 &&
          m_SelectedStepIndex < static_cast<int>(currTrain.Steps.size())) {
        auto &currStep = currTrain.Steps[m_SelectedStepIndex];

        char titleBuf[256];
        strncpy(titleBuf, currStep.Title.c_str(), sizeof(titleBuf));
        if (ImGui::InputText("Title", titleBuf, sizeof(titleBuf)))
          currStep.Title = titleBuf;

        char descBuf[512];
        strncpy(descBuf, currStep.Description.c_str(), sizeof(descBuf));
        if (ImGui::InputTextMultiline("Description", descBuf, sizeof(descBuf)))
          currStep.Description = descBuf;

        char wpBuf[128];
        strncpy(wpBuf, currStep.WaypointCode.c_str(), sizeof(wpBuf));
        if (ImGui::InputText("Waypoint Code", wpBuf, sizeof(wpBuf)))
          currStep.WaypointCode = wpBuf;

        char sqBuf[512];
        strncpy(sqBuf, currStep.SquadMessage.c_str(), sizeof(sqBuf));
        if (ImGui::InputTextMultiline("Squad Message", sqBuf, sizeof(sqBuf),
                                      ImVec2(-1, 60)))
          currStep.SquadMessage = sqBuf;

        ImGui::Spacing();
        ImGui::TextDisabled("Mechanics / Group Assignments:");
        char mechBuf[1024];
        strncpy(mechBuf, currStep.Mechanics.c_str(), sizeof(mechBuf));
        if (ImGui::InputTextMultiline("##Mechanics", mechBuf, sizeof(mechBuf),
                                      ImVec2(-1, 80)))
          currStep.Mechanics = mechBuf;

        ImGui::SameLine();
        if (ImGui::Button("Copy Mechanics")) {
          ImGui::SetClipboardText(currStep.Mechanics.c_str());
        }
      }
    }
    ImGui::Columns(1);
  }
  ImGui::End();
}
