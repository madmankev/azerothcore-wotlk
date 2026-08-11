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
 * AQ War Effort - resource turn-in framework and post-collection phases.
 *
 * Phase state machine:
 *
 *   GATHERING  - both factions are still turning in resources
 *   TRANSIT    - all resources collected; 5-day caravan to Silithus
 *   WAR        - caravan arrived; 10-hour Qiraji war around the Scarab Gong
 *   COMPLETE   - 10-hour war ended; gong/ringing available
 *
 * The state is persisted in `aq_war_effort_save`. The actual Qiraji war
 * spawns in zone_silithus (npc_qiraj_war_spawn / npc_anachronos_quest_trigger)
 * are started by WAR- and COMPLETE-phase hooks.
 */

#include "Creature.h"
#include "GameEventMgr.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "StringFormat.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "aq_war_effort.h"
#include <unordered_map>
#include <unordered_set>

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

    NPC_WARLORD_GORCHUK        = 15700,
    NPC_FIELD_MARSHAL_SNOWFALL = 15701,
    NPC_ANACHRONOS_TRIGGER     = 15426,

    // Phase durations (ms). Can be overridden by operators for testing.
    TRANSIT_DURATION_MS = 5 * DAY * IN_MILLISECONDS,
    WAR_DURATION_MS     = 10 * HOUR * IN_MILLISECONDS,
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
    void Save();

    std::vector<AqWarEffortEntry const*> GetEntriesForCreature(uint32 creatureId) const;
    AqWarEffortEntry const* GetEntry(uint32 creatureId, uint32 itemId) const;

    uint32 GetProgress(uint32 worldState) const;
    bool   IsComplete(uint32 worldState) const;
    void   AddProgress(uint32 worldState, uint32 amount);
    void   CheckAllComplete();

    AqWarEffortPhase GetPhase() const { return _phase; }
    time_t           GetPhaseEnd() const { return _phaseEnd; }
    void SetPhase(AqWarEffortPhase phase, bool announce = true);
    void Update(uint32 diff);

private:
    std::vector<AqWarEffortEntry> _entries;
    std::unordered_map<uint32, uint32> _progress;

    AqWarEffortPhase _phase = PHASE_GATHERING;
    time_t           _phaseEnd = 0;
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
    _phase = PHASE_GATHERING;
    _phaseEnd = 0;

    uint32 oldMSTime = getMSTime();

    if (QueryResult result = WorldDatabase.Query(
        "SELECT creature_id, item_id, item_count, reward_item, reward_count, "
        "       signet_item, signet_count, world_state, goal, completed_event, "
        "       gossip_menu_id, gossip_text_done, faction "
        "FROM creature_aq_war_effort"))
    {
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
    }
    else
    {
        LOG_WARN("server.loading", ">> Loaded 0 AQ war effort turn-in definitions. DB table `creature_aq_war_effort` is empty.");
    }

    LOG_INFO("server.loading", ">> Loaded {} AQ war effort turn-in definitions in {} ms",
             uint32(_entries.size()), GetMSTimeDiffToNow(oldMSTime));

    // Load persisted phase.
    if (QueryResult save = WorldDatabase.Query(
        "SELECT phase, phase_end FROM aq_war_effort_save LIMIT 1"))
    {
        Field* f = save->Fetch();
        _phase    = AqWarEffortPhase(f[0].Get<uint8>());
        _phaseEnd = time_t(f[1].Get<uint32>());

        if (_phase == PHASE_TRANSIT && _phaseEnd <= time(nullptr))
        {
            SetPhase(PHASE_WAR);
        }
        else if (_phase == PHASE_WAR && _phaseEnd <= time(nullptr))
        {
            SetPhase(PHASE_COMPLETE);
        }
    }
}

