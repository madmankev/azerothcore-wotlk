-- AQ War Effort turn-in framework.
--
-- Each row binds one collector NPC entry to one resource turn-in. The C++
-- script `npc_aq_war_effort_collector` reads this table at startup.
--
-- Columns:
--   creature_id       NPC entry of the War Effort recruiter/commander.
--   item_id           item consumed per turn-in.
--   item_count        stack size consumed per turn-in.
--   reward_item       supply-crate item awarded (0 = no reward).
--   reward_count      number of reward items per turn-in.
--   world_state       client world-state field for the progress bar.
--   goal              total items required for this resource.
--   completed_event   optional game_event started when the goal is met (0 = none).
--   gossip_menu_id    base gossip text shown on greet (0 = npc_text default).
--   gossip_text_done  gossip text shown once the goal is reached (0 = default).
--   faction           0 neutral, 1 Alliance, 2 Horde.
--
-- No rows are shipped by default: historically accurate item IDs, counts and
-- world-state fields are realm/DBC specific and should be sourced from a
-- 1.9/1.12 or Classic-era data extract. Add rows below once you have them.

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
