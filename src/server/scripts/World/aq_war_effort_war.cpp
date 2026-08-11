/*
 * AQ 10-hour war spawn controller.
 *
 * Spawns modest waves of Qiraji attackers and Kaldorei defenders around
 * the Scarab Gong in Silithus during AqWarEffortMgr PHASE_WAR. The
 * npc_qiraj_war_spawn AI in zone_silithus.cpp drives their combat
 * behavior; this script only controls periodic spawning so the realm
 * event can run independently of quest 8519.
 */

#include "aq_war_effort.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "World.h"
#include <vector>

namespace AqWar
{
    struct WaveEntry { uint32 creatureId; uint8 count; float x, y, z, o; };

    static WaveEntry const Attackers[] =
    {
        { 15424, 3, -8088.0f, 1530.0f, 2.61f, 0.0f }, // Anubisath Conqueror
        { 15414, 6, -8080.0f, 1531.0f, 2.61f, 0.0f }, // Qiraji Wasp
        { 15422, 3, -8081.0f, 1528.0f, 2.61f, 0.0f }, // Qiraji Tank
    };

    static WaveEntry const Defenders[] =
    {
        { 15423, 8, -8080.0f, 1518.0f, 2.61f, 3.141592f }, // Kaldorei Soldier
        { 15423, 8, -8082.0f, 1520.0f, 2.61f, 3.141592f },
    };

    static constexpr uint32 MAP_KALIMDOR   = 1;
    static constexpr uint32 ZONE_SILITHUS  = 1377;
    static constexpr uint32 WAVE_INTERVAL  = 5 * MINUTE * IN_MILLISECONDS;
    static constexpr uint32 DESPAWN        = 20 * MINUTE * IN_MILLISECONDS;
}

class world_aq_war_effort_war : public WorldScript
{
public:
    world_aq_war_effort_war() : WorldScript("world_aq_war_effort_war"), _waveTimer(0) { }

    void OnUpdate(uint32 diff) override
    {
        if (sAqWarEffortMgr->GetPhase() != PHASE_WAR)
        {
            _waveTimer = 0;
            return;
        }

        _waveTimer += diff;
        if (_waveTimer < AqWar::WAVE_INTERVAL)
            return;
        _waveTimer = 0;

        Player* anchor = FindAnchor();
        if (!anchor)
            return;

        for (auto const& w : AqWar::Attackers) Spawn(anchor, w);
        for (auto const& w : AqWar::Defenders) Spawn(anchor, w);
    }

private:
    static Player* FindAnchor()
    {
        for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
            if (player && player->IsInWorld() &&
                player->GetMapId() == AqWar::MAP_KALIMDOR &&
                player->GetZoneId() == AqWar::ZONE_SILITHUS)
                return player;
        return nullptr;
    }

    static void Spawn(Player* anchor, AqWar::WaveEntry const& w)
    {
        for (uint8 i = 0; i < w.count; ++i)
        {
            float x = w.x + frand(-4.0f, 4.0f);
            float y = w.y + frand(-4.0f, 4.0f);
            if (Creature* c = anchor->SummonCreature(w.creatureId, x, y, w.z, w.o,
                TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, AqWar::DESPAWN))
                c->SetHomePosition(x, y, w.z, w.o);
        }
    }

    uint32 _waveTimer;
};

void AddSC_aq_war_effort_war()
{
    new world_aq_war_effort_war();
}
