/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "Language.h"
#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "AnticheatMgr.h"
#include "Configuration/Config.h"
#include "Player.h"
#include "SpellAuras.h"

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace Acore::ChatCommands;

enum Spells
{
    SHACKLES = 38505,
    LFG_SPELL_DUNGEON_DESERTER = 71041,
    BG_SPELL_DESERTER = 26013,
    SILENCED = 23207
};

class anticheat_commandscript : public CommandScript
{
public:
    anticheat_commandscript() : CommandScript("anticheat_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> anticheatCommandTable =
        {
            { "global",         SEC_GAMEMASTER,     false,   &HandleAntiCheatGlobalCommand,  "" },
            { "player",         SEC_GAMEMASTER,     false,   &HandleAntiCheatPlayerCommand,  "" },
            { "delete",         SEC_ADMINISTRATOR,  false,   &HandleAntiCheatDeleteCommand,  "" },
            { "jail",           SEC_GAMEMASTER,     false,  &HandleAnticheatJailCommand,    "" },
            { "parole",         SEC_ADMINISTRATOR,  false,  &HandleAnticheatParoleCommand,  "" },
            { "purge",          SEC_ADMINISTRATOR,  false,   &HandleAntiCheatPurgeCommand,   "" },
            { "warn",           SEC_GAMEMASTER,     false,   &HandleAnticheatWarnCommand,    "" }
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "anticheat",      SEC_GAMEMASTER,     false,   NULL, "",  anticheatCommandTable},
        };

