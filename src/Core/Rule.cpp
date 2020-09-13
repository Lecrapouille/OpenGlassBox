#include "Core/Rule.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

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

IRule::IRule(std::string const& name)
    : m_id(name)
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

void IRule::setOption(std::string const& option, std::string const& value)
{
    if (option == "rate")
    {
        if (!tryConvertion<uint32_t>(value, m_rate))
            m_rate = 1u;
    }
}
