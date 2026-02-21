#define _CRT_SECURE_NO_WARNINGS
#include "event_catalog.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

using json = nlohmann::json;

EventCatalog::EventCatalog(const std::string &addonDir) : m_AddonDir(addonDir) {
  FetchEventsAsync();
}

EventCatalog::~EventCatalog() {
  if (m_FetchThread.joinable()) {
    m_FetchThread.join();
  }
}

void EventCatalog::PopulateEvents() {}

void EventCatalog::FetchEventsAsync() {
  if (m_IsFetching)
    return;
  m_IsFetching = true;
  m_FetchThread = std::thread([this]() {
    HINTERNET hInternet = InternetOpenA(
        "TrainCommander/1.1", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
      m_IsFetching = false;
      return;
    }

    HINTERNET hConnect = InternetOpenUrlA(
        hInternet,
        "https://raw.githubusercontent.com/qjv/event-timers/main/"
        "event_tracks.json",
        NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);

    if (!hConnect) {
      InternetCloseHandle(hInternet);
      m_IsFetching = false;
      return;
    }

    std::string responseData;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) &&
           bytesRead > 0) {
      responseData.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (!responseData.empty()) {
      ParseWikiJson(responseData);
    }

    m_IsFetching = false;
  });
}

void EventCatalog::ParseWikiJson(const std::string &jsonData) {
  try {
    json raw = json::parse(jsonData);
    std::vector<EventDefinition> newEvents;

    // Calc UTC-3 midnight reference
    auto nowChrono = std::chrono::system_clock::now();
    int64_t current_utc_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(
            nowChrono.time_since_epoch())
            .count();

    int64_t seconds_per_day = 24 * 60 * 60;
    int64_t timezone_offset = -3 * 60 * 60; // UTC-3

    // Math from rust: (current_utc_timestamp + timezone_offset) %
    // seconds_per_day
    int64_t seconds_since_local_midnight =
        (current_utc_seconds + timezone_offset) % seconds_per_day;
    if (seconds_since_local_midnight < 0) {
      seconds_since_local_midnight += seconds_per_day;
    }

    int64_t local_midnight_utc_seconds =
        current_utc_seconds - seconds_since_local_midnight;

    // Now convert the actual local UTC-3 midnight back into a "minute of the
    // UTC day" to map our 0-1440 array directly against `tm_hour * 60 + tm_min`
    time_t midnight_time = (time_t)local_midnight_utc_seconds;
    struct tm *tm_utc_midnight = gmtime(&midnight_time);

    int midnight_minute_of_utc_day =
        tm_utc_midnight->tm_hour * 60 + tm_utc_midnight->tm_min;

    if (raw.contains("categories")) {
      for (const auto &cat : raw["categories"]) {
        std::string catName = cat.value("name", "Unknown");
        if (!cat.contains("tracks"))
          continue;

        for (const auto &track : cat["tracks"]) {
          std::string trackName = track.value("name", "Unknown");
          std::string baseTimeCalc =
              track.value("base_time_calculator", "local_day_start");

          int64_t baseTimeUtcMinute = 0;
          if (baseTimeCalc == "local_day_start") {
            baseTimeUtcMinute = midnight_minute_of_utc_day;
          } else if (baseTimeCalc == "tyria_cycle" ||
                     baseTimeCalc == "cantha_cycle") {
            // Reference: 2025-09-30 17:00:00 UTC-3 = Tyrian 00:00
            int64_t reference_time = 1759262400;
            int64_t cycle_duration = 120 * 60; // 120 minutes

            int64_t time_since_reference = current_utc_seconds - reference_time;
            int64_t cycles_elapsed = time_since_reference / cycle_duration;
            int64_t current_cycle_start_utc =
                reference_time + (cycles_elapsed * cycle_duration);

            // Mapping cycle to UTC day (0-1440)
            time_t cycle_time = (time_t)current_cycle_start_utc;
            struct tm *tm_utc_cycle = gmtime(&cycle_time);
            baseTimeUtcMinute =
                tm_utc_cycle->tm_hour * 60 + tm_utc_cycle->tm_min;
          }

          if (!track.contains("schedules"))
            continue;

          for (const auto &sched : track["schedules"]) {
            std::string eventName = sched.value("name", "Unknown Event");
            std::string wp = sched.value("copy_text", "");
            int duration = sched.value("duration", 15);
            int offset = sched.value("offset", 0);
            int interval = sched.value("interval", 0);

            if (wp.empty())
              continue;

            EventDefinition def;
            def.Name = eventName;
            def.Map = catName; // Group by category name (e.g. Base Game)
            def.WaypointCode = wp;
            def.DefaultSquadMessage = "Next up: " + eventName + " " + wp;

            std::vector<std::pair<int, int>> timeDurations;

            if (interval > 0) {
              int cycle_minutes = 24 * 60; // 1440
              if (baseTimeCalc == "tyria_cycle" ||
                  baseTimeCalc == "cantha_cycle") {
                cycle_minutes = 120; // These loops happen in 2hr frames
              }

              int repetitions = cycle_minutes / interval;
              if (repetitions == 0)
                repetitions = 1; // Safeguard

              // Project events across 24h
              int full_day_repetitions = 1440 / interval;
              for (int i = 0; i < full_day_repetitions; ++i) {
                int spawnTimeLocal = offset + (i * interval);
                int absoluteUtcMinute =
                    (baseTimeUtcMinute + spawnTimeLocal) % 1440;

                timeDurations.push_back({absoluteUtcMinute, duration});
              }
            } else {
              int absoluteUtcMinute = (baseTimeUtcMinute + offset) % 1440;
              timeDurations.push_back({absoluteUtcMinute, duration});

              // Handle cyclical 2h tracks
              if (baseTimeCalc == "tyria_cycle" ||
                  baseTimeCalc == "cantha_cycle") {
                for (int cycleOffset = 120; cycleOffset < 1440;
                     cycleOffset += 120) {
                  timeDurations.push_back(
                      {(absoluteUtcMinute + cycleOffset) % 1440, duration});
                }
              }
            }

            // Deduplicate and Sort
            std::sort(timeDurations.begin(), timeDurations.end());
            auto last = std::unique(timeDurations.begin(), timeDurations.end());
            timeDurations.erase(last, timeDurations.end());

            for (const auto &p : timeDurations) {
              def.SpawnTimesUTC.push_back(p.first);
              def.DurationsUTC.push_back(p.second);
            }
            newEvents.push_back(def);
          }
        }
      }
    }

    std::lock_guard<std::mutex> lock(m_EventsMutex);
    m_Events = newEvents;
  } catch (const std::exception &e) {
    // Log exception if needed
  }
}

