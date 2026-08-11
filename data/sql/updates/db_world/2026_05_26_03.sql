-- DB update 2026_05_26_02 -> 2026_05_26_03
-- Persisted state for the AQ War Effort post-collection phases.
DROP TABLE IF EXISTS `aq_war_effort_save`;
CREATE TABLE `aq_war_effort_save` (
  `phase`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `phase_end` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COMMENT='AQ War Effort phase state';

INSERT INTO `aq_war_effort_save` (`phase`, `phase_end`) VALUES (0, 0);
