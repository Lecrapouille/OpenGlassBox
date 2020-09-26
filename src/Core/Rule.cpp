#include "Core/Rule.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

#if 0
template<class T>
static bool tryConvertion(std::string const& str, T& res)
{
    try
    {
        std::size_t sz;
        res = T(std::stoll(str, &sz, 10));
        return sz == str.length(); //TODO LOG
    }
    catch (std::invalid_argument const& /*ia*/)
    {
        std::cerr << "Failed converting '" << str
                  << "' field '" << res
                  << "' into integer!"
                  << std::endl;
        return false;
    }
}
#endif

IRule::IRule(std::string const& name, uint32_t rate, std::vector<IRuleCommand*> const& commands)
    : m_id(name), m_rate(rate), m_commands(commands)
{}

bool IRule::execute(RuleContext& context)
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

#if 0
void IRule::setOption(std::string const& option, std::string const& value)
{
    if (option == "rate")
    {
        if (!tryConvertion<uint32_t>(value, m_rate))
            m_rate = 1u;
    }
}
#endif
