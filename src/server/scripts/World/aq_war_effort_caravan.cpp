/*
 * AQ War Effort supply caravan.
 *
 * On entering PHASE_TRANSIT, one caravan is launched for each faction
 * from Booty Bay (the only port both factions use to reach Kalimdor).
 * Each caravan consists of a leader, two guards and a few pack animals,
 * walking a road waypoint route through Stranglethorn, Duskwood, the
 * Wetlands, across to Menethil -> boat to Dustwallow (simulated), then
 * through the Barrens/Thousand Needles/Tanaris to Cenarion Hold.
 *
 * When a caravan leader arrives, it calls NotifyCaravanArrived(team) on
 * the AqWarEffortMgr. Once both factions have arrived the phase advances
 * to PHASE_WAR, which kicks off the 10-hour war spawns.
 *
 * Authentic caveat: the real 1.9 caravan travelled by a combination of
 * ground routes and boats, with NPCs specific to that event that don't
 * exist in 3.3.5 data. This implementation uses existing generic
 * caravan NPCs (caravaneers, pack kodos/rams, guards) and a road route
 * which is visible and thematic. The exact waypoints are not the
 * original 1.9 script but approximate the in-lore journey.
 */

#include "aq_war_effort.h"
#include "Creature.h"
#include "GameObject.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"

namespace
{
    enum
    {
        NPC_ALLIANCE_CARAVANEER = 10637,
        NPC_HORDE_CARAVANEER   = 10636,
        NPC_PACK_KODO          = 10635,
        NPC_PACK_RAM           = 10638,
        NPC_ALLIANCE_GUARD     = 1075,   // Stormwind Guard
        NPC_HORDE_GUARD        = 3296,   // Orgrimmar Grunt

        POINT_FIRST            = 1,
        POINT_LAST             = 999,


        CARAVAN_SPAWN_MS       = 5 * MINUTE * IN_MILLISECONDS,
        CARAVAN_DESPAWN_MS     = 2 * MINUTE * IN_MILLISECONDS,
    };

    // Booty Bay -> Cenarion Hold (approximate road route). z=0 samples terrain.
    std::vector<Position> const CaravanPath =
    {
        { -14300.0f,   530.0f, 0.0f, 0.0f }, // Booty Bay
        { -13100.0f,   350.0f, 0.0f, 0.0f }, // northern Stranglethorn
        { -11600.0f,   320.0f, 0.0f, 0.0f }, // into Duskwood
        { -10600.0f,  -750.0f, 0.0f, 0.0f }, // Darkshire
        { -10600.0f, -1900.0f, 0.0f, 0.0f }, // north Duskwood
        {  -9500.0f, -2900.0f, 0.0f, 0.0f }, // into Wetlands
        {  -8200.0f, -3300.0f, 0.0f, 0.0f }, // Menethil Harbor
        // "Boat" to Dustwallow (simulated with a long jump along the coast)
        {  -9200.0f, -2900.0f, 0.0f, 0.0f },
        {  -9600.0f, -2000.0f, 0.0f, 0.0f }, // Dustwallow Marsh
        {  -9800.0f,  -800.0f, 0.0f, 0.0f }, // into Barrens
        {  -8400.0f,   600.0f, 0.0f, 0.0f }, // Thousand Needles
        {  -7800.0f,   500.0f, 0.0f, 0.0f }, // Feralas edge
        {  -7400.0f,   700.0f, 0.0f, 0.0f }, // Tanaris
        {  -6680.0f,   780.0f, 0.0f, 0.0f }, // Cenarion Hold
    };

    class npc_aq_caravan_leader : public CreatureScript
    {
    public:
        npc_aq_caravan_leader() : CreatureScript("npc_aq_caravan_leader") { }

        struct npc_aq_caravan_leaderAI : public ScriptedAI
        {
            npc_aq_caravan_leaderAI(Creature* c) : ScriptedAI(c), _team(TEAM_NEUTRAL), _step(0) { }

