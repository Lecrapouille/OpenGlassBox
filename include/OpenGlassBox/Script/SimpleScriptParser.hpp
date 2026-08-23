//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file SimpleScriptParser.hpp
//! \brief Parser of the historical keyword-based OpenGlassBox script language.


#ifndef OPEN_GLASSBOX_SCRIPT_SIMPLE_SCRIPT_PARSER_HPP
#  define OPEN_GLASSBOX_SCRIPT_SIMPLE_SCRIPT_PARSER_HPP

#  include "OpenGlassBox/Script/IScriptParser.hpp"
#  include "OpenGlassBox/Script/Lexer.hpp"

namespace ogb {

//==============================================================================
//! \brief Parser of the historical keyword based language of OpenGlassBox.
//!
//! The stream of tokens is walked twice. The first pass only declares the
//! names, the second one fills them in. Two things follow: the order of the
//! sections stops mattering, and a name that resolves to nothing is an error
//! with a position instead of a null pointer pushed into a vector of rules.
//!
//! No production throws on a bad token: an error is recorded and the parser
//! resynchronizes on the end of the enclosing construct, so that one typo does
//! not hide the twenty that follow it.
//==============================================================================
class SimpleScriptParser: public IScriptParser
{
public:

    bool parse(std::string const& filename,
               ScriptDefinitions& definitions) override;
    bool parseString(std::string const& source, std::string const& name,
                     ScriptDefinitions& definitions) override;
    std::vector<ParseError> const& errors() const override { return m_errors; }

private:

    //! \brief What the current walk of the token stream is for.
    enum class Pass
    {
        //! \brief Create the named objects and nothing else.
        Declare,
        //! \brief Fill them in, resolving every reference.
        Define,
    };

    //--------------------------------------------------------------------------
    //! \brief Run both passes over the token stream already loaded.
    //--------------------------------------------------------------------------
    bool run(ScriptDefinitions& definitions);
    void runPass(Pass pass);

    // -------------------------------------------------------------------------
    // Productions. Each one leaves the stream just past the construct it read,
    // whether it succeeded or not.
    // -------------------------------------------------------------------------
    void parseResources();
    void parseResource();
    void parsePaths();
    void parsePath();
    void parseWays();
    void parseWay();
    void parseAgents();
    void parseAgent();
    void parseMaps();
    void parseMap();
    void parseUnits();
    void parseUnit();
    void parseAreas();
    void parseArea();
    void parseRules();
    void parseRuleMap();
    void parseRuleUnit();
    void parseRuleArea();
    IRuleCommand* parseCommand(Token const& token);

    void parseResourcesArray(Resources& resources);
    void parseCapacitiesArray(Resources& resources);
    void parseStringArray(std::vector<std::string>& out);
    void parseRuleMapArray(std::vector<RuleMap*>& rules);
    void parseRuleUnitArray(std::vector<RuleUnit*>& rules);
    void parseRuleAreaArray(std::vector<RuleArea*>& rules);

    //--------------------------------------------------------------------------
    //! \brief Consume the next token, recording an error and returning an
    //! invalid token when the input ran out. Every production reads through
    //! this, which is what makes a truncated script terminate instead of
    //! looping on an empty token forever.
    //--------------------------------------------------------------------------
    Token const& expectToken(char const* what);

    //--------------------------------------------------------------------------
    //! \brief Consume the next token and check it is the expected word.
    //--------------------------------------------------------------------------
    bool expectWord(char const* word);

    //--------------------------------------------------------------------------
    //! \brief Skip tokens up to and including the next "end" at the current
    //! nesting, so that the parse can carry on after a bad construct.
    //--------------------------------------------------------------------------
    void skipToEnd();
    void skipToNextSection();
    static bool isSectionKeyword(std::string const& text);

    //--------------------------------------------------------------------------
    //! \brief Skip tokens up to and including the next "]".
    //--------------------------------------------------------------------------
    void skipToCloseBracket();

    // -------------------------------------------------------------------------
    // Checked conversions. A bad number is an error naming the offending word,
    // rather than the zero that atoi used to return.
    // -------------------------------------------------------------------------
    uint32_t toUint(Token const& token);
    float toFloat(Token const& token);
    uint32_t toColor(Token const& token);
    bool toBool(Token const& token);

    //--------------------------------------------------------------------------
    //! \brief Read the next token and convert it.
    //--------------------------------------------------------------------------
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
    //! \brief Record an error at the position of a token.
    //--------------------------------------------------------------------------
    void error(Token const& token, std::string const& message);

    //--------------------------------------------------------------------------
    //! \brief Whether so many errors piled up that going on is pointless.
    //--------------------------------------------------------------------------
    bool tooManyErrors() const;

    //! \brief Shorthand: nothing is built during the declaration pass.
    bool defining() const { return m_pass == Pass::Define; }

private:

    //! \brief Beyond this, the parse gives up: past a certain point the errors
    //! are consequences of the first ones and only bury it.
    static constexpr size_t MAX_ERRORS = 25u;

    Lexer m_lexer;
    Pass m_pass = Pass::Declare;
    ScriptDefinitions* m_definitions = nullptr;
    std::vector<ParseError> m_errors;
};

} // namespace ogb

#endif
