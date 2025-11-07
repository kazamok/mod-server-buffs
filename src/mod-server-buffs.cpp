
/*
mod-server-buffs.cpp */

#include "Common.h"        // Common.h를 최상단으로 이동
#include "WorldSessionMgr.h" // WorldSessionMgr.h를 최상단으로 이동
#include "SpellMgr.h"      // SpellMgr.h를 최상단으로 이동
#include "GameTime.h"      // GameTime.h를 최상단으로 이동
#include "ScriptMgr.h"
#include "World.h"
#include "Player.h"
#include "Chat.h"
#include "Config.h"
#include "Spell.h"
#include "SharedDefines.h" 
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <vector>
#include <set>
#include <ctime>
#include <map>

// 전역 변수 선언
// 모듈 설정 값을 저장할 전역 변수
bool g_serverBuffsEnabled = false;
std::string g_serverBuffsAnnounceMessage = "";
bool g_serverBuffsShowAnnounceMessage = false;
std::vector<uint32> g_serverBuffsSpellIds;
std::vector<std::string> g_serverBuffsCastTimes;
std::vector<int> g_serverBuffsExcludeLevels;
std::vector<uint32> g_serverBuffsSpecialSpellIds;
std::vector<std::string> g_serverBuffsExcludeClasses;

// 오늘 이미 버프를 적용한 시간을 기록 (중복 적용 방지)
std::set<std::string> g_serverBuffsAppliedTimesToday;
// 마지막으로 버프가 적용된 날짜를 추적하는 변수
std::string g_lastBuffApplicationDate;
// 각 버프 적용 시간별 사전 안내가 이루어졌는지 기록 (중복 방지)
// Key: CastTime (e.g., "18:30"), Value: Set of announced minutes (e.g., {10, 5, 1})
std::map<std::string, std::set<int>> g_serverBuffsAnnouncedTimes;

// 문자열을 구분자로 분리하여 벡터로 반환하는 헬퍼 함수
static std::vector<std::string> SplitString(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

std::string GetClassName(uint8 classId)
{
    switch (classId)
    {
        case CLASS_WARRIOR: return "Warrior";
        case CLASS_PALADIN: return "Paladin";
        case CLASS_HUNTER: return "Hunter";
        case CLASS_ROGUE: return "Rogue";
        case CLASS_PRIEST: return "Priest";
        case CLASS_DEATH_KNIGHT: return "DeathKnight";
        case CLASS_SHAMAN: return "Shaman";
        case CLASS_MAGE: return "Mage";
        case CLASS_WARLOCK: return "Warlock";
        case CLASS_DRUID: return "Druid";
        default: return "Unknown";
    }
}


// 모듈 전용 설정 파일을 로드하고 파싱하는 함수
void LoadModuleSpecificConfig_ServerBuffs()
{
    std::string configFilePath = "./configs/modules/mod-server-buffs.conf";

    std::ifstream configFile;

    if (std::filesystem::exists(configFilePath))
    {
        configFile.open(configFilePath);
        LOG_INFO("module", "[서버 버프] 설정 파일 로드: {}", configFilePath);
    }
    else
    {
        LOG_ERROR("module", "[서버 버프] 설정 파일을 찾을 수 없습니다. 모듈이 비활성화됩니다.");
        g_serverBuffsEnabled = false;
        g_serverBuffsAnnounceMessage = "서버 버프 모듈이 설정 파일을 찾을 수 없어 비활성화되었습니다.";
        g_serverBuffsShowAnnounceMessage = true;
        return;
    }

    if (!configFile.is_open())
    {
        LOG_ERROR("module", "[서버 버프] 설정 파일을 열 수 없습니다. 모듈이 비활성화됩니다.");
        g_serverBuffsEnabled = false;
        g_serverBuffsAnnounceMessage = "서버 버프 모듈이 설정 파일을 열 수 없어 비활성화되었습니다.";
        g_serverBuffsShowAnnounceMessage = true;
        return;
    }

    // 기본값 설정
    g_serverBuffsEnabled = true;
    g_serverBuffsAnnounceMessage = "서버 전역 버프가 적용되었습니다!";
    g_serverBuffsShowAnnounceMessage = true;
    g_serverBuffsSpellIds.clear();
    g_serverBuffsCastTimes.clear();
    g_serverBuffsExcludeLevels.clear();
    g_serverBuffsSpecialSpellIds.clear();
    g_serverBuffsExcludeClasses.clear();

    std::string line;
    while (std::getline(configFile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '='))
        {
            std::string value;
            if (std::getline(iss, value))
            {
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "ServerBuffs.Enable")
                {
                    g_serverBuffsEnabled = (value == "1");
                }
                else if (key == "ServerBuffs.AnnounceMessage")
                {
                    if (value.length() >= 2 && value.front() == '"' && value.back() == '"')
                    {
                        g_serverBuffsAnnounceMessage = value.substr(1, value.length() - 2);
                    }
                    else
                    {
                        g_serverBuffsAnnounceMessage = value;
                    }
                }
                else if (key == "ServerBuffs.ShowAnnounceMessage")
                {
                    g_serverBuffsShowAnnounceMessage = (value == "1");
                }
                else if (key == "ServerBuffs.BuffSpells")
                {
                    std::vector<std::string> spellStr = SplitString(value, ',');
                    for (const std::string& s : spellStr)
                    {
                        try
                        {
                            g_serverBuffsSpellIds.push_back(std::stoul(s));
                        }
                        catch (const std::exception& e)
                        {
                            LOG_ERROR("module", "[서버 버프] 잘못된 주문 ID 형식: {} ({})", s, e.what());
                        }
                    }
                }
                else if (key == "ServerBuffs.CastTimes")
                {
                    g_serverBuffsCastTimes = SplitString(value, ',');
                }
                else if (key == "ServerBuffs.ExcludeLevels")
                {
                    std::vector<std::string> levelStr = SplitString(value, ',');
                    for (const std::string& s : levelStr)
                    {
                        try
                        {
                            g_serverBuffsExcludeLevels.push_back(std::stoi(s));
                        }
                        catch (const std::exception& e)
                        {
                            LOG_ERROR("module", "[서버 버프] 잘못된 제외 레벨 형식: {} ({})", s, e.what());
                        }
                    }
                }
                else if (key == "ServerBuffs.Specialbuffs")
                {
                    std::vector<std::string> spellStr = SplitString(value, ',');
                    for (const std::string& s : spellStr)
                    {
                        try
                        {
                            g_serverBuffsSpecialSpellIds.push_back(std::stoul(s));
                        }
                        catch (const std::exception& e)
                        {
                            LOG_ERROR("module", "[서버 버프] 잘못된 특별 버프 주문 ID 형식: {} ({})", s, e.what());
                        }
                    }
                }
                else if (key == "ServerBuffs.ExcludeClasss")
                {
                    g_serverBuffsExcludeClasses = SplitString(value, ',');
                }
            }
        }
    }
    configFile.close();
}

