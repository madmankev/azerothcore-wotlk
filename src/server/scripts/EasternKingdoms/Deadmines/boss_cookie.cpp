/*
 * Deadmines: Cookie the ship's cook.
 * A simple murloc encounter with a couple of food-themed abilities.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

enum Spells
{
    SPELL_ACID_SPLASH    = 65880,
    SPELL_MORTAL_WOUND   = 12003,
    SPELL_FOOD_TOSS      = 73957,
};

enum Events
{
    EVENT_ACID   = 1,
    EVENT_WOUND  = 2,
    EVENT_FOOD   = 3,
};

enum Says
{
    SAY_AGGRO = 0,
    SAY_DEATH = 1,
};

class boss_cookie : public CreatureScript
{
public:
    boss_cookie() : CreatureScript("boss_cookie") { }

    struct boss_cookieAI : public ScriptedAI
    {
        boss_cookieAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_ACID,  4000);
            events.ScheduleEvent(EVENT_WOUND, 8000);
            events.ScheduleEvent(EVENT_FOOD, 12000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            if (InstanceScript* i = me->GetInstanceScript())
                i->SetData(TYPE_SMITE, DONE); // Cookie is tied to the ship area
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
                    case EVENT_ACID:
                        DoCastVictim(SPELL_ACID_SPLASH);
                        events.ScheduleEvent(EVENT_ACID, urand(5000, 8000));
                        break;
                    case EVENT_WOUND:
                        DoCastVictim(SPELL_MORTAL_WOUND);
                        events.ScheduleEvent(EVENT_WOUND, urand(8000, 13000));
                        break;
                    case EVENT_FOOD:
                        if (Unit* t = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                            DoCast(t, SPELL_FOOD_TOSS);
                        events.ScheduleEvent(EVENT_FOOD, urand(10000, 16000));
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const override
    {
        return GetDeadminesAI<boss_cookieAI>(c);
    }
};

void AddSC_boss_cookie()
{
    new boss_cookie();
}
