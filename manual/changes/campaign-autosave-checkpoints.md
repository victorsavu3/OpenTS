---
title: Two campaign autosave checkpoints, at mission start and before mission selection
category: feature
release: 0.2.0
targets:
- type: key
  id: CampaignAutosaveOnMissionStart
  effect: added
- type: key
  id: CampaignAutosaveBeforeVictory
  effect: added
credit:
- Victor
---

A campaign mission now writes two checkpoint saves of its own, separate from the existing rotating automatic save. `CampaignAutosaveOnMissionStart=yes` saves as the mission starts, described `Start of mission: <name>`. `CampaignAutosaveBeforeVictory=yes` saves right before the map selection screen, described `Before choosing the next mission: <name>`; loading it reopens that screen so a different mission can be chosen. Both default to `yes` and apply only to a single-player campaign.
