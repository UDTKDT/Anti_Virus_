#include <algorithm>
#include <dirent.h>
#include <iostream>
#include "ansi_color.h"
#include "util.h"
#include "yara_checker.h"

CYaraChecker::CYaraChecker(const std::string& rulesDirectory) : m_strRulesDirectory(rulesDirectory) {
    GetRuleFiles();
}

// YARA 룰 매칭 콜백 함수
int CYaraChecker::YaraCallbackFunction(YR_SCAN_CONTEXT* context, int message, void* messageData, void* yaraData) {
    if (message == CALLBACK_MSG_RULE_MATCHING) {
        auto* data = static_cast<ST_YaraData*>(yaraData);        
        std::vector<std::string>* detectedMalware = data->DetectedMalware;
        const std::string* filePath = data->FilePath; // 파일 경로 가져오기
        YR_RULE* rule = (YR_RULE*)messageData; // 탐지된 룰 정보 가져오기
        // 중복 검사
        if (std::find(detectedMalware->begin(), detectedMalware->end(), *filePath) == detectedMalware->end()) {
            detectedMalware->push_back(*filePath);
            std::cout << "\n" << COLOR_RED << "[+] Malware detected: [" << *filePath << "]" << COLOR_RESET << "\n";
            std::cout << COLOR_RED << "[+] Detected by rule: [" << rule->identifier << "]" << COLOR_RESET << "\n\n";
            data->NameOfYaraRule = rule->identifier;
        }
    }
    return CALLBACK_CONTINUE;
}

// 디렉토리 내 YARA 룰 파일들을 가져오는 함수
int CYaraChecker::GetRuleFiles() {
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(m_strRulesDirectory.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_REG) { // regular file
                std::string strFilePath = m_strRulesDirectory + "/" + ent->d_name;
                m_vecRuleFiles.push_back(GetAbsolutePath(strFilePath));
            }
        }
        closedir(dir);
        return SUCCESS_CODE;
    } else {
        PrintError("Could not open directory " + m_strRulesDirectory );
        return ERROR_CANNOT_OPEN_DIRECTORY;
    }
}

int CYaraChecker::CheckYaraRule(const std::string& filePath, std::vector<std::string>& detectedMalware, std::string& detectionCause) {
    YR_COMPILER* compiler = nullptr;
    YR_RULES* rules = nullptr;

    // YARA 룰 파일 리스트
    if (m_vecRuleFiles.empty()) {
        PrintError("No YARA rules files found.");
    }
    
    // filePath가 ruleFiles에 있는지 확인
    if (std::find(m_vecRuleFiles.begin(), m_vecRuleFiles.end(), GetAbsolutePath(filePath)) != m_vecRuleFiles.end()) {
        std::cout << "\n" << COLOR_YELLOW << "[+] Skipping YARA rule check for file : " << filePath << COLOR_RESET << "\n\n";
        return SUCCESS_CODE;
    }

    // yara 라이브러리 초기화
    if (yr_initialize() != ERROR_SUCCESS) {
        PrintError("Failed to initialize YARA.");
        return ERROR_YARA_LIBRARY;
    }
    // yara 컴파일러 객체 생성
    if (yr_compiler_create(&compiler) != ERROR_SUCCESS) {
        PrintError("Failed to create YARA compiler.");
        yr_finalize();
        return ERROR_YARA_LIBRARY;
    }

    int nResult = SUCCESS_CODE;

    // YARA 룰 파일들 추가
    for (const auto& ruleFile : m_vecRuleFiles) {
        FILE* ruleFilePtr = fopen(ruleFile.c_str(), "r");
        if (!ruleFilePtr) {
            PrintError("Failed to open YARA rules file.");
            nResult = ERROR_CANNOT_OPEN_FILE;
            break;
        }
        // yara rule 컴파일
        if (yr_compiler_add_file(compiler, ruleFilePtr, nullptr, ruleFile.c_str()) != 0) {
            PrintError("Failed to compile YARA rules.");
            fclose(ruleFilePtr);
            nResult = ERROR_YARA_LIBRARY;
            break;
        }
        fclose(ruleFilePtr);
    }
    
    if(nResult == SUCCESS_CODE) {
        // 컴파일된 룰 가져오기
        if (yr_compiler_get_rules(compiler, &rules) != ERROR_SUCCESS) {
            PrintError("Failed to get compiled YARA rules.");
            nResult =  ERROR_YARA_LIBRARY;
        } else {
            ST_YaraData yaraData { &detectedMalware, &filePath, "" };

            // 스캔
            int scanResult = yr_rules_scan_file(rules, filePath.c_str(), 0, YaraCallbackFunction, &yaraData, 0);
            if (scanResult != ERROR_SUCCESS && scanResult != CALLBACK_MSG_RULE_NOT_MATCHING) {
                PrintError("Error scanning file " + filePath);
                nResult = ERROR_YARA_LIBRARY;
            }
            detectionCause = yaraData.NameOfYaraRule;
        }
    }

    if (rules) {
        yr_rules_destroy(rules);
    }
    if (compiler) {
        yr_compiler_destroy(compiler);
    }
    yr_finalize();
    return nResult;
}
