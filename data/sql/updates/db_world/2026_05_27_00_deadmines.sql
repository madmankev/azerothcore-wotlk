-- Bind new Deadmines boss scripts
UPDATE `creature_template` SET `ScriptName` = 'boss_rhahkzor'        WHERE `entry` = 644;
UPDATE `creature_template` SET `ScriptName` = 'boss_sneeds_shredder' WHERE `entry` = 642;
UPDATE `creature_template` SET `ScriptName` = 'boss_sneed'           WHERE `entry` = 643;
UPDATE `creature_template` SET `ScriptName` = 'boss_gilnid'          WHERE `entry` = 1763;
UPDATE `creature_template` SET `ScriptName` = 'boss_captain_greenskin' WHERE `entry` = 647;
UPDATE `creature_template` SET `ScriptName` = 'boss_cookie'          WHERE `entry` = 645;
UPDATE `creature_template` SET `ScriptName` = 'boss_mr_smite'        WHERE `entry` = 646;
UPDATE `creature_template` SET `ScriptName` = 'boss_vancleef'        WHERE `entry` = 639;
UPDATE `gameobject_template` SET `ScriptName` = 'go_deadmines_cannon' WHERE `entry` = 16398;

-- Factory door opens when Rhahk'Zor dies
UPDATE `gameobject_template` SET `data1` = 1 WHERE `entry` = 13965;
