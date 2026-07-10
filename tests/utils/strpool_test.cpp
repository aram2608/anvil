#include "strpool/strpool.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace Anvil;

TEST(StrPool, InternStrings) {
  //
  StrPool p{};
  std::string hello{"Hello"};
  std::string world{"world"};

  EXPECT_EQ(p.InternOrGet(hello), p.InternOrGet(hello));
  EXPECT_EQ(p.InternOrGet(world), p.InternOrGet(world));
  EXPECT_NE(p.InternOrGet(hello), p.InternOrGet(world));
}