        return commandTable;
    }

    static bool HandleAnticheatWarnCommand(ChatHandler* handler, const char* args)
    {
        if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0))
            return false;

        Player* pTarget = NULL;

        std::string strCommand;

        char* command = strtok((char*)args, " ");

        if (command)
        {
            strCommand = command;
            normalizePlayerName(strCommand);

            pTarget = ObjectAccessor::FindPlayerByName(strCommand.c_str()); // get player by name
        }else
            pTarget = handler->getSelectedPlayer();

        if (!pTarget)
            return false;

        WorldPacket data;

        // need copy to prevent corruption by strtok call in LineFromMessage original string
        char* buf = strdup("The anticheat system has reported several times that you may be cheating. You will be monitored to confirm if this is accurate.");
        char* pos = buf;

        while (char* line = handler->LineFromMessage(pos))
        {
            handler->BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, NULL, line);
            pTarget->GetSession()->SendPacket(&data);
        }

        free(buf);
        return true;
    }

    static bool HandleAnticheatJailCommand(ChatHandler* handler, const char* args)
    {
        if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0))
            return false;

        Player* pTarget = NULL;

        std::string strCommand;

        char* command = strtok((char*)args, " ");

        if (command)
        {
            strCommand = command;
            normalizePlayerName(strCommand);

            pTarget = ObjectAccessor::FindPlayerByName(strCommand.c_str()); // get player by name
        }else
            pTarget = handler->getSelectedPlayer();

        if (!pTarget)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (pTarget == handler->GetSession()->GetPlayer())
            return false;

        // teleport both to jail.
        handler->GetSession()->GetPlayer()->TeleportTo(1,16226.5f,16403.6f,-64.5f,3.2f);

        WorldLocation loc = WorldLocation(1, 16226.5f, 16403.6f, -64.5f, 3.2f); // GM Jail Location
        pTarget->TeleportTo(loc);
        pTarget->SetHomebind(loc, 876); // GM Jail Homebind location
        pTarget->CastSpell(pTarget, SHACKLES); // Shackle him in place to ensure no exploit happens for jail break attempt

        if (Aura* dungdesert = pTarget->AddAura(LFG_SPELL_DUNGEON_DESERTER, pTarget))// LFG_SPELL_DUNGEON_DESERTER
        {
            dungdesert->SetDuration(-1);
        }
        if (Aura* bgdesert = pTarget->AddAura(BG_SPELL_DESERTER, pTarget))// BG_SPELL_DESERTER
        {
            bgdesert->SetDuration(-1);
        }
        if (Aura* silent = pTarget->AddAura(SILENCED, pTarget))// SILENCED
        {
            silent->SetDuration(-1);
        }

        return true;
    }

    static bool HandleAnticheatParoleCommand(ChatHandler* handler, const char* args)
    {
        if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0))
            return false;

        Player* pTarget = NULL;

        std::string strCommand;

        char* command = strtok((char*)args, " ");

        if (command)
        {
            strCommand = command;
            normalizePlayerName(strCommand);

            pTarget = ObjectAccessor::FindPlayerByName(strCommand.c_str()); // get player by name
        }
        else
            pTarget = handler->getSelectedPlayer();

        if (!pTarget)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (pTarget == handler->GetSession()->GetPlayer())
            return false;

        WorldLocation Aloc = WorldLocation(0, -8833.37f, 628.62f, 94.00f, 1.06f);// Stormwind
        WorldLocation Hloc = WorldLocation(1, 1569.59f, -4397.63f, 16.06f, 0.54f);// Orgrimmar

        if (pTarget->GetTeamId() == TEAM_ALLIANCE)
        {
            pTarget->TeleportTo(0, -8833.37f, 628.62f, 94.00f, 1.06f);//Stormwind
            pTarget->SetHomebind(Aloc, 1519);// Stormwind Homebind location
        }
        else
        {
            pTarget->TeleportTo(1, 1569.59f, -4397.63f, 7.7f, 0.54f);//Orgrimmar
            pTarget->SetHomebind(Hloc, 1653);// Orgrimmar Homebind location
        }
        pTarget->RemoveAura(SHACKLES);// remove shackles
        pTarget->RemoveAura(LFG_SPELL_DUNGEON_DESERTER);// LFG_SPELL_DUNGEON_DESERTER
        pTarget->RemoveAura(BG_SPELL_DESERTER);// BG_SPELL_DESERTER
        pTarget->RemoveAura(SILENCED);// SILENCED
        sAnticheatMgr->AnticheatDeleteCommand(pTarget->GetGUID());// deletes auto reports on player

        return true;
    }

    static bool HandleAntiCheatDeleteCommand(ChatHandler* handler, const char* args)
    {
        if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0))
            return false;

        std::string strCommand;

        char* command = strtok((char*)args, " "); // get entered name

        if (!command)
            return true;

        strCommand = command;

        if (strCommand.compare("deleteall") == 0)
            sAnticheatMgr->AnticheatDeleteCommand(ObjectGuid::Empty);
        else
        {
            normalizePlayerName(strCommand);
            Player* player = ObjectAccessor::FindPlayerByName(strCommand.c_str()); // get player by name
            if (!player)
                handler->PSendSysMessage("Player doesn't exist");
            else
                sAnticheatMgr->AnticheatDeleteCommand(player->GetGUID());
        }

        return true;
    }

    static bool HandleAntiCheatPlayerCommand(ChatHandler* handler, const char* args)
    {
        if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0))
            return false;

        std::string strCommand;

        char* command = strtok((char*)args, " ");

        ObjectGuid guid;
        Player* player = nullptr;

        if (command)
        {
            strCommand = command;

            normalizePlayerName(strCommand);
            player = ObjectAccessor::FindPlayerByName(strCommand.c_str()); // get player by name

            if (player)
                guid = player->GetGUID();
        }else
        {
            player = handler->getSelectedPlayer();
            if (player)
                guid = player->GetGUID();
        }

        if (!guid)
        {
            handler->PSendSysMessage("There is no player.");
            return true;
        }

        float average = sAnticheatMgr->GetAverage(guid);
        uint32 total_reports = sAnticheatMgr->GetTotalReports(guid);
        uint32 speed_reports = sAnticheatMgr->GetTypeReports(guid, 0);
        uint32 fly_reports = sAnticheatMgr->GetTypeReports(guid, 1);
        uint32 jump_reports = sAnticheatMgr->GetTypeReports(guid, 3);
        uint32 waterwalk_reports = sAnticheatMgr->GetTypeReports(guid, 2);
        uint32 teleportplane_reports = sAnticheatMgr->GetTypeReports(guid, 4);
        uint32 climb_reports = sAnticheatMgr->GetTypeReports(guid, 5);
        uint32 teleport_reports = sAnticheatMgr->GetTypeReports(guid, 6);
        uint32 ignorecontrol_reports = sAnticheatMgr->GetTypeReports(guid, 7);
        uint32 zaxis_reports = sAnticheatMgr->GetTypeReports(guid, 8);

        handler->PSendSysMessage("Information about player %s",player->GetName().c_str());
        handler->PSendSysMessage("Average: %f || Total Reports: %u ",average,total_reports);
        handler->PSendSysMessage("Speed Reports: %u || Fly Reports: %u || Jump Reports: %u ",speed_reports,fly_reports,jump_reports);
        handler->PSendSysMessage("Walk On Water Reports: %u  || Teleport To Plane Reports: %u",waterwalk_reports,teleportplane_reports);
        handler->PSendSysMessage("Teleport Reports: %u || Climb Reports: %u", teleport_reports,climb_reports);
        handler->PSendSysMessage("Ignore Control Reports: %u || Ignore Z-Axis Reports: %u", ignorecontrol_reports, zaxis_reports);
        return true;
    }

    static bool HandleAntiCheatGlobalCommand(ChatHandler* handler, const char* /* args */)
    {
        if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0))
        {
            handler->PSendSysMessage("The Anticheat System is disabled.");
            return true;
        }

        sAnticheatMgr->AnticheatGlobalCommand(handler);

        return true;
    }

    static bool HandleAntiCheatPurgeCommand(ChatHandler* handler, const char* /* args */)
    {
        sAnticheatMgr->AnticheatPurgeCommand(handler);
        handler->PSendSysMessage("The Anticheat daily_player_reports has been purged.");
        return true;
    }
};

void AddSC_anticheat_commandscript()
{
    new anticheat_commandscript();
}
