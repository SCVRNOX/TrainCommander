#include "event_ui.h"
#include "imgui/imgui.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <map>
#include <string>
#include <vector>

ImU32 GetCategoryBaseColor(const std::string &category) {
  if (category == "Day and night")
    return IM_COL32(80, 80, 150, 255);
  if (category == "World bosses")
    return IM_COL32(180, 50, 50, 255);
  if (category == "Hard world bosses")
    return IM_COL32(220, 30, 30, 255);
  if (category == "Heart of Thorns")
    return IM_COL32(50, 160, 50, 255);
  if (category == "Path of Fire")
    return IM_COL32(160, 40, 160, 255);
  if (category == "Icebrood Saga")
    return IM_COL32(40, 150, 220, 255);
  if (category == "End of Dragons")
    return IM_COL32(20, 200, 150, 255);
  if (category == "Secrets of the Obscure")
    return IM_COL32(180, 180, 30, 255);
  if (category == "Janthir Wilds")
    return IM_COL32(160, 90, 30, 255);

  // Hash fallback for unknowns
  std::hash<std::string> hasher;
  size_t hash = hasher(category);
  int r = (hash & 0xFF) % 150 + 50;
  int g = ((hash >> 8) & 0xFF) % 150 + 50;
  int b = ((hash >> 16) & 0xFF) % 150 + 50;
  return IM_COL32(r, g, b, 255);
}

ImU32 GetDistinctColor(const std::string &category, int index) {
  ImU32 baseColor = GetCategoryBaseColor(category);

  int rBase = (baseColor & 0xFF);
  int gBase = ((baseColor >> 8) & 0xFF);
  int bBase = ((baseColor >> 16) & 0xFF);

  // Hue shift for variety
  if (index % 3 == 1) {
    // Shift hue: swap channels
    return IM_COL32(gBase, bBase, rBase, 255);
  } else if (index % 3 == 2) {
    // Shift hue the other way
    return IM_COL32(bBase, rBase, gBase, 255);
  }

  return baseColor;
}

EventUI::EventUI(AddonAPI_t *api, TrainManager *manager, EventCatalog *catalog)
    : m_API(api), m_Manager(manager), m_Catalog(catalog) {
  if (m_API && m_API->Textures_Get) {
    m_Icon = m_API->Textures_Get("TC_ICON");
  }
}

