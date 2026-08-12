/*
 * Deadmines: Sneed / Sneed's Shredder.
 * The shredder must be destroyed first; Sneed then jumps out and
 * continues the fight on foot.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ObjectAccessor.h"
#include "deadmines.h"

enum Spells
{
    SPELL_THRASH_PASSIVE      = 3391,
    SPELL_SNEED_DISMEMBER     = 7355,
    SPELL_SNEED_SINISTER     = 14873,
    SPELL_SNEED_GOUGE        = 12540,
    SPELL_SHREDDER_PIERCE    = 6713,
};

enum Events
{
    EVENT_PIERCE   = 1,
    EVENT_THRASH   = 2,
    EVENT_SINISTER = 3,
    EVENT_GOUGE    = 4,
};

enum Says
{
    SAY_SHREDDER_AGGRO = 0,
    SAY_SHREDDER_DEATH = 1,
    SAY_SNEED_ENTER    = 2,
    SAY_SNEED_DEATH    = 3,
};

enum Creatures
{
    NPC_SNEEDS_SHREDDER = 642,
    NPC_SNEED           = 643,
};

Position const SneedJumpPos = { -170.0f, -810.0f, 18.5f, 0.0f };

class boss_sneeds_shredder : public CreatureScript
{
public:
    boss_sneeds_shredder() : CreatureScript("boss_sneeds_shredder") { }

    struct boss_sneeds_shredderAI : public ScriptedAI
    {
        boss_sneeds_shredderAI(Creature* c) : ScriptedAI(c), _isDead(false)
        {
        }

        EventMap events;
        bool _isDead;

        void Reset() override
        {
            events.Reset();
            DoCastSelf(SPELL_THRASH_PASSIVE, true);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_SHREDDER_AGGRO);
            events.ScheduleEvent(EVENT_PIERCE, 4000);
        }

        void JustDied(Unit* killer) override
        {
            if (_isDead)
                return;
            _isDead = true;

            Talk(SAY_SHREDDER_DEATH);

            // Spawn Sneed on top of the shredder and aggro the killer.
            if (Creature* sneed = me->SummonCreature(NPC_SNEED, me->GetPosition(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000))
            {
                sneed->AI()->Talk(SAY_SNEED_ENTER);
                if (killer)
                    sneed->AI()->AttackStart(killer);
                else if (Unit* t = SelectTargetFromPlayerList(50.0f))
                    sneed->AI()->AttackStart(t);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            while (uint32 e = events.ExecuteEvent())
            {
                if (e == EVENT_PIERCE)
                {
                    DoCastVictim(SPELL_SHREDDER_PIERCE);
                    events.ScheduleEvent(EVENT_PIERCE, urand(5000, 9000));
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const override
    {
        return GetDeadminesAI<boss_sneeds_shredderAI>(c);
    }
};

class boss_sneed : public CreatureScript
{
public:
    boss_sneed() : CreatureScript("boss_sneed") { }

    struct boss_sneedAI : public ScriptedAI
    {
        boss_sneedAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_SINISTER, 2000);
            events.ScheduleEvent(EVENT_GOUGE,    8000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_SNEED_DEATH);
            if (InstanceScript* i = me->GetInstanceScript())
                i->SetData(TYPE_SNEED, DONE);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            while (uint32 e = events.ExecuteEvent())
            {
                switch (e)
                {
                    case EVENT_SINISTER:
                        DoCastVictim(SPELL_SNEED_SINISTER);
                        events.ScheduleEvent(EVENT_SINISTER, urand(3000, 6000));
                        break;
                    case EVENT_GOUGE:
                        DoCastVictim(SPELL_SNEED_GOUGE);
                        events.ScheduleEvent(EVENT_GOUGE, urand(8000, 14000));
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const override
    {
        return GetDeadminesAI<boss_sneedAI>(c);
    }
};

void AddSC_boss_sneed()
{
    new boss_sneeds_shredder();
    new boss_sneed();
}
