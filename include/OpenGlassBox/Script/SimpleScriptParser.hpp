//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file SimpleScriptParser.hpp
//! \brief Parser of the historical keyword-based OpenGlassBox script language.

#ifndef OPEN_GLASSBOX_SCRIPT_SIMPLE_SCRIPT_PARSER_HPP
#define OPEN_GLASSBOX_SCRIPT_SIMPLE_SCRIPT_PARSER_HPP

#include "OpenGlassBox/Script/IScriptParser.hpp"
#include "OpenGlassBox/Script/Lexer.hpp"

namespace ogb
{

//==============================================================================
//! \brief Parser of the original keyword based language of OpenGlassBox, the
//! one every \c .ogs file is written in.
//!
//! The language is a handful of sections, each closed by \c end, and inside
//! them one declaration per line. Whitespace does not matter, apart from one
//! place noted on parseRate(). A \c # runs to the end of the line.
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
//!     unitRule SendPeopleToWork
//!         rate 45 minutes
//!         hour between 8 18
//!         local People greater 0
//!         local People remove 1
//!         agent Worker to Work add [ People 1 ]
//!     end
//! end
//!
//! units                           # kinds of building
//!     unit Home color 0xFF00FF layerRadius 1 rules [ SendPeopleToWork ]
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
//! The stream of words is walked twice. The first pass only declares the names,
//! the second fills them in. Two things follow from that: the order of the
//! sections stops mattering, so a building may list a rule written further
//! down, and a name resolving to nothing becomes an error with a line number
//! rather than a null pointer quietly pushed into a list of rules.
//!
//! Nothing throws on a bad word. An error is recorded and the parser
//! resynchronises on the end of the construct it was reading, so that one typo
//! does not hide the twenty after it. Past MAX_ERRORS it gives up, the rest
//! being consequences of the first few.
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

    //! \brief What the current walk of the stream of words is for.
    enum class Pass
    {
        //! \brief Declare the names and nothing else, so that the second pass
        //! can resolve a reference to anything, wherever it was written.
        Declare,

        //! \brief Fill those names in, resolving every reference.
        Define,
    };

    //--------------------------------------------------------------------------
    //! \brief Run both passes over the words already read.
    //! \param[out] definitions where to put what was understood.
    //! \return true when nothing was found wrong.
    //--------------------------------------------------------------------------
    bool run(ScriptDefinitions& definitions);

    //--------------------------------------------------------------------------
    //! \brief Walk the whole stream once, section by section.
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
    void parseUnits();
    void parseUnit();
    void parseZones();
    void parseZone();
    void parseRules();
    void parseRuleLayer();
    void parseRuleUnit();
    void parseRuleZone();
    IRuleCommand* parseCommand(Token const& token);

    void parseResourcesArray(Resources& resources);
    void parseCapacitiesArray(Resources& resources);
    void parseStringArray(std::vector<std::string>& out);
    void parseRuleLayerArray(std::vector<RuleLayer*>& rules);
    void parseRuleUnitArray(std::vector<RuleUnit*>& rules);
    void parseRuleZoneArray(std::vector<RuleZone*>& rules);

    //--------------------------------------------------------------------------
    //! \brief Take the next word, complaining when there is none.
    //!
    //! Every production reads through this, which is what makes a script cut
    //! short come to a stop instead of looping for ever on an empty word.
    //!
    //! \param[in] what to name in the error, such as "the name of a resource".
    //! \return the word, or an invalid one when the input ran out, in which
    //! case an error has been recorded.
    //--------------------------------------------------------------------------
    Token const& expectToken(char const* what);

    //--------------------------------------------------------------------------
    //! \brief Take the next word and check it is the one expected.
    //! \param[in] word the word that has to come next, such as "between".
    //! \return false when it was something else, having recorded an error.
    //--------------------------------------------------------------------------
    bool expectWord(char const* word);

    //--------------------------------------------------------------------------
    //! \brief Skip up to and past the next \c end, so that the parse can carry
    //! on after a construct it could not understand.
    //--------------------------------------------------------------------------
    void skipToEnd();

    //--------------------------------------------------------------------------
    //! \brief Skip up to the next section keyword, for when even the end of the
    //! construct cannot be trusted.
    //--------------------------------------------------------------------------
    void skipToNextSection();

    //--------------------------------------------------------------------------
    //! \brief \param[in] text a word.
    //! \return true when it opens a section, such as "units" or "rules".
    //--------------------------------------------------------------------------
    [[nodiscard]] static bool isSectionKeyword(std::string const& text);

    //--------------------------------------------------------------------------
    //! \brief Skip up to and past the next \c ], for when a bracketed list
    //! could not be understood.
    //--------------------------------------------------------------------------
    void skipToCloseBracket();

    // -------------------------------------------------------------------------
    // Checked conversions of a word already taken. A word that is not a number
    // becomes an error naming it, rather than the zero atoi used to hand back.
    // Colours are read as 0xRRGGBB, and a boolean as "true" or "false".
    // -------------------------------------------------------------------------
    uint32_t toUint(Token const& token);
    float toFloat(Token const& token);
    uint32_t toColor(Token const& token);
    bool toBool(Token const& token);

    // -------------------------------------------------------------------------
    // Take the next word and convert it. The parameter names what is expected,
    // such as "a percentage", and only appears in the error message.
    // -------------------------------------------------------------------------
    uint32_t nextUint(char const* what);
    float nextFloat(char const* what);
    uint32_t nextColor(char const* what);
    bool nextBool(char const* what);

    //--------------------------------------------------------------------------
    //! \brief Read the period of a rule, either as a number of ticks or as a
    //! duration of game time: "rate 7", "rate 30 minutes", "rate 2 hours",
    //! "rate 1 day". The unit word is optional and ticks are assumed without
    //! it, which is what every script written before this said. It must sit on
    //! the same line as the number, because "hour" is also a command.
    //! \param[out] rate: the number of ticks, meaningful only when rateMinutes
    //! comes back as zero.
    //! \param[out] rateMinutes: the duration in minutes of game time, zero when
    //! the script counted ticks.
    //--------------------------------------------------------------------------
    void parseRate(uint32_t& rate, uint32_t& rateMinutes);

    //--------------------------------------------------------------------------
    //! \brief Record something wrong, at the place a word was written.
    //! \param[in] token the offending word, whose line and column are used.
    //! \param[in] message what is wrong with it.
    //--------------------------------------------------------------------------
    void error(Token const& token, std::string const& message);

    //--------------------------------------------------------------------------
    //! \brief \return true once MAX_ERRORS have piled up and going on is
    //! pointless.
    //--------------------------------------------------------------------------
    bool tooManyErrors() const;

    //--------------------------------------------------------------------------
    //! \brief \return true during the second pass, which is the one that builds
    //! anything. Read by the productions, which declare on the first pass and
    //! resolve on the second.
    //--------------------------------------------------------------------------
    bool defining() const
    {
        return m_pass == Pass::Define;
    }

private:

    //! \brief Past this many errors the parse gives up: beyond a certain point
    //! the errors are consequences of the first ones and only bury them.
    static constexpr size_t MAX_ERRORS = 25u;

    //! \brief The words of the script, walked twice.
    Lexer m_lexer;

    //! \brief What the current walk is for.
    Pass m_pass = Pass::Declare;

    //! \brief Where what is understood goes. Not owned, and only valid for the
    //! duration of a parse.
    ScriptDefinitions* m_definitions = nullptr;

    //! \brief What was found wrong, in the order it was found.
    std::vector<ParseError> m_errors;
};

} // namespace ogb

#endif
