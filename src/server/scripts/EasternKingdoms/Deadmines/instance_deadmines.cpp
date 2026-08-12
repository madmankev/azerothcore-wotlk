/*
 * Deadmines instance script.
 */

#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "deadmines.h"

class instance_deadmines : public InstanceMapScript
{
public:
    instance_deadmines() : InstanceMapScript(DeadminesScriptName, 36) { }

    struct instance_deadmines_InstanceMapScript : public InstanceScript
    {
        instance_deadmines_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            Initialize();
        }

        void Initialize()
        {
            memset(&_encounters, 0, sizeof(_encounters));
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_HEAVY_DOOR_1:
                case GO_HEAVY_DOOR_2:
                case GO_DOOR_LEVER_1:
                case GO_DOOR_LEVER_2:
                case GO_DOOR_LEVER_3:
                case GO_CANNON:
                    go->UpdateSaveToDb(true);
                    break;
                case GO_FACTORY_DOOR:
                    go->UpdateSaveToDb(true);
                    if (_encounters[TYPE_RHAHK_ZOR] == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_IRON_CLAD_DOOR:
                    go->UpdateSaveToDb(true);
                    if (go->GetStateSavedOnInstance() == GO_STATE_ACTIVE)
                        go->DespawnOrUnsummon();
                    break;
            }
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_RHAHK_ZOR:
                case NPC_SNEED:
                case NPC_SNEEDS_SHREDDER:
                case NPC_GILNID:
                case NPC_MR_SMITE:
                case NPC_CAPTAIN_GREENSKIN:
                case NPC_EDWIN_VANCLEEF:
                case NPC_COOKIE:
                    creature->SetFullHealth();
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type < MAX_ENCOUNTERS)
            {
                _encounters[type] = data;

                // Open doors and spawn adds when bosses die.
                if (data == DONE)
                {
                    switch (type)
                    {
                        case TYPE_RHAHK_ZOR:
                            HandleGameObject(GO_FACTORY_DOOR, true);
                            break;
                        case TYPE_SMITE:
                            HandleGameObject(GO_IRON_CLAD_DOOR, true);
                            break;
                        default:
                            break;
                    }
                    SaveToDB();
                }
            }
        }

        uint32 GetData(uint32 type) const override
        {
            return (type < MAX_ENCOUNTERS) ? _encounters[type] : 0;
        }

        std::string GetSaveData() override
        {
            std::ostringstream saveStream;
            saveStream << "D E";
            for (uint8 i = 0; i < MAX_ENCOUNTERS; ++i)
                saveStream << ' ' << _encounters[i];
            return saveStream.str();
        }

        void Load(const char* in) override
        {
            if (!in)
                return;

            char head1, head2;
            std::istringstream loadStream(in);
            loadStream >> head1 >> head2;
            if (head1 == 'D' && head2 == 'E')
            {
                for (uint8 i = 0; i < MAX_ENCOUNTERS; ++i)
                {
                    loadStream >> _encounters[i];
                    if (_encounters[i] == IN_PROGRESS)
                        _encounters[i] = NOT_STARTED;
                }
            }
        }

    private:
        uint32 _encounters[MAX_ENCOUNTERS];
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_deadmines_InstanceMapScript(map);
    }
};

void AddSC_instance_deadmines()
{
    new instance_deadmines();
}
