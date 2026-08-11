/*
 * The Scarab Gong - Ahn'Qiraj gate opening.
 *
 * Players who have reached the final Scepter turn-in (quest 8743
 * "Bang a Gong!") can ring the Scarab Gong once the War Effort phase
 * has reached PHASE_COMPLETE. Ringing it broadcasts the realm-wide
 * announcement, rewards the quest for everyone in the ringer's group
 * and advances the phase to PHASE_OPEN so the gong can't be re-rung.
 */

#include "aq_war_effort.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "World.h"

enum
{
    QUEST_BANG_A_GONG           = 8743,

    GONG_YELL_CHAMPION         = 11427, // $n ... has rung the Scarab Gong...
};

class go_scarab_gong_aq : public GameObjectScript
{
public:
    go_scarab_gong_aq() : GameObjectScript("go_scarab_gong_aq") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!player || !go)
            return false;

        if (sAqWarEffortMgr->GetPhase() != PHASE_COMPLETE)
        {
            player->Say("The gong is silent. The war must be won before the gates may open.", LANG_UNIVERSAL);
            return true;
        }

        if (player->GetQuestStatus(QUEST_BANG_A_GONG) != QUEST_STATUS_INCOMPLETE)
        {
            player->Say("Only a bearer of the Scepter of the Shifting Sands may ring the gong.", LANG_UNIVERSAL);
            return true;
        }

        RingTheGong(player, go);
        return true;
    }

    static void RingTheGong(Player* ringer, GameObject* go)
    {
        // Realm-wide broadcast of the champion's name using broadcast_text 11427.
        ringer->Yell(GONG_YELL_CHAMPION);

        // Also send the plain text realm-wide via a chat packet. The
        // broadcast_text 11427 uses a $n placeholder; for a manual packet
        // we substitute the ringer's name directly.
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID_BOSS_EMOTE, LANG_UNIVERSAL,
                                     ringer->GetGUID(), ObjectGuid::Empty,
                                     Acore::StringFormat("{}, Champion of the Bronze Dragonflight, has rung the Scarab Gong. "
                                                         "The ancient gates of Ahn\'Qiraj open, revealing the horrors of a forgotten war...",
                                                         ringer->GetName()));
        sWorld->SendGlobalMessage(&data);

        RewardGong(ringer, go);

        if (Group* group = ringer->GetGroup())
        {
            for (auto const& slot : group->GetMemberSlots())
            {
                Player* member = ObjectAccessor::FindPlayer(slot.guid);
                if (member && member != ringer)
                    RewardGong(member, go);
            }
        }

        sAqWarEffortMgr->SetPhase(PHASE_OPEN, false);
    }

    static void RewardGong(Player* player, GameObject* go)
    {
        if (player->GetQuestStatus(QUEST_BANG_A_GONG) != QUEST_STATUS_INCOMPLETE)
            return;

        Quest const* q = sObjectMgr->GetQuestTemplate(QUEST_BANG_A_GONG);
        if (!q)
            return;

        player->CompleteQuest(QUEST_BANG_A_GONG);
        player->RewardQuest(q, 0, go, true);
    }
};

void AddSC_aq_scarab_gong()
{
    new go_scarab_gong_aq();
}
