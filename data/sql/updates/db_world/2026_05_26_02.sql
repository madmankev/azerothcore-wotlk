-- DB update 2026_05_26_01 -> 2026_05_26_02
-- Bind the AQ War Effort commander NPCs to the gossip script so players
-- can exchange Commendation Signets for higher-tier supply crates.
UPDATE `creature_template`
SET `ScriptName` = 'npc_aq_war_effort_collector'
WHERE `entry` IN (15700, 15701);
