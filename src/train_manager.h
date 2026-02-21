#pragma once

#include "train_types.h"
#include <map>
#include <string>
#include <vector>


class TrainManager {
public:
  TrainManager(const std::string &addonDir);
  ~TrainManager() = default;

  void LoadTrains();
  void SaveTrains();

  std::vector<TrainTemplate> &GetTrains() { return m_Trains; }

  void SetActiveTrain(int index);
  TrainTemplate *GetActiveTrain();

  int GetCurrentStepIndex() const { return m_CurrentStepIndex; }
  void NextStep();
  void PreviousStep();

  bool ImportFromClipboard(const std::string &base64Json);
  std::string ExportToClipboard(int trainIndex);

private:
  std::string m_AddonDir;
  std::string m_ConfigPath;
  std::vector<TrainTemplate> m_Trains;

  int m_ActiveTrainIndex = -1;
  int m_CurrentStepIndex = 0;
};
