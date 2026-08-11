-- DB update 2022_08_25_05 -> 2026_05_26_00
-- AQ War Effort turn-in framework.
DROP TABLE IF EXISTS `creature_aq_war_effort`;
CREATE TABLE `creature_aq_war_effort` (
  `creature_id`      INT UNSIGNED NOT NULL COMMENT 'Collector NPC entry',
  `item_id`          INT UNSIGNED NOT NULL COMMENT 'Item handed in per turn-in',
  `item_count`       INT UNSIGNED NOT NULL COMMENT 'Stack size consumed per turn-in',
  `reward_item`      INT UNSIGNED NOT NULL DEFAULT 21509 COMMENT "Ahn'Qiraj War Effort Supplies crate",
  `reward_count`     INT UNSIGNED NOT NULL DEFAULT 1,
  `signet_item`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Commendation Signet (21436 Alliance / 21438 Horde)',
  `signet_count`     INT UNSIGNED NOT NULL DEFAULT 0,
  `world_state`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Client world-state field for the progress bar',
  `goal`             INT UNSIGNED NOT NULL,
  `completed_event`  SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `gossip_menu_id`   MEDIUMINT UNSIGNED NOT NULL DEFAULT 0,
  `gossip_text_done`MEDIUMINT UNSIGNED NOT NULL DEFAULT 0,
  `faction`          TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 neutral, 1 Alliance, 2 Horde',
  PRIMARY KEY (`creature_id`, `item_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COMMENT='AQ War Effort turn-in definitions';
