-- AQ War Effort turn-in framework.
--
-- Each row binds one collector NPC to one resource. The C++ script
-- npc_aq_war_effort_collector reads this table at startup.
--
-- Historically accurate turn-in sizes, item totals and signet rewards are
-- taken from the 1.9/1.12 realm event. The world_state column is left at 0
-- because the exact client fields per resource were not shipped with 3.3.5
-- data; set them per realm if you have a DBC extract that defines them.

DROP TABLE IF EXISTS `creature_aq_war_effort`;
CREATE TABLE `creature_aq_war_effort` (
  `creature_id`      INT UNSIGNED NOT NULL,
  `item_id`          INT UNSIGNED NOT NULL,
  `item_count`       INT UNSIGNED NOT NULL,
  `reward_item`      INT UNSIGNED NOT NULL DEFAULT 21509,
  `reward_count`     INT UNSIGNED NOT NULL DEFAULT 1,
  `signet_item`      INT UNSIGNED NOT NULL DEFAULT 0,
  `signet_count`     INT UNSIGNED NOT NULL DEFAULT 0,
  `world_state`      INT UNSIGNED NOT NULL DEFAULT 0,
  `goal`             INT UNSIGNED NOT NULL,
  `completed_event`  SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `gossip_menu_id`   MEDIUMINT UNSIGNED NOT NULL DEFAULT 0,
  `gossip_text_done`MEDIUMINT UNSIGNED NOT NULL DEFAULT 0,
  `faction`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`creature_id`, `item_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COMMENT='AQ War Effort turn-in definitions';

--
-- Both factions
--

-- Copper Bar (90,000 total, 20 per turn-in, 1 signet)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` IN (15459, 15383) AND `item_id` = 2840;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15459, 2840, 20, 21436, 1, 90000, 1), -- Miner Cromwell (Ironforge)
(15383, 2840, 20, 21438, 1, 90000, 2); -- Sergeant Stonebrow (Orgrimmar)

-- Purple Lotus (26,000 total, 20 per turn-in, 7 signets)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` IN (15437, 15512) AND `item_id` = 8831;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15437, 8831, 20, 21436, 7, 26000, 1), -- Master Nightsong
(15512, 8831, 20, 21438, 7, 26000, 2); -- Apothecary Jezel

-- Thick Leather (80,000 total, 10 per turn-in, 7 signets)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` IN (15450, 15522) AND `item_id` = 4304;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15450, 4304, 10, 21436, 7, 80000, 1), -- Marta Finespindle
(15522, 4304, 10, 21438, 7, 80000, 2); -- Sergeant Umala

-- Spotted Yellowtail (17,000 total, 20 per turn-in, 7 signets)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` IN (15457, 15535) AND `item_id` = 6887;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15457, 6887, 20, 21436, 7, 17000, 1), -- Huntress Swiftriver
(15535, 6887, 20, 21438, 7, 17000, 2); -- Chief Sharpclaw

-- Runecloth Bandage (400,000 total, 20 per turn-in, 10 signets)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` IN (15453, 15532) AND `item_id` = 14529;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15453, 14529, 20, 21436, 10, 400000, 1), -- Keeper Moonshade
(15532, 14529, 20, 21438, 10, 400000, 2); -- Stoneguard Clayhoof

--
-- Alliance only (Ironforge, Military Ward)
--

-- Linen Bandage (800,000 / 20 / 1)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15451 AND `item_id` = 1251;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15451, 1251, 20, 21436, 1, 800000, 1); -- Sentinel Silversky

-- Light Leather (180,000 / 20 / 1)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15446 AND `item_id` = 2318;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15446, 2318, 20, 21436, 1, 180000, 1); -- Bonnie Stoneflayer

-- Medium Leather (110,000 / 10 / 3)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15448 AND `item_id` = 2319;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15448, 2319, 10, 21436, 3, 110000, 1); -- Private Porter

-- Stranglekelp (33,000 / 20 / 3)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15434 AND `item_id` = 3820;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15434, 3820, 20, 21436, 3, 33000, 1); -- Private Draxlegauge

-- Rainbow Fin Albacore (14,000 / 20 / 3)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15455 AND `item_id` = 2115;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15455, 5095, 20, 21436, 3, 14000, 1); -- Slicky Gastronome

-- Silk Bandage (600,000 / 20 / 5)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15452 AND `item_id` = 6450;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15452, 6450, 20, 21436, 5, 600000, 1); -- Nurse Stonefield

-- Iron Bar (28,000 / 20 / 5)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15431 AND `item_id` = 3575;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15431, 3575, 20, 21436, 5, 28000, 1); -- Corporal Carnes

-- Roast Raptor (20,000 / 20 / 5)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15456 AND `item_id` = 12210;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15456, 12210, 20, 21436, 5, 20000, 1); -- Sarah Sadwhistle

-- Thorium Bar (24,000 / 20 / 10)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15432 AND `item_id` = 12359;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15432, 12359, 20, 21436, 10, 24000, 1); -- Dame Twinbraid

-- Arthas' Tears (20,000 / 20 / 10)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15445 AND `item_id` = 8836;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15445, 8836, 20, 21436, 10, 20000, 1); -- Sergeant Major Germaine

--
-- Horde only (Orgrimmar, Valley of Spirits)
--

-- Peacebloom (96,000 / 20 / 1)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15477 AND `item_id` = 2447;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15477, 2447, 20, 21438, 1, 96000, 2); -- Herbalist Proudfeather

-- Lean Wolf Steak (10,000 / 20 / 1)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15533 AND `item_id` = 12209;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15533, 12209, 20, 21438, 1, 10000, 2); -- Bloodguard Rawtar

-- Tin Bar (22,000 / 20 / 3)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15460 AND `item_id` = 3576;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15460, 3576, 20, 21438, 3, 22000, 2); -- Grunt Maug

-- Wool Bandage (250,000 / 20 / 3)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15528 AND `item_id` = 3530;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15528, 3530, 20, 21438, 3, 250000, 2); -- Healer Longrunner

-- Firebloom (19,000 / 20 / 5)
-- No dedicated Firebloom collector exists in 3.3.5 data. Operators can add a
-- custom row here once the correct NPC entry is sourced.

-- Heavy Leather (60,000 / 10 / 5)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15515 AND `item_id` = 4234;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15515, 4234, 10, 21438, 5, 60000, 2); -- Skinner Jamani

-- Mithril Bar (18,000 / 20 / 7)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15469 AND `item_id` = 3860;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15469, 3860, 20, 21438, 7, 18000, 2); -- Senior Sergeant T'kelah

-- Mageweave Bandage (250,000 / 20 / 7)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15529 AND `item_id` = 8544;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15529, 8544, 20, 21438, 7, 250000, 2); -- Lady Callow

-- Rugged Leather (60,000 / 10 / 10)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15525 AND `item_id` = 8170;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15525, 8170, 10, 21438, 10, 60000, 2); -- Doctor Serratus

-- Baked Salmon (10,000 / 20 / 10)
DELETE FROM `creature_aq_war_effort` WHERE `creature_id` = 15535 AND `item_id` = 13933;
INSERT INTO `creature_aq_war_effort` (`creature_id`, `item_id`, `item_count`, `signet_item`, `signet_count`, `goal`, `faction`) VALUES
(15535, 13935, 20, 21438, 10, 10000, 2); -- Chief Sharpclaw
