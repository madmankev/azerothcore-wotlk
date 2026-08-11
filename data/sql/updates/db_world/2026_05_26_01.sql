-- DB update 2026_05_26_00 -> 2026_05_26_01
-- Bind the AQ War Effort turn-in script to all collectors that have rows
-- in creature_aq_war_effort.
UPDATE `creature_template`
SET `ScriptName` = 'npc_aq_war_effort_collector'
WHERE `entry` IN (
    15383, -- Sergeant Stonebrow
    15431, -- Corporal Carnes
    15432, -- Dame Twinbraid
    15434, -- Private Draxlegauge
    15437, -- Master Nightsong
    15445, -- Sergeant Major Germaine
    15446, -- Bonnie Stoneflayer
    15448, -- Private Porter
    15450, -- Marta Finespindle
    15451, -- Sentinel Silversky
    15452, -- Nurse Stonefield
    15453, -- Keeper Moonshade
    15455, -- Slicky Gastronome
    15456, -- Sarah Sadwhistle
    15457, -- Huntress Swiftriver
    15459, -- Miner Cromwell
    15460, -- Grunt Maug
    15469, -- Senior Sergeant T'kelah
    15477, -- Herbalist Proudfeather
    15512, -- Apothecary Jezel
    15515, -- Skinner Jamani
    15522, -- Sergeant Umala
    15525, -- Doctor Serratus
    15528, -- Healer Longrunner
    15529, -- Lady Callow
    15532, -- Stoneguard Clayhoof
    15533, -- Bloodguard Rawtar
    15535  -- Chief Sharpclaw
);
