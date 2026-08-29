# Bundled simulations

This folder holds the rulesets and city saves shipped with the demo. For the `.ogs` language itself, see the [script language reference](../../../doc/script.md).

## File formats

- **`.ogs`**: ruleset (or mod): resources, rules, layers, paths, segments, agents, buildings, and zones.
- **`.ogc`**: city save: header (ruleset name, SHA-256 of the `.ogs`, types in use), geometry, and live state (clock, globals, layer cells, buildings, agents, segment flows).

A separate world file is not needed. A save identifies the ruleset it was created with. Loading fails if a required type is missing or the ruleset hash differs. **Apply** in the Script panel stamps the saves sitting beside the ruleset with its new fingerprint, so editing a script here does not leave the bundled cities unopenable. Failing that, File → **Open saves with a stale checksum** bypasses the check; required types must still exist.

**New city** selects a `.ogs` ruleset and starts with an empty city. **Apply** in the Script panel reparses the current ruleset and keeps the geometry if every placed type remains defined.

## Bundled scenarios

Two rulesets are shipped, and the saves pick one of them.

- `sandbox.ogs`: introductory RCI ruleset with homes, workplaces, shops, pollution, desirability, and zone growth. **Start here.** Four cities are built on it:
    - `sandbox.ogc`: the small hand-drawn town the ruleset was written for.
    - `qq.ogc` and `qq2.ogc`: two larger hand-drawn towns.
    - `regular.ogc`: CiudadSim-style `Regular(6,6)` grid, **bidirectional** segments.
    - `chicago.ogc`: simplified downtown arteries (not the 546-node Scilab `chisincen.net`). Two orders of magnitude larger than the others, which is what the router is measured on.
- `braess.ogs` + `braess.ogc`: four-node Braess paradox. A ruleset of its own, since the point of it is three segment types whose speed and capacity are chosen so that the shortcut makes everybody slower.

## Worked example: `sandbox`

`sandbox.ogs` is the best starting point for understanding a complete ruleset. It combines environmental layers, homes, workplaces, shops, restaurants, several agent types, a road network, and Residential, Commercial, and Industrial zones.

The clock and `hour between` were not in the C# port; they drive the day:

- **08:00**: houses send workers out. A new simulation opens at that hour rather than at midnight (`SimulationConfig::startHour`; the clock panel sets it). A household holds eight and lets one leave for work every three quarters of an hour, so the commute never empties it.
- **Factories** turn people into goods, pollute their neighbourhood, and pay the treasury.
- **Goods** reach the shops on a `Truck`, since workers carry people, not stock. Without that freight rule the shops had nothing to sell.
- **10:00–20:00**: residents left at home go shopping. A shop only sells to customers standing in it (`local People greater 0` in `SellGoods`), so money follows traffic instead of appearing on its own.
- **12:00–14:00**: workers eat: a `Diner` heads for the nearest `Restaurant`, and `SendDinersBack` returns them before 15:00. A restaurant answers to `Restaurant` only and is supplied by its own freight rule, `ShipFood`.
- **18:00–22:00**: everybody goes home.

A building that finds nowhere to deliver wanders for two game hours (`SimulationConfig::agentGiveUpTicks`) and then hands its load back to the building that sent it out. Agents searching without ever finding usually mean the city is short of a building type: a single factory holds twelve workers, so a district of houses needs an Industrial zone, or more factories, before everybody has somewhere to go.

Desirability and zone growth are counted in hours and days: a district that is built and abandoned within one afternoon reads as a bug, not as a city.
