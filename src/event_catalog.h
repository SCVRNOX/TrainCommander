#pragma once

#include "nlohmann_json.hpp"
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct EventDefinition {
  std::string Name;
  std::string Map;
  std::string WaypointCode;
  std::string DefaultSquadMessage;
  std::vector<int> SpawnTimesUTC; // Minute of the day (0-1439)
  std::vector<int> DurationsUTC;  // Corresponding durations
};

struct UpcomingEvent {
  EventDefinition Definition;
  int MinutesUntilSpawn;
  float ExactMinutesUntilSpawn;
  int DurationMinutes;
};

class EventCatalog {
public:
  EventCatalog(const std::string &addonDir);
  ~EventCatalog();

  void FetchEventsAsync();
  std::vector<UpcomingEvent> GetUpcomingEvents(int limit = 10);
  std::vector<UpcomingEvent> GetEventsInRange(int minMinutesOffset,
                                              int maxMinutesOffset);
  bool IsFetching() const { return m_IsFetching; }

private:
  void PopulateEvents();
  void ParseWikiJson(const std::string &jsonData);

  std::string m_AddonDir;
  std::vector<EventDefinition> m_Events;
  mutable std::mutex m_EventsMutex;
  bool m_IsFetching = false;
  std::thread m_FetchThread;
};
