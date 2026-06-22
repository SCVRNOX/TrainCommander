#pragma once
#include "nexus/Nexus.h"

#include "event_catalog.h"
#include "train_manager.h"
#include <vector>
#include <cstring>

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
  // Deferred chat send state
  std::wstring m_PendingWideMsg;
  size_t m_PendingIndex = 0;
  int m_PendingDelayFrames = 0;
  bool m_PendingPaste = false;
  std::string m_PendingClipboardMsg;
  int m_PendingPasteState = 0; // 0=init,1=after-clear,2=pasted

  // Custom messages
  std::vector<std::string> m_CustomMessages;
  int m_SelectedCustomMessage = -1;
  char m_NewMessageBuf[512];

  void LoadCustomMessages();
  void SaveCustomMessages();
};
