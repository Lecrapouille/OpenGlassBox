//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file OpenGlassBox.hpp
//! \brief Main entry point for the OpenGlassBox public API.
//!
//! Include this header to get the full API.
//! Or include single headers (e.g. \c OpenGlassBox/Simulation.hpp) for faster builds.
//!
//! Some headers are not included on purpose. The lexer, the script parser,
//! and cell iterators are internal tools. Include them directly if you extend the engine.

#ifndef OPEN_GLASSBOX_HPP
#define OPEN_GLASSBOX_HPP

// Core types and configuration
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Name.hpp"
#include "OpenGlassBox/SimulationClock.hpp"
#include "OpenGlassBox/Types.hpp"
#include "OpenGlassBox/Vector.hpp"

// Rules, resources and economy
#include "OpenGlassBox/OpeningHours.hpp"
#include "OpenGlassBox/Resource.hpp"
#include "OpenGlassBox/Resources.hpp"
#include "OpenGlassBox/Rule.hpp"
#include "OpenGlassBox/RuleCommand.hpp"
#include "OpenGlassBox/RuleValue.hpp"

// Layers of the environment, zones and placement
#include "OpenGlassBox/Zone.hpp"
#include "OpenGlassBox/Entity.hpp"
#include "OpenGlassBox/Layer.hpp"
#include "OpenGlassBox/CellRegion.hpp"

// Road network and routing
#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Path.hpp"
#include "OpenGlassBox/Router.hpp"

// Buildings and agents
#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Building.hpp"

// Rules read from a script
#include "OpenGlassBox/Ruleset.hpp"
#include "OpenGlassBox/Script/IScriptParser.hpp"
#include "OpenGlassBox/Script/ScriptDefinitions.hpp"

// The game itself
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Listener.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/World.hpp"

#endif
