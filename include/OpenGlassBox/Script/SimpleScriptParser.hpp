//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file SimpleScriptParser.hpp
//! \brief Parser for the keyword-based OpenGlassBox script language.

#ifndef OPEN_GLASSBOX_SCRIPT_SIMPLE_SCRIPT_PARSER_HPP
#define OPEN_GLASSBOX_SCRIPT_SIMPLE_SCRIPT_PARSER_HPP

#include "OpenGlassBox/Script/IScriptParser.hpp"
#include "OpenGlassBox/Script/Lexer.hpp"

namespace ogb
{

//==============================================================================
//! \brief Parser for the original keyword language of OpenGlassBox. Every
//! \c .ogs file uses it.
//!
//! The language has sections closed by \c end. Each section has one
//! declaration per line. Whitespace does not matter, except where noted on
//! parseRate(). A \c # starts a comment to the end of the line.
//!
//! \code
//! resources                       # what everything else is counted in
//!     resource People
//! end
//!
//! paths                           # kinds of network
//!     path Road color 0xAAAAAA
//! end
//!
//! segments                        # kinds of street, with their traffic model
//!     segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
//! end
//!
//! agents                          # kinds of traveller
//!     agent Worker color 0xFFFFFF speed 10
//! end
//!
//! rules                           # the behaviour of everything
//!     buildingRule SendPeopleToWork
//!         rate 45 minutes
//!         hour between 8 18
//!         local People greater 0
//!         local People remove 1
//!         agent Worker to Work add [ People 1 ]
//!     end
//! end
//!
//! buildings                       # kinds of building
//!     building Home color 0xFF00FF layerRadius 1 rules [ SendPeopleToWork ]
//!          targets [ Home ] caps [ People 8 ] resources [ People 8 ]
//! end
//!
//! layers                            # layers of the environment
//!     layer Water color 0x0000FF capacity 100 rules [ ]
//! end
//!
//! zones                           # kinds of zone the player paints
//!     zone Residential color 0x44AA44 rules [ GrowHomes ]
//! end
//! \endcode
//!
//! The token stream is read twice. The first pass declares names. The second
//! pass fills them in. Section order does not matter. A building can reference
//! a rule defined later. An unknown name becomes an error with a line number,
//! not a null pointer in a rule list.
//!
//! A bad word does not throw. The parser records an error and skips to the
//! \c end of the current construct, so one typo does not hide later errors.
//! After MAX_ERRORS the parser stops; later errors are usually side effects.
//==============================================================================
class SimpleScriptParser: public IScriptParser
{
public:

    //! \copydoc IScriptParser::parseFile
    bool parseFile(std::string const& filename,
                   ScriptDefinitions& definitions) override;

    //! \copydoc IScriptParser::parseString
    bool parseString(std::string const& source,
                     std::string const& name,
                     ScriptDefinitions& definitions) override;

    //! \copydoc IScriptParser::errors
    std::vector<ParseError> const& errors() const override
    {
        return m_errors;
    }

private:

    //! \brief Purpose of the current pass over the token stream.
    enum class Pass
    {
        //! \brief Declare names only, so the second pass can resolve any
        //! reference.
        Declare,

        //! \brief Fill in definitions and resolve every reference.
        Define,
    };

    //--------------------------------------------------------------------------
    //! \brief Run both passes over the tokens already read.
    //! \param[out] definitions where to store parsed results.
    //! \return true when no errors were found.
    //--------------------------------------------------------------------------
    bool run(ScriptDefinitions& definitions);

    //--------------------------------------------------------------------------
    //! \brief Walk the token stream once, section by section.
    //! \param[in] pass what this walk is for.
    //--------------------------------------------------------------------------
    void runPass(Pass pass);

    // -------------------------------------------------------------------------
    // Productions. Each one leaves the stream just past the construct it read,
    // whether it succeeded or not.
    // -------------------------------------------------------------------------
    void parseResources();
    void parseResource();
    void parsePaths();
    void parsePath();
    void parseSegments();
    void parseSegment();
    void parseAgents();
    void parseAgent();
    void parseLayers();
    void parseLayer();
    void parseBuildings();
    void parseBuilding();
    void parseZones();
    void parseZone();
    void parseRules();
    void parseRuleLayer();
    void parseRuleBuilding();
    void parseRuleZone();
    IRuleCommand* parseCommand(Token const& token);

    void parseResourcesArray(Resources& resources);
    void parseCapacitiesArray(Resources& resources);
    void parseStringArray(std::vector<std::string>& out);
    void parseRuleLayerArray(std::vector<RuleLayer*>& rules);
    void parseRuleBuildingArray(std::vector<RuleBuilding*>& rules);
    void parseRuleZoneArray(std::vector<RuleZone*>& rules);

