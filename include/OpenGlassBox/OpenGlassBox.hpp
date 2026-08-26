//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file OpenGlassBox.hpp
//! \brief Single entry point for the OpenGlassBox public API.
//!
//! Include this header to pull in the whole engine surface, or include
//! individual headers (e.g. \c OpenGlassBox/Simulation.hpp) for faster builds.

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

// Map, areas and placement
#include "OpenGlassBox/Area.hpp"
#include "OpenGlassBox/Entity.hpp"
#include "OpenGlassBox/Map.hpp"
#include "OpenGlassBox/MapCoordinatesInsideRadius.hpp"
#include "OpenGlassBox/MapRandomCoordinates.hpp"
#include "OpenGlassBox/MapRegion.hpp"

// Road network and routing
#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Path.hpp"
#include "OpenGlassBox/Router.hpp"

// Buildings and agents
#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Unit.hpp"

// Script parsing
#include "OpenGlassBox/Script/IScriptParser.hpp"
#include "OpenGlassBox/Script/Lexer.hpp"
#include "OpenGlassBox/Script/ScriptDefinitions.hpp"
#include "OpenGlassBox/Script/SimpleScriptParser.hpp"
#include "OpenGlassBox/ScriptParser.hpp"

// Simulation
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/World.hpp"

#endif
