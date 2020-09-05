#include "main.hpp"

#define protected public
#define private public
#  include "src/Path.hpp"
#undef protected
#undef private
#  include "src/City.hpp"

TEST(TestsPath, NominalCase)
{
    City c("Paris", 32u, 32u);
    Path p("chemin", c);

    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(n1.id(), 0u);
    ASSERT_STREQ(n1.path().id().c_str(), "chemin");
    ASSERT_EQ(n2.id(), 1u);
    ASSERT_STREQ(n2.path().id().c_str(), "chemin");
}
