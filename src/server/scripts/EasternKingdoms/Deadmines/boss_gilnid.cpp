/*
 * Deadmines: Gilnid the Smelter.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

enum Spells
{
    SPELL_SMELT          = 7276,
    SPELL_LIQUID_FIRE    = 7275,
    SPELL_SUMMON_GOBLIN  = 7277,
};

enum Events
{
    EVENT_SMELT          = 1,
    EVENT_LIQUID_FIRE    = 2,
    EVENT_SUMMON_GOBLIN  = 3,
};

enum Says
{
    SAY_AGGRO            = 0,
    SAY_SUMMON           = 1,
    SAY_DEATH            = 2,
};

enum Creatures
{
    NPC_DEFIAS_EVOKER    = 450, // Defias Evoker (add)
    NPC_DEFIAS_WIZARD    = 449, // Defias Renegade Mage (add)
};

class boss_gilnid : public CreatureScript
{
public:
    boss_gilnid() : CreatureScript("boss_gilnid") { }

    struct boss_gilnidAI : public ScriptedAI
    {
        boss_gilnidAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SMELT, 4000);
            events.ScheduleEvent(EVENT_LIQUID_FIRE, 7000);
            events.ScheduleEvent(EVENT_SUMMON_GOBLIN, 20000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            if (InstanceScript* i = me->GetInstanceScript())
                i->SetData(TYPE_GILNID, DONE);
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
                    case EVENT_SMELT:
                        DoCastVictim(SPELL_SMELT);
                        events.ScheduleEvent(EVENT_SMELT, urand(5000, 9000));
                        break;
                    case EVENT_LIQUID_FIRE:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                            DoCast(target, SPELL_LIQUID_FIRE);
                        events.ScheduleEvent(EVENT_LIQUID_FIRE, urand(8000, 14000));
                        break;
                    case EVENT_SUMMON_GOBLIN:
                        Talk(SAY_SUMMON);
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            if (Creature* add = me->SummonCreature(
                                (i == 0 ? NPC_DEFIAS_EVOKER : NPC_DEFIAS_WIZARD),
                                me->GetPositionX() + cos(i) * 3.0f,
                                me->GetPositionY() + sin(i) * 3.0f,
                                me->GetPositionZ(), 0,
                                TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
                            {
                                if (Unit* t = SelectTargetFromPlayerList(50.0f))
                                    add->AI()->AttackStart(t);
                            }
                        }
                        events.ScheduleEvent(EVENT_SUMMON_GOBLIN, 25000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const override
    {
        return GetDeadminesAI<boss_gilnidAI>(c);
    }
};

void AddSC_boss_gilnid()
{
    new boss_gilnid();
}
