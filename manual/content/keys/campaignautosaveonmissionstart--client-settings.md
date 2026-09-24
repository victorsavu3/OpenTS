---
key: CampaignAutosaveOnMissionStart
scope: client-settings
label: Autosave at mission start
when_omitted:
  kind: value
  value: "yes"
---

`CampaignAutosaveOnMissionStart=yes` writes a save as soon as a campaign mission starts, described `Start of mission: <name>` in the load dialog. `CampaignAutosaveOnMissionStart=no` turns this off.

The save is written under a name keyed to the mission, so restarting or replaying the same mission overwrites its own file rather than adding another. [Save games](/formats/save-games/#mission-checkpoints) owns the file name and how it differs from the rotating [automatic save](/formats/save-games/#automatic-saves).

The key applies only to a single-player campaign; skirmish and multiplayer games are unaffected. It is read from `sun.ini` when the game starts and written back with the other options; no dialog offers it.