void AqWarEffortMgr::Save()
{
    WorldDatabase.DirectExecute("TRUNCATE TABLE aq_war_effort_save");
    WorldDatabase.Execute("INSERT INTO aq_war_effort_save (phase, phase_end) VALUES ({}, {})",
                          uint32(_phase), uint32(_phaseEnd));
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

void AqWarEffortMgr::SetPhase(AqWarEffortPhase phase, bool announce)
{
    if (_phase == phase)
        return;

    _phase = phase;
    _phaseEnd = 0;

    if (phase == PHASE_GATHERING)
        _caravanArrived[0] = _caravanArrived[1] = false;

    switch (phase)
    {
        case PHASE_TRANSIT:
            _phaseEnd = time(nullptr) + TRANSIT_DURATION_MS / IN_MILLISECONDS;
            if (announce)
            {
                // Both factions have finished; materiel is being shipped to Silithus.
                sWorld->SendWorldText(11146); // Alliance commander yell
                sWorld->SendWorldText(11148); // Horde commander yell
            }
            break;
        case PHASE_WAR:
            _phaseEnd = time(nullptr) + WAR_DURATION_MS / IN_MILLISECONDS;
            // The 10-hour war has begun; spawns are handled by
            // npc_anachronos_quest_trigger in zone_silithus.
            if (announce)
                sWorld->SendWorldText(11427);
            break;
        case PHASE_COMPLETE:
            if (announce)
                sWorld->SendWorldText(11427);
            break;
        default:
            break;
    }

    Save();
}

void AqWarEffortMgr::NotifyCaravanArrived(TeamId team)
{
    if (team >= TEAM_NEUTRAL)
        return;

    _caravanArrived[team] = true;

    // The 10-hour war begins once both faction caravans have arrived.
    if (_caravanArrived[TEAM_ALLIANCE] && _caravanArrived[TEAM_HORDE])
        SetPhase(PHASE_WAR);
}

void AqWarEffortMgr::CheckAllComplete()
{
    if (_phase != PHASE_GATHERING || _entries.empty())
        return;

    std::unordered_set<uint32> states;
    for (AqWarEffortEntry const& e : _entries)
        states.insert(e.worldState);

    for (uint32 state : states)
        if (!IsComplete(state))
            return;

    SetPhase(PHASE_TRANSIT);
}

void AqWarEffortMgr::Update(uint32 /*diff*/)
{
    if (_phase == PHASE_TRANSIT || _phase == PHASE_WAR)
    {
        if (_phaseEnd && _phaseEnd <= time(nullptr))
        {
            if (_phase == PHASE_TRANSIT)
                SetPhase(PHASE_WAR);
            else
                SetPhase(PHASE_COMPLETE);
        }
    }
}

/*
 * Gossip NPC. One gossip option per turn-in row plus signet exchanges
 * for the two commanders.
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

        AqWarEffortPhase const phase = AqWarEffortMgr::instance()->GetPhase();

        auto entries = AqWarEffortMgr::instance()->GetEntriesForCreature(creature->GetEntry());
        bool const hasExchange = IsExchangeCommander(creature->GetEntry());

        // During WAR/COMPLETE the collectors still accept signet exchanges
        // but resource turn-ins are closed.
        bool const canTurnIn = phase == PHASE_GATHERING;

        if (entries.empty() && !hasExchange)
            return false;

        if (!entries.empty())
        {
            SendGossipMenuFor(player,
                              entries.front()->gossipMenuId ? entries.front()->gossipMenuId
                                                            : player->GetGossipTextId(creature),
                              creature->GetGUID());
        }
        else if (hasExchange)
        {
            SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        }

        if (canTurnIn)
        {
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

        // Turn-ins are only valid during GATHERING.
        if (AqWarEffortMgr::instance()->GetPhase() != PHASE_GATHERING)
        {
            CloseGossipMenuFor(player);
            return true;
        }

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

        AqWarEffortMgr::instance()->CheckAllComplete();

        ClearGossipMenuFor(player);
        return OnGossipHello(player, creature);
    }

    // The DB has the AQ "More..." repeatable turn-in quests (8493-8531)
    // but no signet/supply-crate rewards attached. Grant them here based on
    // the quest's requested item so players receive the historically correct
    // number of Commendation Signets and one War Effort Supplies crate.
    bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 /*opt*/) override
    {
        if (!player || !creature || !quest)
            return false;

        uint32 const qid = quest->GetQuestId();
        if (qid < 8492 || qid > 8531)
            return false;

        uint32 requiredItem = 0;
        for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            if (quest->RequiredItemId[i])
            {
                requiredItem = quest->RequiredItemId[i];
                break;
            }
        }

        static std::unordered_map<uint32, uint8> const signetForItem =
        {
            { 2840,  1 }, // Copper Bar
            { 3575,  5 }, // Iron Bar
            { 12359, 10 }, // Thorium Bar
            { 3820,  3 }, // Stranglekelp
            { 8831,  7 }, // Purple Lotus
            { 8836, 10 }, // Arthas' Tears
            { 2318,  1 }, // Light Leather
            { 2319,  3 }, // Medium Leather
            { 4304,  7 }, // Thick Leather
            { 1251,  1 }, // Linen Bandage
            { 6450,  5 }, // Silk Bandage
            { 14529, 10 }, // Runecloth Bandage
            { 5095,  3 }, // Rainbow Fin Albacore
            { 12210, 5 }, // Roast Raptor
            { 6887,  7 }, // Spotted Yellowtail
        };

        auto it = signetForItem.find(requiredItem);
        uint8 signets = (it != signetForItem.end()) ? it->second : 0;

        uint32 const signetItem = (creature->GetFactionTemplateEntry() && creature->GetFactionTemplateEntry()->team == ALLIANCE ? 21436 : 21438);
        uint32 const crateItem  = 21509;

        if (signets)
            player->AddItem(signetItem, signets);
        // The Singed Corestone quest (8530/8531) awards a Field Duty Papers
        // crate rather than signets; skip item grant for it.
        if (requiredItem != 20737 && requiredItem != 7076)
            player->AddItem(crateItem, 1);

        return true;
    }

