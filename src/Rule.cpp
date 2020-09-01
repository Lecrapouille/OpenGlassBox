#include "Rule.hpp"
#include <cstdlib>

template<class T>
static bool TryParse(std::string const& str, T& res)
{
    try
    {
        std::size_t sz;
        res = T(std::stoll(str, &sz, 10));
        return sz == word.length();
    }
    catch (const std::invalid_argument& /*ia*/)
    {
        return false;
    }
}

virtual bool Rule::execute(Rule::Context& context)
{
    for (size_t i = 0u; i < m_commands.size(); ++i)
    {
        if (!m_commands[i].validate(context))
            return false;
    }

    for (size_t i = 0u; i < m_commands.size(); ++i)
        m_commands[i].execute(context);

    return true;
}

virtual void Rule::setOption(std::string const& optionId, std::string const& val)
{
    switch (optionId)
    {
    case "rate":
        if (!TryParse<uint32_t>(val, m_rate))
            m_rate = 1u;
        break;
    }
}
