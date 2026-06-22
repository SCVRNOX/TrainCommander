#pragma once

#include <string>
#include <vector>

enum class TrainType { Boss, Farm, Mixed };

struct TrainStep {
  std::string Title;
  std::string Description;
  std::string WaypointCode;
  std::string SquadMessage;
  std::string Mechanics;   // Group assignments / mechanic instructions
  int SpawnMinuteUTC = -1; // Minute (0-1439), -1 = no schedule
  int DurationMinutes = 0;
};

struct TrainTemplate {
  std::string Name;
  std::string Author;
  TrainType Type = TrainType::Mixed;
  std::vector<TrainStep> Steps;
};
