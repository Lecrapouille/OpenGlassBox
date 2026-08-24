# Simulation files

OpenGlassBox uses two simulation file formats:

- **`.ogs`** is the ruleset (or mod): resources, rules, maps, paths, segments, agents, units, and areas.
- **`.ogc`** is a city save: its header identifies the ruleset and types in use, while the rest stores geometry and live state.

A separate world file is not needed. A save identifies the ruleset it was created with. Loading fails if a required type is missing or the ruleset hash differs. During ruleset development, File ==> **Open saves with a stale checksum** can bypass the hash check; required types must still exist. The Script panel displays the checksum and can re-stamp the open save.

## Bundled simulations

- `test_city.ogs` + `test_city.ogc` : introductory RCI sandbox with homes, workplaces, shops, pollution, desirability, and zone growth. Start here.
- `braess.ogs` + `braess.ogc` : four-node Braess paradox.
- `regular.ogs` + `regular.ogc` : CiudadSim-style `Regular(6,6)` grid, **bidirectional** ways.
- `chicago.ogs` + `chicago.ogc` : simplified downtown arteries (not the 546-node Scilab `chisincen.net`).

**New city** selects a `.ogs` ruleset and starts with an empty city. **Apply** in the Script panel reparses the current ruleset and keeps the geometry if every placed type remains defined.

## Worked example: `test_city`

`test_city.ogs` is the best starting point for understanding a complete ruleset. It combines environmental maps, homes, workplaces, shops, restaurants, several agent types, a road network, and Residential, Commercial, and Industrial zones.

The clock and `hour between` were not in the C# port; they drive the day:

- 08:00 the houses send workers out, and a new simulation opens at that hour rather than at midnight (`SimulationConfig::startHour`, and the clock panel of the demo sets it). A household holds eight and lets one leave for work every three quarters of an hour, so the commute never empties it: what shares the residents between the factories and the shops is the period of the rules against the size of the house, and a condition such as `local People greater 1` is not needed;
- the factories turn people into goods, pollute their neighbourhood and pay the treasury;
- goods reach the shops on a `Truck`, since the workers carry people, not stock. Without that freight rule the shops had nothing to sell;
- 10:00 to 20:00 the residents left at home go shopping, and a shop only sells to the customers standing in it (`local People greater 0` in `SellGoods`), so the money follows the traffic instead of appearing on its own;
- 12:00 to 14:00 the workers eat: a `Diner` heads for the nearest `Restaurant`, and `SendDinersBack` returns them before 15:00. A restaurant answers to `Restaurant` only and is supplied by its own freight rule, `ShipFood`. It used to answer to `Shop` as well, so a canteen standing closer than the shop caught every truck and every shopper, and the shops were never opened;
- 18:00 to 22:00 everybody goes home.

A building that finds nowhere to deliver wanders for two game hours (`SimulationConfig::agentGiveUpTicks`) and then hands its load back to the building that sent it out. Agents searching without ever finding usually mean the city is short of a building type: a single factory holds twelve workers, so a district of houses needs an Industrial zone, or more factories, before everybody has somewhere to go.

Desirability and zone growth are counted in hours and days: a district that is built and abandoned within one afternoon reads as a bug, not as a city.
