-- Bind the Seduction SpellScript that restricts the spell to humanoids.
DELETE FROM `spell_script_names` WHERE `spell_id` = 6358 AND `ScriptName` = 'spell_warl_seduction_spell';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(6358, 'spell_warl_seduction_spell');
