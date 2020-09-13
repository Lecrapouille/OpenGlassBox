#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/Rule.hpp"
#undef protected
#undef private

TEST(TestsRule, Constructor)
{
    Rule r("rule");

    ASSERT_STREQ(r.m_id.c_str(), "rule");
    ASSERT_EQ(r.m_commands.size(), 0u);
}