// 월드 서버 이벤트를 처리하는 클래스
class mod_server_buffs_world : public WorldScript
{
public:
    mod_server_buffs_world() : WorldScript("mod_server_buffs_world") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        // 모듈 전용 설정 파일을 로드하고 파싱합니다。
        LoadModuleSpecificConfig_ServerBuffs();
    }

    void OnUpdate(uint32 diff) override
    {
        if (!g_serverBuffsEnabled)
            return;

        time_t rawtime = static_cast<time_t>(GameTime::GetGameTime().count());
        struct tm* timeinfo = std::localtime(&rawtime);

        std::stringstream current_date_ss;
        current_date_ss << std::put_time(timeinfo, "%Y-%m-%d");
        std::string currentDateStr = current_date_ss.str();

        // 자정이 지나면 적용된 시간 목록 및 안내 기록을 초기화합니다.
        if (g_lastBuffApplicationDate.empty() || currentDateStr != g_lastBuffApplicationDate)
        {
            g_serverBuffsAppliedTimesToday.clear();
            g_serverBuffsAnnouncedTimes.clear(); // 사전 안내 기록 초기화
            g_lastBuffApplicationDate = currentDateStr;
            LOG_INFO("module", "[서버 버프] 날짜가 변경되어 버프 적용 및 안내 기록을 초기화합니다.");
        }

        for (const std::string& castTime : g_serverBuffsCastTimes)
        {
            int castHour, castMinute;
            try
            {
                size_t pos;
                castHour = std::stoi(castTime, &pos);
                if (pos >= castTime.length() || castTime[pos] != ':') continue;
                castMinute = std::stoi(castTime.substr(pos + 1));
            }
            catch (const std::exception&)
            {
                continue; // 잘못된 형식의 시간은 건너뜁니다.
            }

            int currentTotalMinutes = timeinfo->tm_hour * 60 + timeinfo->tm_min;
            int castTotalMinutes = castHour * 60 + castMinute;

            // 사전 안내 로직
            std::vector<int> preAnnounceMinutes = {10, 5, 1};
            for (int minutesBefore : preAnnounceMinutes)
            {
                if (currentTotalMinutes == castTotalMinutes - minutesBefore)
                {
                    if (g_serverBuffsAnnouncedTimes[castTime].find(minutesBefore) == g_serverBuffsAnnouncedTimes[castTime].end())
                    {
                        if (g_serverBuffsShowAnnounceMessage)
                        {
                            std::stringstream announce_ss;
                            announce_ss << minutesBefore << "분 후에 서버 전체 버프가 적용됩니다!";
                            WorldSessionMgr::Instance()->SendServerMessage(SERVER_MSG_STRING, announce_ss.str());
                            LOG_INFO("module", "[서버 버프] {}분 전 안내 메시지 전송: {}", minutesBefore, castTime);
                        }
                        g_serverBuffsAnnouncedTimes[castTime].insert(minutesBefore);
                    }
                }
            }

            // 버프 적용 로직
            if (currentTotalMinutes == castTotalMinutes && g_serverBuffsAppliedTimesToday.find(castTime) == g_serverBuffsAppliedTimesToday.end())
            {
                LOG_INFO("module", "[서버 버프] 버프 적용 시간 도달: {}", castTime);

                if (g_serverBuffsShowAnnounceMessage)
                {
                    WorldSessionMgr::Instance()->SendServerMessage(SERVER_MSG_STRING, g_serverBuffsAnnounceMessage);
                }

                WorldSessionMgr::Instance()->DoForAllOnlinePlayers([&](Player* player)
                {
                    if (!player || !player->IsInWorld())
                        return;

                    // Check if player's level is in the exclude list
                    uint8 playerLevel = player->GetLevel();
                    bool isExcluded = false;
                    for (int excludedLevel : g_serverBuffsExcludeLevels)
                    {
                        if (playerLevel == excludedLevel)
                        {
                            isExcluded = true;
                            break;
                        }
                    }

                    if (isExcluded)
                    {
                        LOG_INFO("module", "[서버 버프] 플레이어 {} (GUID: {}, 레벨: {})는 제외 레벨이므로 버프를 건너뜁니다.", player->GetName(), player->GetGUID().GetRawValue(), playerLevel);
                        return; // Skip this player
                    }

                    // Special buffs with class exclusion
                    std::string playerClass = GetClassName(player->getClass());
                    bool isClassExcluded = false;
                    for (const std::string& excludedClass : g_serverBuffsExcludeClasses)
                    {
                        if (playerClass == excludedClass)
                        {
                            isClassExcluded = true;
                            break;
                        }
                    }

                    if (!isClassExcluded)
                    {
                        for (uint32 spellId : g_serverBuffsSpecialSpellIds)
                        {
                            if (!sSpellMgr->GetSpellInfo(spellId))
                            {
                                LOG_ERROR("module", "[서버 버프] 유효하지 않은 특별 버프 주문 ID: {}", spellId);
                                continue;
                            }

                            if (player->AddAura(spellId, player))
                            {
                                LOG_INFO("module", "[서버 버프] 플레이어 {} (GUID: {})에게 특별 버프 주문 {} 적용 성공.", player->GetName(), player->GetGUID().GetRawValue(), spellId);
                            }
                            else
                            {
                                LOG_ERROR("module", "[서버 버프] 플레이어 {} (GUID: {})에게 특별 버프 주문 {} 적용 실패.", player->GetName(), player->GetGUID().GetRawValue(), spellId);
                            }
                        }
                    }

                    for (uint32 spellId : g_serverBuffsSpellIds)
                    {
                        if (!sSpellMgr->GetSpellInfo(spellId))
                        {
                            LOG_ERROR("module", "[서버 버프] 유효하지 않은 주문 ID: {}", spellId);
                            continue;
                        }

                        if (player->AddAura(spellId, player))
                        {
                            LOG_INFO("module", "[서버 버프] 플레이어 {} (GUID: {})에게 주문 {} 적용 성공.", player->GetName(), player->GetGUID().GetRawValue(), spellId);
                        }
                        else
                        {
                            LOG_ERROR("module", "[서버 버프] 플레이어 {} (GUID: {})에게 주문 {} 적용 실패.", player->GetName(), player->GetGUID().GetRawValue(), spellId);
                        }
                    }
                });
                g_serverBuffsAppliedTimesToday.insert(castTime);
            }
        }
    }
};

// 모든 스크립트를 추가하는 함수 (모듈 로드 시 호출됨)
void Addmod_server_buffsScripts()
{
    new mod_server_buffs_world();
}