std::vector<UpcomingEvent> EventCatalog::GetUpcomingEvents(int limit) {
  std::vector<UpcomingEvent> upcoming;

  auto nowChrono = std::chrono::system_clock::now();
  time_t now = std::chrono::system_clock::to_time_t(nowChrono);
  struct tm *tm_utc = gmtime(&now);

  int currentMinuteOfDay = tm_utc->tm_hour * 60 + tm_utc->tm_min;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                nowChrono.time_since_epoch()) %
            1000;
  float fractionalMinute = tm_utc->tm_sec / 60.0f + ms.count() / 60000.0f;

  std::lock_guard<std::mutex> lock(m_EventsMutex);

  for (const auto &ev : m_Events) {
    int nextSpawn = -1;
    int minDiff = 999999;

    int nextDuration = 15;

    for (size_t i = 0; i < ev.SpawnTimesUTC.size(); ++i) {
      int spawnTime = ev.SpawnTimesUTC[i];
      int duration = ev.DurationsUTC.size() > i ? ev.DurationsUTC[i] : 15;

      int diff = spawnTime - currentMinuteOfDay;
      if (diff >= -15 && diff < minDiff) {
        minDiff = diff;
        nextSpawn = spawnTime;
        nextDuration = duration;
      }
    }

    if (nextSpawn == -1 && !ev.SpawnTimesUTC.empty()) {
      nextSpawn = ev.SpawnTimesUTC[0];
      minDiff = (nextSpawn + 1440) - currentMinuteOfDay;
    }

    if (nextSpawn != -1) {
      UpcomingEvent upcomingEv;
      upcomingEv.Definition = ev;
      upcomingEv.MinutesUntilSpawn = minDiff;
      upcomingEv.DurationMinutes = nextDuration;
      upcomingEv.ExactMinutesUntilSpawn =
          static_cast<float>(minDiff) - fractionalMinute;
      upcoming.push_back(upcomingEv);
    }
  }

  std::sort(upcoming.begin(), upcoming.end(),
            [](const UpcomingEvent &a, const UpcomingEvent &b) {
              return a.MinutesUntilSpawn < b.MinutesUntilSpawn;
            });

  if (limit > 0 && limit < static_cast<int>(upcoming.size())) {
    upcoming.resize(limit);
  }

  return upcoming;
}

std::vector<UpcomingEvent>
EventCatalog::GetEventsInRange(int minMinutesOffset, int maxMinutesOffset) {
  std::vector<UpcomingEvent> upcoming;

  auto nowChrono = std::chrono::system_clock::now();
  time_t now = std::chrono::system_clock::to_time_t(nowChrono);
  struct tm *tm_utc = gmtime(&now);

  int currentMinuteOfDay = tm_utc->tm_hour * 60 + tm_utc->tm_min;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                nowChrono.time_since_epoch()) %
            1000;
  float fractionalMinute = tm_utc->tm_sec / 60.0f + ms.count() / 60000.0f;

  std::lock_guard<std::mutex> lock(m_EventsMutex);

  for (const auto &ev : m_Events) {
    for (size_t i = 0; i < ev.SpawnTimesUTC.size(); ++i) {
      int spawnTime = ev.SpawnTimesUTC[i];
      int duration = ev.DurationsUTC.size() > i ? ev.DurationsUTC[i] : 15;

      int diff = spawnTime - currentMinuteOfDay;

      if (diff > maxMinutesOffset) {
        diff -= 1440;
      } else if (diff < minMinutesOffset) {
        diff += 1440;
      }

      if (diff >= minMinutesOffset && diff <= maxMinutesOffset) {
        UpcomingEvent uEv;
        uEv.Definition = ev;
        uEv.MinutesUntilSpawn = diff;
        uEv.DurationMinutes = duration;
        uEv.ExactMinutesUntilSpawn =
            static_cast<float>(diff) - fractionalMinute;
        upcoming.push_back(uEv);
      }
    }
  }

  std::sort(upcoming.begin(), upcoming.end(),
            [](const UpcomingEvent &a, const UpcomingEvent &b) {
              return a.MinutesUntilSpawn < b.MinutesUntilSpawn;
            });

  return upcoming;
}
