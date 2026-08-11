/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * AQ War Effort - resource turn-in framework.
 *
 * Instead of hard-coding every War Effort resource/quest into the core, each
 * collector NPC is described by a row in `creature_aq_war_effort`:
 *
 *   creature_id        NPC entry of the collector
 *   item_id            item handed in
 *   item_count         items consumed per turn-in
 *   reward_item        supply-crate item awarded per turn-in (0 = no reward)
 *   reward_count       number of reward items per turn-in
 *   signet_item        faction commendation signet item (0 = no signet)
 *   signet_count       signets awarded per repeatable turn-in
 *   world_state        world-state field updated with the new total
 *   goal               total items required to complete this resource
 *   completed_event    optional game_event id enabled when the goal is reached
 *   gossip_menu_id     base gossip text shown on greet
 *   gossip_text_done   gossip text shown once the goal has been reached
 *   faction            0 neutral, 1 Alliance, 2 Horde
 *
 * A single gossip option per row is offered. Selecting it consumes one
 * stack turn-in, awards a supply crate and the appropriate number of
 * Commendation Signets, advances the in-memory progress and pushes the
 * world-state update to players in the collector's zone. Once a resource
 * reaches its goal the gossip option is hidden and the optional completion
 * game event is started.
 *
 * Progress is kept in memory and reset on server restart, matching how the
 * 1.9 realm-wide war effort was tracked on live servers.
 */

#include "Creature.h"
#include "GameEventMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "StringFormat.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <unordered_map>

enum AqWarEffortMisc
{
    AQ_WAR_EFFORT_GAME_EVENT = 22,

    GOSSIP_SENDER_TURN_IN  = 1,
    GOSSIP_SENDER_EXCHANGE = 2,

    SUPPLIES_CRATE_ILVL_10 = 21509,
    SUPPLIES_CRATE_ILVL_20 = 21510,
    SUPPLIES_CRATE_ILVL_30 = 21511,
    SUPPLIES_CRATE_ILVL_40 = 21512,
    SUPPLIES_CRATE_ILVL_50 = 21513,
};

struct AqWarEffortEntry
{
    uint32 creatureId;
    uint32 itemId;
    uint32 itemCount;
    uint32 rewardItem;
    uint32 rewardCount;
    uint32 signetItem;
    uint32 signetCount;
    uint32 worldState;
    uint32 goal;
    uint16 completedEvent;
    uint32 gossipMenuId;
    uint32 gossipTextDone;
    uint8  faction;
};

class AqWarEffortMgr
{
public:
    static AqWarEffortMgr* instance();

    void Load();
    std::vector<AqWarEffortEntry const*> GetEntriesForCreature(uint32 creatureId) const;
    AqWarEffortEntry const* GetEntry(uint32 creatureId, uint32 itemId) const;

    uint32 GetProgress(uint32 worldState) const;
    bool   IsComplete(uint32 worldState) const;
    void   AddProgress(uint32 worldState, uint32 amount);

private:
    std::vector<AqWarEffortEntry> _entries;
    std::unordered_map<uint32, uint32> _progress; // worldState -> items collected
};

AqWarEffortMgr* AqWarEffortMgr::instance()
{
    static AqWarEffortMgr inst;
    return &inst;
}