private:
    static bool IsExchangeCommander(uint32 entry)
    {
        return entry == NPC_WARLORD_GORCHUK || entry == NPC_FIELD_MARSHAL_SNOWFALL;
    }

    static uint32 GetExchangeSignetItem(uint32 entry)
    {
        if (entry == NPC_WARLORD_GORCHUK)
            return 21438;
        if (entry == NPC_FIELD_MARSHAL_SNOWFALL)
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
    world_aq_war_effort() : WorldScript("world_aq_war_effort"), _announceTimer(0) { }

    void OnStartup() override
    {
        AqWarEffortMgr::instance()->Load();
    }

    void OnShutdown() override
    {
        AqWarEffortMgr::instance()->Save();
    }

    void OnAfterUnloadAllMaps() override
    {
        AqWarEffortMgr::instance()->Save();
    }

    void OnUpdate(uint32 diff) override
    {
        AqWarEffortMgr* mgr = AqWarEffortMgr::instance();

        if (mgr->GetPhase() == PHASE_GATHERING)
        {
            _announceTimer += diff;
            if (_announceTimer < 5 * MINUTE * IN_MILLISECONDS)
                return;
            _announceTimer = 0;

            if (!sGameEventMgr->IsActiveEvent(AQ_WAR_EFFORT_GAME_EVENT))
                return;

            Announce(NPC_FIELD_MARSHAL_SNOWFALL, 1537);
            Announce(NPC_WARLORD_GORCHUK, 1637);
            return;
        }

        if (mgr->GetPhase() == PHASE_TRANSIT || mgr->GetPhase() == PHASE_WAR)
        {
            time_t const end = mgr->GetPhaseEnd();
            if (end && end <= time(nullptr))
            {
                if (mgr->GetPhase() == PHASE_TRANSIT)
                    mgr->SetPhase(PHASE_WAR);
                else
                    mgr->SetPhase(PHASE_COMPLETE);
            }
        }
    }

private:
    static void Announce(uint32 entry, uint32 zone)
    {
        for (auto const& pair : ObjectAccessor::GetPlayers())
        {
            Player* player = pair.second;
            if (!player || !player->IsInWorld())
                continue;
            if (player->GetZoneId() != zone)
                continue;
            if (Creature* commander = player->FindNearestCreature(entry, 200.0f, true))
                commander->Yell("The war effort continues! Bring your supplies to the quartermasters. Steel, leather, herbs, bandages - we need them all!", LANG_UNIVERSAL);
            break;
        }
    }

    uint32 _announceTimer;
};

void AddSC_aq_war_effort()
{
    new npc_aq_war_effort_collector();
    new world_aq_war_effort();
}
