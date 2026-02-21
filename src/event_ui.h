#pragma once
#include "nexus/Nexus.h"

#include "event_catalog.h"
#include "imgui/imgui.h"
#include "train_manager.h"

class EventUI {
public:
  EventUI(AddonAPI_t *api, TrainManager *manager, EventCatalog *catalog);
  void Render();
  bool IsVisible() const { return m_Visible; }
  void Show() { m_Visible = true; }
  void Hide() { m_Visible = false; }
  void Toggle() { m_Visible = !m_Visible; }

private:
  AddonAPI_t *m_API = nullptr;
  Texture_t *m_Icon = nullptr;
  bool m_Visible = false;
  TrainManager *m_Manager = nullptr;
  EventCatalog *m_Catalog = nullptr;
  bool m_IconHovered = false;
};