void AqWarEffortMgr::Load()
{
    _entries.clear();
    _progress.clear();

    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query(
        "SELECT creature_id, item_id, item_count, reward_item, reward_count, "
        "       signet_item, signet_count, world_state, goal, completed_event, "
        "       gossip_menu_id, gossip_text_done, faction "
        "FROM creature_aq_war_effort");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 AQ war effort turn-in definitions. DB table `creature_aq_war_effort` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        AqWarEffortEntry entry;
        entry.creatureId      = fields[0].Get<uint32>();
        entry.itemId          = fields[1].Get<uint32>();
        entry.itemCount       = fields[2].Get<uint32>();
        entry.rewardItem      = fields[3].Get<uint32>();
        entry.rewardCount     = fields[4].Get<uint32>();
        entry.signetItem      = fields[5].Get<uint32>();
        entry.signetCount     = fields[6].Get<uint32>();
        entry.worldState      = fields[7].Get<uint32>();
        entry.goal            = fields[8].Get<uint32>();
        entry.completedEvent  = fields[9].Get<uint16>();
        entry.gossipMenuId    = fields[10].Get<uint32>();
        entry.gossipTextDone  = fields[11].Get<uint32>();
        entry.faction         = fields[12].Get<uint8>();

        if (!entry.creatureId || !entry.itemId || !entry.itemCount || !entry.worldState || !entry.goal)
        {
            LOG_ERROR("sql.sql", "CreatureAqWarEffort: skipping invalid row creature {} item {}", entry.creatureId, entry.itemId);
            continue;
        }

        if (!sObjectMgr->GetItemTemplate(entry.itemId))
        {
            LOG_ERROR("sql.sql", "CreatureAqWarEffort: creature {} references unknown item {}", entry.creatureId, entry.itemId);
            continue;
        }

        if (entry.rewardItem && !sObjectMgr->GetItemTemplate(entry.rewardItem))
        {
            LOG_ERROR("sql.sql", "CreatureAqWarEffort: creature {} references unknown reward item {}", entry.creatureId, entry.rewardItem);
            entry.rewardItem = 0;
            entry.rewardCount = 0;
        }

        if (entry.signetItem && !sObjectMgr->GetItemTemplate(entry.signetItem))
        {
            LOG_ERROR("sql.sql", "CreatureAqWarEffort: creature {} references unknown signet item {}", entry.creatureId, entry.signetItem);
            entry.signetItem = 0;
            entry.signetCount = 0;
        }

        _entries.push_back(entry);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} AQ war effort turn-in definitions in {} ms",
             uint32(_entries.size()), GetMSTimeDiffToNow(oldMSTime));
}

std::vector<AqWarEffortEntry const*> AqWarEffortMgr::GetEntriesForCreature(uint32 creatureId) const
{
    std::vector<AqWarEffortEntry const*> matches;
    for (AqWarEffortEntry const& entry : _entries)
        if (entry.creatureId == creatureId)
            matches.push_back(&entry);
    return matches;
}

AqWarEffortEntry const* AqWarEffortMgr::GetEntry(uint32 creatureId, uint32 itemId) const
{
    for (AqWarEffortEntry const& entry : _entries)
        if (entry.creatureId == creatureId && entry.itemId == itemId)
            return &entry;
    return nullptr;
}

uint32 AqWarEffortMgr::GetProgress(uint32 worldState) const
{
    auto it = _progress.find(worldState);
    return it == _progress.end() ? 0u : it->second;
}

bool AqWarEffortMgr::IsComplete(uint32 worldState) const
{
    for (AqWarEffortEntry const& entry : _entries)
        if (entry.worldState == worldState)
            return GetProgress(worldState) >= entry.goal;
    return false;
}

void AqWarEffortMgr::AddProgress(uint32 worldState, uint32 amount)
{
    _progress[worldState] += amount;
}

/*
 * Gossip NPC. One gossip option per turn-in row. The option action encodes the
 * item id so a single OnGossipSelect handler can dispatch every row.
 */