    //--------------------------------------------------------------------------
    //! \brief Take the next word and report an error when the input is empty.
    //!
    //! Every production reads through this, so a truncated script stops instead
    //! of looping on empty tokens.
    //!
    //! \param[in] what name for the error, such as "the name of a resource".
    //! \return the word, or an invalid token when the input ran out.
    //--------------------------------------------------------------------------
    Token const& expectToken(char const* what);

    //--------------------------------------------------------------------------
    //! \brief Take the next word and check it matches the expected word.
    //! \param[in] word the expected word, such as "between".
    //! \return false when it differs; an error was recorded.
    //--------------------------------------------------------------------------
    bool expectWord(char const* word);

    //--------------------------------------------------------------------------
    //! \brief Skip to and past the next \c end after a failed construct parse.
    //--------------------------------------------------------------------------
    void skipToEnd();

    //--------------------------------------------------------------------------
    //! \brief Skip to the next section keyword when even \c end cannot be
    //! trusted.
    //--------------------------------------------------------------------------
    void skipToNextSection();

    //--------------------------------------------------------------------------
    //! \param[in] text a word.
    //! \return true when it opens a section, such as "buildings" or "rules".
    //--------------------------------------------------------------------------
    [[nodiscard]] static bool isSectionKeyword(std::string const& text);

    //--------------------------------------------------------------------------
    //! \brief Skip to and past the next \c ] after a failed bracketed list.
    //--------------------------------------------------------------------------
    void skipToCloseBracket();

    // -------------------------------------------------------------------------
    // Convert a token already read. A non-number becomes an error, not zero
    // from atoi. Colours use 0xRRGGBB. Booleans use "true" or "false".
    // -------------------------------------------------------------------------
    uint32_t toUint(Token const& token);
    float toFloat(Token const& token);
    uint32_t toColor(Token const& token);
    bool toBool(Token const& token);

    // -------------------------------------------------------------------------
    // Take the next word and convert it. The parameter names what is expected,
    // such as "a percentage", and appears only in the error message.
    // -------------------------------------------------------------------------
    uint32_t nextUint(char const* what);
    float nextFloat(char const* what);
    uint32_t nextColor(char const* what);
    bool nextBool(char const* what);

    // -------------------------------------------------------------------------
    //! \brief Take the next word as a share from zero to one hundred.
    //! A value above one hundred is an error, and reads as one hundred so that
    //! the rest of the file still parses.
    //! \param[in] what names what is expected, for the error message.
    //! \return the share, from 0 to 100.
    // -------------------------------------------------------------------------
    uint32_t nextPercent(char const* what);

    //--------------------------------------------------------------------------
    //! \brief Read a rule period as ticks or game time: "rate 7", "rate 30
    //! minutes", "rate 2 hours", "rate 1 day". The unit word is optional; ticks
    //! are assumed without it. The unit must sit on the same line as the number,
    //! because "hour" is also a command.
    //! \param[out] rate tick count, used only when rateMinutes is zero.
    //! \param[out] rateMinutes duration in minutes of game time, zero when the
    //! script used ticks.
    //--------------------------------------------------------------------------
    void parseRate(uint32_t& rate, uint32_t& rateMinutes);

    //--------------------------------------------------------------------------
    //! \brief Record an error at the token position.
    //! \param[in] token the bad word, for line and column.
    //! \param[in] message what is wrong.
    //--------------------------------------------------------------------------
    void error(Token const& token, std::string const& message);

    //--------------------------------------------------------------------------
    //! \return true after MAX_ERRORS errors; further parsing is useless.
    //--------------------------------------------------------------------------
    bool tooManyErrors() const;

    //--------------------------------------------------------------------------
    //! \return true during the second pass, which builds definitions.
    //! Productions declare on the first pass and resolve on the second.
    //--------------------------------------------------------------------------
    bool defining() const
    {
        return m_pass == Pass::Define;
    }

private:

    //! \brief Which of the three kinds of rule the parser is reading.
    //! Each kind runs on a different thing, so each accepts other commands.
    enum class RuleKind
    {
        Layer,
        Building,
        Zone,
    };

    //! \brief Stop parsing after this many errors. Later errors usually repeat
    //! the first ones.
    static constexpr size_t MAX_ERRORS = 25u;

    //! \brief Tokens from the script, read twice.
    Lexer m_lexer;

    //! \brief Current pass purpose.
    Pass m_pass = Pass::Declare;

    //! \brief Kind of the rule being read. Set by parseRuleLayer(),
    //! parseRuleBuilding() and parseRuleZone(), read by parseCommand().
    RuleKind m_ruleKind = RuleKind::Building;

    //! \brief Output catalogue. Not owned; valid only during parse().
    ScriptDefinitions* m_definitions = nullptr;

    //! \brief Errors found, in order.
    std::vector<ParseError> m_errors;
};

} // namespace ogb

#endif
