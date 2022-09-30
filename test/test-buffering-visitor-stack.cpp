// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <csetjmp>
#include <gtest/gtest.h>
#include <quick-lint-js/fe/buffering-visitor-stack.h>
#include <quick-lint-js/port/unreachable.h>

namespace quick_lint_js {
namespace {
// These tests assume a leak checker will fail if a memory leak occurs.
// TODO(strager): Create a leak-checking memory resource.

TEST(test_buffering_visitor_stack, empty) {
  buffering_visitor_stack stack;
  // This test assumes a leak checker will fail if a memory leak occurs.
}

TEST(test_buffering_visitor_stack, push_one_pop_one) {
  buffering_visitor_stack stack;
  {
    stacked_buffering_visitor v = stack.push();
    // pop
  }
  // This test assumes a leak checker will fail if a memory leak occurs.
}

TEST(test_buffering_visitor_stack, push_two_pop_two) {
  buffering_visitor_stack stack;
  {
    stacked_buffering_visitor outer_v = stack.push();
    {
      stacked_buffering_visitor inner_v = stack.push();
      // pop
    }
    // pop
  }
  // This test assumes a leak checker will fail if a memory leak occurs.
}

TEST(test_buffering_visitor_stack, push_pop_push_pop) {
  buffering_visitor_stack stack;
  {
    stacked_buffering_visitor v = stack.push();
    // pop
  }
  {
    stacked_buffering_visitor v = stack.push();
    // pop
  }
  // This test assumes a leak checker will fail if a memory leak occurs.
}

TEST(test_buffering_visitor_stack, longjmp_around_pop) {
  buffering_visitor_stack stack;

  volatile bool pushed = false;
  std::jmp_buf buf;
  if (setjmp(buf) == 0) {
    stacked_buffering_visitor v = stack.push();
    pushed = true;
    std::longjmp(buf, 1);
    // pop (doesn't execute)
  }
  ASSERT_TRUE(pushed);
  // This test assumes a leak checker will fail if a memory leak occurs.
}
}
}

// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
