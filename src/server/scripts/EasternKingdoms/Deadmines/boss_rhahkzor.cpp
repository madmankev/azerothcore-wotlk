/*
 * Copyright (C) 2008-2018 TrinityCore <https://www.trinitycore.org/>
 * Deadmines: Rhahk'Zor
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

enum Spells
{
    SPELL_RHAHKZOR_SMITE     = 6432,
    SPELL_RHAHKZOR_HEAL      = 6433,
    SPELL_RHAHKZOR_DAZE      = 1604,
};

enum Events
{
    EVENT_SMITE  = 1,
    EVENT_HEAL   = 2,
    EVENT_DAZE   = 3,
};

enum Says
{
    SAY_AGGRO = 0,
    SAY_SMITE = 1,
    SAY_DEATH = 2,
};

class boss_rhahkzor : public CreatureScript
{
public:
    boss_rhahkzor() : CreatureScript("boss_rhahkzor") { }

    struct boss_rhahkzorAI : public ScriptedAI
    {
        boss_rhahkzorAI(Creature* creature) : ScriptedAI(creature)
        {
            Initialize();
        }

        void Initialize()
        {
            _below50 = false;
        }

        EventMap events;
        bool _below50;

        void Reset() override
        {
            events.Reset();
            Initialize();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SMITE, 3000);
            events.ScheduleEvent(EVENT_DAZE,  8000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            if (InstanceScript* inst = me->GetInstanceScript())
                inst->SetData(TYPE_RHAHK_ZOR, DONE);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (!_below50 && HealthBelowPct(50))
            {
                _below50 = true;
                Talk(SAY_SMITE);
                DoCastSelf(SPELL_RHAHKZOR_HEAL);
                events.ScheduleEvent(EVENT_HEAL, 12000);
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SMITE:
                        DoCastVictim(SPELL_RHAHKZOR_SMITE);
                        events.ScheduleEvent(EVENT_SMITE, urand(4000, 7000));
                        break;
                    case EVENT_HEAL:
                        if (HealthBelowPct(60))
                            DoCastSelf(SPELL_RHAHKZOR_HEAL);
                        events.ScheduleEvent(EVENT_HEAL, 15000);
                        break;
                    case EVENT_DAZE:
                        if (Unit* v = me->GetVictim())
                            DoCast(v, SPELL_RHAHKZOR_DAZE);
                        events.ScheduleEvent(EVENT_DAZE, urand(10000, 16000));
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_rhahkzorAI>(creature);
    }
};

void AddSC_boss_rhahkzor()
{
    new boss_rhahkzor();
}
