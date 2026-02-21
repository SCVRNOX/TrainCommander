#include "train_manager.h"
#include "base64.h"
#include "nlohmann_json.hpp"

#include <fstream>
#include <windows.h>

using json = nlohmann::json;

TrainManager::TrainManager(const std::string &addonDir) : m_AddonDir(addonDir) {
  m_ConfigPath = m_AddonDir + "\\trains.json";
}

void TrainManager::LoadTrains() {
  std::ifstream file(m_ConfigPath);
  if (!file.is_open())
    return;

  try {
    json j;
    file >> j;

    m_Trains.clear();
    for (const auto &jTrain : j["trains"]) {
      TrainTemplate train;
      train.Name = jTrain.value("Name", "Unnamed Train");
      train.Author = jTrain.value("Author", "Unknown");

      for (const auto &jStep : jTrain["steps"]) {
        TrainStep step;
        step.Title = jStep.value("Title", "Step");
        step.Description = jStep.value("Description", "");
        step.WaypointCode = jStep.value("WaypointCode", "");
        step.SquadMessage = jStep.value("SquadMessage", "");
        step.Mechanics = jStep.value("Mechanics", "");
        step.SpawnMinuteUTC = jStep.value("SpawnMinuteUTC", -1);
        step.DurationMinutes = jStep.value("DurationMinutes", 0);
        train.Steps.push_back(step);
      }
      m_Trains.push_back(train);
    }
  } catch (...) {
  }
}

void TrainManager::SaveTrains() {
  json j;
  j["trains"] = json::array();

  for (const auto &train : m_Trains) {
    json jTrain;
    jTrain["Name"] = train.Name;
    jTrain["Author"] = train.Author;

    jTrain["steps"] = json::array();
    for (const auto &step : train.Steps) {
      json jStep;
      jStep["Title"] = step.Title;
      jStep["Description"] = step.Description;
      jStep["WaypointCode"] = step.WaypointCode;
      jStep["SquadMessage"] = step.SquadMessage;
      jStep["Mechanics"] = step.Mechanics;
      jStep["SpawnMinuteUTC"] = step.SpawnMinuteUTC;
      jStep["DurationMinutes"] = step.DurationMinutes;
      jTrain["steps"].push_back(jStep);
    }
    j["trains"].push_back(jTrain);
  }

  // Create dir if missing
  CreateDirectoryA(m_AddonDir.c_str(), NULL);

  std::ofstream file(m_ConfigPath);
  if (file.is_open()) {
    file << j.dump(4);
  }
}

void TrainManager::SetActiveTrain(int index) {
  if (index >= 0 && index < m_Trains.size()) {
    m_ActiveTrainIndex = index;
    m_CurrentStepIndex = 0;
  } else {
    m_ActiveTrainIndex = -1;
  }
}

TrainTemplate *TrainManager::GetActiveTrain() {
  if (m_ActiveTrainIndex >= 0 && m_ActiveTrainIndex < m_Trains.size()) {
    return &m_Trains[m_ActiveTrainIndex];
  }
  return nullptr;
}

void TrainManager::NextStep() {
  TrainTemplate *active = GetActiveTrain();
  if (active && m_CurrentStepIndex < (int)active->Steps.size() - 1) {
    m_CurrentStepIndex++;
  }
}

void TrainManager::PreviousStep() {
  if (m_CurrentStepIndex > 0) {
    m_CurrentStepIndex--;
  }
}

bool TrainManager::ImportFromClipboard(const std::string &base64Json) {
  std::string decoded = Base64::Decode(base64Json);
  if (decoded.empty())
    return false;

  try {
    json jTrain = json::parse(decoded);
    TrainTemplate train;
    train.Name = jTrain.value("Name", "Imported Train");
    train.Author = jTrain.value("Author", "Unknown");

    for (const auto &jStep : jTrain["steps"]) {
      TrainStep step;
      step.Title = jStep.value("Title", "Step");
      step.Description = jStep.value("Description", "");
      step.WaypointCode = jStep.value("WaypointCode", "");
      step.SquadMessage = jStep.value("SquadMessage", "");
      train.Steps.push_back(step);
    }

    m_Trains.push_back(train);
    SaveTrains();
    return true;
  } catch (...) {
    return false;
  }
}

std::string TrainManager::ExportToClipboard(int trainIndex) {
  if (trainIndex < 0 || trainIndex >= m_Trains.size())
    return "";

  const auto &train = m_Trains[trainIndex];
  json jTrain;
  jTrain["Name"] = train.Name;
  jTrain["Author"] = train.Author;

  jTrain["steps"] = json::array();
  for (const auto &step : train.Steps) {
    json jStep;
    jStep["Title"] = step.Title;
    jStep["Description"] = step.Description;
    jStep["WaypointCode"] = step.WaypointCode;
    jStep["SquadMessage"] = step.SquadMessage;
    jTrain["steps"].push_back(jStep);
  }

  std::string serialized = jTrain.dump();
  return Base64::Encode(serialized);
}
