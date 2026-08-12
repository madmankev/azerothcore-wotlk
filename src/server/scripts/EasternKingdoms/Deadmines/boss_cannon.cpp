/*
 * Deadmines: Cannon event that blows the door leading to Mr. Smite.
 * A player fires the cannon; Letharn/Defias Pirate then spawns from
 * it, and the Iron Clad Door is opened. Largely implemented via
 * gameobject spell in DB, but we tie it to the instance state.
 */

#include "GameObjectAI.h"
#include "ScriptMgr.h"
#include "deadmines.h"

class go_deadmines_cannon : public GameObjectScript
{
public:
    go_deadmines_cannon() : GameObjectScript("go_deadmines_cannon") { }

    struct go_deadmines_cannonAI : public GameObjectAI
    {
        go_deadmines_cannonAI(GameObject* go) : GameObjectAI(go) { }

        bool GossipHello(Player* player, GameObject* /*go*/) override
        {
            InstanceScript* inst = me->GetInstanceScript();
            if (!inst)
                return false;

            if (inst->GetData(TYPE_CANNON) == DONE)
                return false;

            if (player->HasItemCount(277736, 1) || player->HasItemCount(25693, 1)) // Large Cannon Shell
                inst->SetData(TYPE_CANNON, DONE);
            return true;
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return GetDeadminesAI<go_deadmines_cannonAI>(go);
    }
};

void AddSC_boss_cannon()
{
    new go_deadmines_cannon();
}
