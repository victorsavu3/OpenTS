---
key: CampaignAutosaveBeforeVictory
scope: client-settings
label: Autosave before mission selection
when_omitted:
  kind: value
  value: "yes"
---

`CampaignAutosaveBeforeVictory=yes` writes a save right before the map selection screen that follows a won campaign mission, described `Before choosing the next mission: <name>` in the load dialog. `CampaignAutosaveBeforeVictory=no` turns this off.

Loading that save returns to the map selection screen instead of ordinary gameplay, so a different next mission can be chosen than was chosen the first time. [Save games](/formats/save-games/#mission-checkpoints) owns the file name and this behavior. The save is keyed to the mission just won, so winning it again overwrites the same file.

The key applies only to a single-player campaign; skirmish and multiplayer games are unaffected, and a mission that skips map selection with [`SkipMapSelect=yes`](/keys/skipmapselect/) writes no such save. It is read from `sun.ini` when the game starts and written back with the other options; no dialog offers it.
