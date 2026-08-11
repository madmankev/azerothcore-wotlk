/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

enum Spells
{
    // VanCleef has a main-hand and off-hand sword; retail-style toolkit.
    SPELL_THRASH_PASSIVE     = 12787, // 5% extra attack (passive, applied in DB)
    SPELL_VANCLEEF_BACKSTAB  = 9210,  // used on random targets
    SPELL_VANCLEEF_STEALTH   = 30840, // visual enter-stealth on reset
    SPELL_VANCLEEF_TELEPORT  = 5209,  // shadow step-like teleport to target
};

enum Events
{
    EVENT_BACKSTAB    = 1,
    EVENT_TELEPORT   = 2,
    EVENT_RESUMMON   = 3,
};

enum Says
{
    SAY_AGGRO        = 0, // "Lapdogs, all of you!"
    SAY_DUAL_WIELD   = 1,
    SAY_DEATH        = 2, // "I'll get you, I'll get you all!"
    SAY_KILL         = 3,
};

enum Creatures
{
    NPC_DEFIAS_BLACKGUARD = 636,
};

// Boss position - the deck of his ship in the Deadmines.
Position const DefiasBlackguardPositions[2] =
{
    { -65.0f, -820.0f, 42.0f, 0.0f },
    { -78.0f, -820.0f, 42.0f, 0.0f },
};

class boss_vancleef : public CreatureScript
{
public:
    boss_vancleef() : CreatureScript("boss_vancleef") { }

    struct boss_vancleefAI : public ScriptedAI
    {
        boss_vancleefAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            Initialize();
        }

        void Initialize()
        {
            _didIntro = false;
        }

        EventMap events;
        SummonList summons;
        bool _didIntro;

        void Reset() override
        {
            events.Reset();
            summons.DespawnAll();
            Initialize();

            // VanCleef is a dual-wielding rogue; make sure he has both swords.
            me->LoadEquipment(1);
            me->SetCanDualWield(true);

            if (!me->IsInCombat())
                me->SetStandState(UNIT_STAND_STATE_STAND);
        }

        // Defias Blackguards that guard the captain's cabin are called
        // into the fight once the encounter starts, matching the retail
        // behavior of the two adds standing near VanCleef.
        void SummonBlackguards()
        {
            for (uint8 i = 0; i < 2; ++i)
            {
                if (Creature* add = me->SummonCreature(NPC_DEFIAS_BLACKGUARD,
                                                       DefiasBlackguardPositions[i],
                                                       TEMPSUMMON_CORPSE_TIMED_DESPAWN,
                                                       30000))
                {
                    summons.Summon(add);
                    if (Unit* target = SelectTargetFromPlayerList(60.0f))
                        add->AI()->AttackStart(target);
                }
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!_didIntro && who && who->GetTypeId() == TYPEID_PLAYER &&
                me->IsWithinDistInMap(who, 20.0f) && me->IsValidAttackTarget(who))
            {
                Talk(SAY_AGGRO);
                _didIntro = true;
            }
            ScriptedAI::MoveInLineOfSight(who);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            me->CallForHelp(12.0f);
            SummonBlackguards();

            events.ScheduleEvent(EVENT_BACKSTAB,  4000);
            events.ScheduleEvent(EVENT_TELEPORT, 12000);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_KILL);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();
        }

        void JustSummoned(Creature* summoned) override
        {
            summons.Summon(summoned);
        }

        void JustReachedHome() override
        {
            summons.DespawnAll();
            ScriptedAI::JustReachedHome();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_BACKSTAB:
                    // Backstab a random player - VanCleef is a rogue.
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 5.0f, true))
                        DoCast(target, SPELL_VANCLEEF_BACKSTAB);
                    events.ScheduleEvent(EVENT_BACKSTAB, urand(4000, 7000));
                    break;
                case EVENT_TELEPORT:
                    // Periodically teleport to a random ranged target to
                    // encourage repositioning - classic rogue behavior.
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    {
                        DoCast(target, SPELL_VANCLEEF_TELEPORT, true);
                        AttackStart(target);
                    }
                    events.ScheduleEvent(EVENT_TELEPORT, urand(10000, 18000));
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDeadminesAI<boss_vancleefAI>(creature);
    }
};

// Two Defias Blackguards flanking VanCleef on his ship. They are simple
// adds that melee and have a Mortal Strike-like disarm, mirroring their
// warrior archetype.
class npc_deadmines_blackguard : public CreatureScript
{
public:
    npc_deadmines_blackguard() : CreatureScript("npc_deadmines_blackguard") { }

    struct npc_deadmines_blackguardAI : public ScriptedAI
    {
        npc_deadmines_blackguardAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            me->CallForHelp(8.0f);
            events.ScheduleEvent(1, 6000);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (events.ExecuteEvent() == 1)
            {
                if (Unit* victim = me->GetVictim())
                    DoCast(victim, 6713); // Disarm
                events.ScheduleEvent(1, urand(8000, 14000));
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const override
    {
        return GetDeadminesAI<npc_deadmines_blackguardAI>(c);
    }
};

void AddSC_boss_vancleef()
{
    new boss_vancleef();
    new npc_deadmines_blackguard();
}
