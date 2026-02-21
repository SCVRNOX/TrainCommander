#pragma once
#include "nexus/Nexus.h"

#include "event_catalog.h"
#include "train_manager.h"

class OverlayUI {
public:
  OverlayUI(AddonAPI_t *api, TrainManager *manager, EventCatalog *catalog);
  ~OverlayUI() = default;

  void Render();

  bool IsVisible() const { return m_Visible; }
  void Show() { m_Visible = true; }
  void Hide() { m_Visible = false; }
  void Toggle() { m_Visible = !m_Visible; }

private:
  AddonAPI_t *m_API;
  Texture_t *m_Icon = nullptr;
  TrainManager *m_Manager;
  EventCatalog *m_Catalog;
  bool m_Visible = true;
  bool m_IconHovered = false;
};
