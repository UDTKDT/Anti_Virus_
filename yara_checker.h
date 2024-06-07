#pragma once

#include <string>
#include <vector>
#include <yara.h>

struct ST_YaraData  {
    std::vector<std::string>* DetectedMalware;
    const std::string* FilePath;
    std::string NameOfYaraRule;
};

int YaraCallbackFunction(YR_SCAN_CONTEXT* context, int message, void* messageData, void* userData);
int CheckYaraRule(const std::string& filePath, std::vector<std::string>& detectedMalware, std::string& strDetectionCause);
int GetRuleFiles(const std::string& directory, std::vector<std::string>& ruleFiles);