void EventUI::Render() {
  if (!m_Visible || !m_Manager || !m_Catalog)
    return;

  ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Event Catalog", &m_Visible)) {
    if (m_Icon) {
      float iconSize = m_IconHovered ? 32.0f : 24.0f;
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
    ImGui::Text("Train Commander - Event Catalog");
    ImGui::Separator();

    if (m_Catalog->IsFetching()) {
      ImGui::Text("Fetching live event schedule from GW2 Wiki...");
    } else {
      int minOffset = -15;
      int maxOffset = 120;
      auto upcomingEvents = m_Catalog->GetEventsInRange(minOffset, maxOffset);

      std::map<std::string, std::vector<UpcomingEvent>> groupedEvents;
      for (const auto &ev : upcomingEvents) {
        groupedEvents[ev.Definition.Map].push_back(ev);
      }

      std::vector<std::string> order = {
          "Day and night",   "World bosses",           "Hard world bosses",
          "Heart of Thorns", "Path of Fire",           "Icebrood Saga",
          "End of Dragons",  "Secrets of the Obscure", "Janthir Wilds"};

      for (const auto &pair : groupedEvents) {
        if (std::find(order.begin(), order.end(), pair.first) == order.end()) {
          order.push_back(pair.first);
        }
      }

      float availableWidth = ImGui::GetContentRegionAvail().x - 190.0f;
      float pixelsPerMinute =
          (std::max)(6.0f, availableWidth /
                               static_cast<float>(maxOffset - minOffset));
      float timelineWidth = (maxOffset - minOffset) * pixelsPerMinute;
      float rowHeight = 35.0f;

      ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV |
                              ImGuiTableFlags_ScrollX |
                              ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg;

      if (ImGui::BeginTable("TimelineTable", 2, flags)) {
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed,
                                180.0f);
        ImGui::TableSetupColumn("Timeline", ImGuiTableColumnFlags_WidthFixed,
                                timelineWidth);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Expansions / Categories");

        ImGui::TableSetColumnIndex(1);
        ImVec2 headerPos = ImGui::GetCursorScreenPos();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        for (int m = minOffset; m <= maxOffset; m += 15) {
          float x = (m - minOffset) * pixelsPerMinute;
          std::string timeLabel =
              (m == 0) ? "Now" : ((m > 0 ? "+" : "") + std::to_string(m) + "m");
          drawList->AddText(ImVec2(headerPos.x + x, headerPos.y),
                            IM_COL32(200, 200, 200, 255), timeLabel.c_str());
        }

        float nowX = (0 - minOffset) * pixelsPerMinute;
        drawList->AddLine(ImVec2(headerPos.x + nowX, headerPos.y),
                          ImVec2(headerPos.x + nowX, headerPos.y + 1000.0f),
                          IM_COL32(255, 50, 50, 200), 2.0f);

        for (const auto &cat : order) {
          if (groupedEvents.find(cat) == groupedEvents.end())
            continue;

          ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
          ImGui::TableSetColumnIndex(0);

          ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                               (rowHeight - ImGui::GetTextLineHeight()) * 0.5f);
          ImGui::Text("%s", cat.c_str());

          ImGui::TableSetColumnIndex(1);
          ImVec2 cellPos = ImGui::GetCursorScreenPos();
          int colorIndex = 0;
          ImVec2 mousePos = ImGui::GetMousePos();
          bool hoverConsumed = false;

          for (const auto &ev : groupedEvents[cat]) {
            ImU32 blockColor = GetDistinctColor(cat, colorIndex);
            ImU32 hoverColor = IM_COL32(255, 255, 255, 100);
            colorIndex++;

            float startX =
                (ev.ExactMinutesUntilSpawn - minOffset) * pixelsPerMinute;
            float blockWidth =
                static_cast<float>(ev.DurationMinutes) * pixelsPerMinute;

            // Clamp to prevent overlap
            const auto &evList = groupedEvents[cat];
            size_t evIdx = &ev - &evList[0];
            if (evIdx + 1 < evList.size()) {
              float nextStartX =
                  (evList[evIdx + 1].ExactMinutesUntilSpawn - minOffset) *
                  pixelsPerMinute;
              if (startX + blockWidth > nextStartX) {
                blockWidth = nextStartX - startX;
              }
            }
            if (blockWidth < 1.0f)
              blockWidth = 1.0f; // Always render at least 1px

            ImVec2 blockMin(cellPos.x + startX, cellPos.y + 2.0f);
            ImVec2 blockMax(cellPos.x + startX + blockWidth,
                            cellPos.y + rowHeight - 2.0f);

            // Manual hover check
            bool mouseOverBlock = !hoverConsumed && mousePos.x >= blockMin.x &&
                                  mousePos.x < blockMax.x &&
                                  mousePos.y >= blockMin.y &&
                                  mousePos.y < blockMax.y;

            ImGui::SetCursorScreenPos(blockMin);
            std::string blockId = "##" + ev.Definition.Name +
                                  std::to_string(ev.MinutesUntilSpawn);
            // Use InvisibleButton just to register the item in ImGui's system
            ImGui::InvisibleButton(blockId.c_str(),
                                   ImVec2(blockWidth, rowHeight - 4.0f));

            // Only the physically-topmost block reacts to click and hover
            bool isClicked = mouseOverBlock && ImGui::IsMouseClicked(0);
            bool isHovered = mouseOverBlock;

            if (mouseOverBlock)
              hoverConsumed = true; // mark consumed for rest of row

            if (isClicked) {
              if (m_Manager->GetActiveTrain()) {
                TrainStep newStep;
                newStep.Title = ev.Definition.Name;

                newStep.Description = ev.Definition.Map;
                newStep.WaypointCode = ev.Definition.WaypointCode;
                newStep.SquadMessage = ev.Definition.DefaultSquadMessage;
                // Calc UTC spawn minute
                {
                  auto nowChrono = std::chrono::system_clock::now();
                  time_t t_now =
                      std::chrono::system_clock::to_time_t(nowChrono);
                  struct tm utcNow = {};
                  gmtime_s(&utcNow, &t_now);
                  int currentUTCMinute = utcNow.tm_hour * 60 + utcNow.tm_min;

                  int spawnMinute =
                      (int)(currentUTCMinute + ev.ExactMinutesUntilSpawn);
                  spawnMinute = ((spawnMinute % 1440) + 1440) % 1440;
                  newStep.SpawnMinuteUTC = spawnMinute;
                }
                newStep.DurationMinutes = ev.DurationMinutes;

                auto activeTrain = m_Manager->GetActiveTrain();
                activeTrain->Steps.push_back(newStep);
              }
            }

            drawList->AddRectFilled(blockMin, blockMax, blockColor, 0.0f);
            drawList->AddRect(blockMin, blockMax, IM_COL32(0, 0, 0, 255), 0.0f,
                              0, 1.0f);
            if (isHovered) {
              drawList->AddRectFilled(blockMin, blockMax, hoverColor, 0.0f);

              int totalSecs =
                  static_cast<int>(std::abs(ev.ExactMinutesUntilSpawn * 60.0f));
              std::string hoverTimeStr =
                  (ev.ExactMinutesUntilSpawn <= 0)
                      ? "Started " + std::to_string(totalSecs / 60) + "m " +
                            std::to_string(totalSecs % 60) + "s ago"
                      : "in " + std::to_string(totalSecs / 60) + "m " +
                            std::to_string(totalSecs % 60) + "s";

              ImGui::SetTooltip("Click to Append:\n%s (%s)\nWaypoint: %s",
                                ev.Definition.Name.c_str(),
                                hoverTimeStr.c_str(),
                                ev.Definition.WaypointCode.c_str());
            }

            ImVec2 textPos(blockMin.x + 4.0f,
                           blockMin.y +
                               (rowHeight - ImGui::GetTextLineHeight()) * 0.5f -
                               2.0f);
            drawList->PushClipRect(blockMin, blockMax, true);
            drawList->AddText(textPos, IM_COL32(255, 255, 255, 255),
                              ev.Definition.Name.c_str());
            drawList->PopClipRect();
          }
        }

        ImGui::EndTable();
      }
    }
  }
  ImGui::End();
}
