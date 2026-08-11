-- DB update 2026_05_26_03 -> 2026_05_26_04
-- Bind the Scarab Gong to the AQ gate-opening script.
UPDATE `gameobject_template` SET `ScriptName` = 'go_scarab_gong_aq'
WHERE `entry` IN (180717, 180718);
