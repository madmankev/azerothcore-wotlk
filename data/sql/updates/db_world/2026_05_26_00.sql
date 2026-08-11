-- DB update 2022_08_25_05 -> 2026_05_26_00
-- AQ War Effort turn-in framework table.
DROP TABLE IF EXISTS `creature_aq_war_effort`;
CREATE TABLE `creature_aq_war_effort` (
  `creature_id`      INT UNSIGNED NOT NULL,
  `item_id`          INT UNSIGNED NOT NULL,
  `item_count`       INT UNSIGNED NOT NULL,
  `reward_item`      INT UNSIGNED NOT NULL DEFAULT 0,
  `reward_count`     INT UNSIGNED NOT NULL DEFAULT 0,
  `world_state`      INT UNSIGNED NOT NULL,
  `goal`             INT UNSIGNED NOT NULL,
  `completed_event`  SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `gossip_menu_id`   MEDIUMINT UNSIGNED NOT NULL DEFAULT 0,
  `gossip_text_done`MEDIUMINT UNSIGNED NOT NULL DEFAULT 0,
  `faction`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`creature_id`, `item_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COMMENT='AQ War Effort turn-in definitions';
