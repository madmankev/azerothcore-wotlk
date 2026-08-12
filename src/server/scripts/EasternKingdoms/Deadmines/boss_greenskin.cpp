/*
 * Deadmines: Captain Greenskin.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

enum Spells
{
    SPELL_REND            = 13738,
    SPELL_MORTAL_STRIKE   = 15708,
    SPELL_PUMMEL          = 15615,
    SPELL_INTIMIDATING_SHOUT = 1913,
};

enum Events
{
    EVENT_MORTAL = 1,
    EVENT_REND,
    EVENT_PUMMEL,
    EVENT_SHOUT,
};

enum Says
{
    SAY_AGGRO = 0,
    SAY_DEATH = 1,
};

enum Creatures
{
    NPC_DEFIAS_PIRATE    = 636, // Defias Blackguard (add)
};

class boss_captain_greenskin : public CreatureScript
{
public:
    boss_captain_greenskin() : CreatureScript("boss_captain_greenskin") { }

    struct boss_captain_greenskinAI : public ScriptedAI
    {
        boss_captain_greenskinAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            Talk(SAY_AGGRO, who);
            events.ScheduleEvent(EVENT_MORTAL, 6000);
            events.ScheduleEvent(EVENT_REND, 2000);
            events.ScheduleEvent(EVENT_PUMMEL, 10000);
            events.ScheduleEvent(EVENT_SHOUT, 18000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            if (InstanceScript* i = me->GetInstanceScript())
                i->SetData(TYPE_GREENSKIN, DONE);
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
                    case EVENT_MORTAL:
                        DoCastVictim(SPELL_MORTAL_STRIKE);
                        events.ScheduleEvent(EVENT_MORTAL, urand(7000, 12000));
                        break;
                    case EVENT_REND:
                        DoCastVictim(SPELL_REND);
                        events.ScheduleEvent(EVENT_REND, urand(6000, 10000));
                        break;
                    case EVENT_PUMMEL:
                        if (Unit* v = me->GetVictim())
                            if (v->IsNonMeleeSpellCast(false))
                                DoCast(v, SPELL_PUMMEL);
                        events.ScheduleEvent(EVENT_PUMMEL, urand(8000, 15000));
                        break;
                    case EVENT_SHOUT:
                        DoCastAOE(SPELL_INTIMIDATING_SHOUT);
                        events.ScheduleEvent(EVENT_SHOUT, 25000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const override
    {
        return GetDeadminesAI<boss_captain_greenskinAI>(c);
    }
};

void AddSC_boss_greenskin()
{
    new boss_captain_greenskin();
}