            void InitializeAI() override
            {
                if (me->GetEntry() == NPC_ALLIANCE_CARAVANEER)
                    _team = TEAM_ALLIANCE;
                else if (me->GetEntry() == NPC_HORDE_CARAVANEER)
                    _team = TEAM_HORDE;

                me->SetWalk(true);
                _step = 0;
                if (_step < CaravanPath.size())
                    me->GetMotionMaster()->MovePoint(POINT_FIRST + _step, CaravanPath[_step], true);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;
                if (id != POINT_FIRST + _step)
                    return;

                ++_step;
                if (_step >= CaravanPath.size())
                {
                    Arrive();
                    return;
                }
                me->GetMotionMaster()->MovePoint(POINT_FIRST + _step, CaravanPath[_step], true);
            }

            void Arrive()
            {
                me->Yell("War supplies delivered!", LANG_UNIVERSAL);
                sAqWarEffortMgr->NotifyCaravanArrived(_team);
                me->DespawnOrUnsummon(CARAVAN_DESPAWN_MS);
            }

            TeamId _team;
            size_t _step;
        };

        CreatureAI* GetAI(Creature* c) const override { return new npc_aq_caravan_leaderAI(c); }
    };

    class world_aq_caravan : public WorldScript
    {
    public:
        world_aq_caravan() : WorldScript("world_aq_caravan"), _spawnTimer(0), _spawnedAlliance(false), _spawnedHorde(false) { }

        void OnStartup() override
        {
            _spawnTimer = 0;
            _spawnedAlliance = sAqWarEffortMgr->CaravanArrived(TEAM_ALLIANCE);
            _spawnedHorde    = sAqWarEffortMgr->CaravanArrived(TEAM_HORDE);
        }

        void OnUpdate(uint32 diff) override
        {
            if (sAqWarEffortMgr->GetPhase() != PHASE_TRANSIT)
            {
                _spawnTimer = 0;
                return;
            }

            _spawnTimer += diff;
            if (_spawnTimer < CARAVAN_SPAWN_MS)
                return;
            _spawnTimer = 0;

            if (!_spawnedAlliance)
            {
                LaunchCaravan(NPC_ALLIANCE_CARAVANEER, NPC_PACK_RAM, NPC_ALLIANCE_GUARD, TEAM_ALLIANCE);
                _spawnedAlliance = true;
            }
            if (!_spawnedHorde)
            {
                LaunchCaravan(NPC_HORDE_CARAVANEER, NPC_PACK_KODO, NPC_HORDE_GUARD, TEAM_HORDE);
                _spawnedHorde = true;
            }
        }

    private:
        static void LaunchCaravan(uint32 leaderEntry, uint32 mountEntry, uint32 guardEntry, TeamId /*team*/)
        {
            Map* map = sMapMgr->FindMap(0, 0);
            if (!map)
                return;

            Position const& start = CaravanPath.front();
            if (Creature* leader = map->SummonCreature(leaderEntry, start, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                                                        CARAVAN_SPAWN_MS + CaravanPath.size() * 30000))
            {
                // A couple of pack animals and guards flanking the leader.
                for (uint8 i = 0; i < 2; ++i)
                {
                    float dx = (i == 0) ? -3.0f : 3.0f;
                    if (Creature* m = map->SummonCreature(mountEntry, start.x + dx, start.y + frand(-2.0f, 2.0f), start.z, 0,
                                                          TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                                                          CARAVAN_SPAWN_MS + CaravanPath.size() * 30000))
                        m->GetMotionMaster()->MoveFollow(leader, 2.0f + i, (i == 0) ? float(M_PI/2) : -float(M_PI/2));
                }
                for (uint8 i = 0; i < 2; ++i)
                {
                    float dx = (i == 0) ? -4.0f : 4.0f;
                    if (Creature* g = map->SummonCreature(guardEntry, start.x + dx, start.y + frand(-2.0f, 2.0f), start.z, 0,
                                                          TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                                                          CARAVAN_SPAWN_MS + CaravanPath.size() * 30000))
                        g->GetMotionMaster()->MoveFollow(leader, 4.0f + i, (i == 0) ? float(M_PI/4) : -float(M_PI/4));
                }
            }
        }

        uint32 _spawnTimer;
        bool   _spawnedAlliance;
        bool   _spawnedHorde;
    };
}

void AddSC_aq_war_effort_caravan()
{
    new npc_aq_caravan_leader();
    new world_aq_caravan();
}
