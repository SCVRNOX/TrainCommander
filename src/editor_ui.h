#pragma once
#include "nexus/Nexus.h"

#include "train_manager.h"

class EventUI; // Forward declaration

class EditorUI {
public:
  EditorUI(AddonAPI_t *api, TrainManager *manager, EventUI *eventUI);
  ~EditorUI() = default;

  void Render();

  bool IsVisible() const { return m_Visible; }
  void Show() { m_Visible = true; }
  void Hide() { m_Visible = false; }
  void Toggle() { m_Visible = !m_Visible; }

private:
  TrainManager *m_Manager;
  EventUI *m_EventUI;
  AddonAPI_t *m_API = nullptr;
  Texture_t *m_Icon = nullptr;
  bool m_Visible = false;

  int m_SelectedTrainIndex = -1;
  int m_SelectedStepIndex = -1;
};