class npc_aq_war_effort_collector : public CreatureScript
{
public:
    npc_aq_war_effort_collector() : CreatureScript("npc_aq_war_effort_collector") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!creature || !player)
            return false;

        if (!sGameEventMgr->IsActiveEvent(AQ_WAR_EFFORT_GAME_EVENT))
            return false;

        auto entries = AqWarEffortMgr::instance()->GetEntriesForCreature(creature->GetEntry());
        bool const hasExchange = IsExchangeCommander(creature->GetEntry());
        if (entries.empty() && !hasExchange)
            return false;

        if (!entries.empty())
        {
            SendGossipMenuFor(player,
                              entries.front()->gossipMenuId ? entries.front()->gossipMenuId
                                                            : player->GetGossipTextId(creature),
                              creature->GetGUID());
        }
        else
        {
            SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        }

        for (AqWarEffortEntry const* entry : entries)
        {
            if (AqWarEffortMgr::instance()->IsComplete(entry->worldState))
                continue;

            ItemTemplate const* item = sObjectMgr->GetItemTemplate(entry->itemId);
            if (!item)
                continue;

            std::string const gossipText = Acore::StringFormat("Turn in {}x {}", entry->itemCount, item->Name1);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, gossipText,
                             GOSSIP_SENDER_TURN_IN, entry->itemId);
        }

        if (hasExchange)
        {
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Exchange 5 Signets for War Effort Supplies",  GOSSIP_SENDER_EXCHANGE, 5);
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Exchange 10 Signets for War Effort Supplies", GOSSIP_SENDER_EXCHANGE, 10);
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Exchange 15 Signets for War Effort Supplies", GOSSIP_SENDER_EXCHANGE, 15);
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Exchange 20 Signets for War Effort Supplies", GOSSIP_SENDER_EXCHANGE, 20);
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Exchange 30 Signets for War Effort Supplies", GOSSIP_SENDER_EXCHANGE, 30);
        }

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (!player || !creature)
            return true;

        if (sender == GOSSIP_SENDER_EXCHANGE)
        {
            HandleExchange(player, creature, action);
            ClearGossipMenuFor(player);
            return OnGossipHello(player, creature);
        }

        if (sender != GOSSIP_SENDER_TURN_IN)
            return true;

        AqWarEffortEntry const* entry =
            AqWarEffortMgr::instance()->GetEntry(creature->GetEntry(), action);
        if (!entry)
            return true;

        if (AqWarEffortMgr::instance()->IsComplete(entry->worldState))
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (!player->HasItemCount(entry->itemId, entry->itemCount, true))
            return true;

        player->DestroyItemCount(entry->itemId, entry->itemCount, true);

        if (entry->rewardItem && entry->rewardCount)
            player->AddItem(entry->rewardItem, entry->rewardCount);

        if (entry->signetItem && entry->signetCount)
            player->AddItem(entry->signetItem, entry->signetCount);

        bool wasComplete = AqWarEffortMgr::instance()->IsComplete(entry->worldState);
        AqWarEffortMgr::instance()->AddProgress(entry->worldState, entry->itemCount);
        bool isComplete  = AqWarEffortMgr::instance()->IsComplete(entry->worldState);

        BroadcastWorldState(creature, entry->worldState,
                            AqWarEffortMgr::instance()->GetProgress(entry->worldState));

        if (!wasComplete && isComplete && entry->completedEvent)
            sGameEventMgr->StartEvent(entry->completedEvent, true);

        ClearGossipMenuFor(player);
        return OnGossipHello(player, creature);
    }

private:
    static bool IsExchangeCommander(uint32 entry)
    {
        return entry == 15700 || entry == 15701;
    }

    static uint32 GetExchangeSignetItem(uint32 entry)
    {
        if (entry == 15700)
            return 21438;
        if (entry == 15701)
            return 21436;
        return 0;
    }

    static void HandleExchange(Player* player, Creature const* creature, uint32 signets)
    {
        uint32 const signetItem = GetExchangeSignetItem(creature->GetEntry());
        if (!signetItem)
            return;

        uint32 crate = 0;
        switch (signets)
        {
            case 5:  crate = SUPPLIES_CRATE_ILVL_10; break;
            case 10: crate = SUPPLIES_CRATE_ILVL_20; break;
            case 15: crate = SUPPLIES_CRATE_ILVL_30; break;
            case 20: crate = SUPPLIES_CRATE_ILVL_40; break;
            case 30: crate = SUPPLIES_CRATE_ILVL_50; break;
            default: return;
        }

        if (!player->HasItemCount(signetItem, signets, true))
            return;

        player->DestroyItemCount(signetItem, signets, true);
        player->AddItem(crate, 1);
    }

    static void BroadcastWorldState(Creature const* creature, uint32 worldState, uint32 value)
    {
        if (!creature || !worldState)
            return;

        WorldPacket data(SMSG_UPDATE_WORLD_STATE, 8);
        data << uint32(worldState);
        data << uint32(value);

        Map::PlayerList const& players = creature->GetMap()->GetPlayers();
        for (auto const& ref : players)
            if (Player* p = ref.GetSource())
                if (p->GetZoneId() == creature->GetZoneId())
                    p->GetSession()->SendPacket(&data);
    }
};

class world_aq_war_effort : public WorldScript
{
public:
    world_aq_war_effort() : WorldScript("world_aq_war_effort") { }

    void OnStartup() override
    {
        AqWarEffortMgr::instance()->Load();
    }
};

void AddSC_aq_war_effort()
{
    new npc_aq_war_effort_collector();
    new world_aq_war_effort();
}
