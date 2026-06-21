#pragma once

#include <string>
#include <vector>

enum class TrainType { Boss, Farm, Mixed };

struct CustomMessage {
  std::string title;
  std::string text;
};

struct TrainStep {
  std::string Title;
  std::string Description;
  std::string WaypointCode;
  std::string SquadMessage;
  std::string Mechanics;   // Group assignments / mechanic instructions
  int SpawnMinuteUTC = -1; // Minute (0-1439), -1 = no schedule
  int DurationMinutes = 0;
  std::vector<CustomMessage> CustomMessages;  // Custom messages for this step
};

struct TrainTemplate {
  std::string Name;
  std::string Author;
  TrainType Type = TrainType::Mixed;
  std::vector<TrainStep> Steps;
};
