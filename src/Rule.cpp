#include "Rule.hpp"
#include <cstdlib>
#include <stdexcept>

template<class T>
static bool TryParse(std::string const& str, T& res)
{
    try
    {
        std::size_t sz;
        res = T(std::stoll(str, &sz, 10));
        return sz == str.length(); //TODO LOG
    }
    catch (std::invalid_argument const& /*ia*/)
    {
        //TODO LOG
        return false;
    }
}

bool Rule::execute(RuleContext& context)
{
    size_t i = m_commands.size();
    while (i--)
    {
        if (!m_commands[i]->validate(context))
            return false;
    }

    i = m_commands.size();
    while (i--)
    {
        m_commands[i]->execute(context);
    }

    return true;
}

void Rule::setOption(std::string const& optionId, std::string const& val)
{
    if (optionId == "rate")
    {
        if (!TryParse<uint32_t>(val, m_rate))
            m_rate = 1u;
    }
}
