// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <quick-lint-js/array.h>
#include <quick-lint-js/cli/cli-location.h>
#include <quick-lint-js/container/concat.h>
#include <quick-lint-js/container/padded-string.h>
#include <quick-lint-js/diag-collector.h>
#include <quick-lint-js/diag-matcher.h>
#include <quick-lint-js/diag/diagnostic-types.h>
#include <quick-lint-js/fe/language.h>
#include <quick-lint-js/fe/parse.h>
#include <quick-lint-js/parse-support.h>
#include <quick-lint-js/port/char8.h>
#include <quick-lint-js/spy-visitor.h>
#include <string>
#include <string_view>
#include <vector>

using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAreArray;

namespace quick_lint_js {
namespace {
class test_parse_typescript_declare_namespace : public test_parse_expression {};

TEST_F(test_parse_typescript_declare_namespace,
       declare_namespace_is_not_allowed_in_javascript) {
  test_parser p(u8"declare namespace ns {}"_sv, javascript_options,
                capture_diags);
  p.parse_and_visit_module();
  EXPECT_THAT(p.visits, ElementsAreArray({
                            "visit_variable_declaration",   // ns
                            "visit_enter_namespace_scope",  // {
                            "visit_exit_namespace_scope",   // }
                            "visit_end_of_module",
                        }));
  EXPECT_THAT(
      p.errors,
      ElementsAreArray({
          DIAG_TYPE_OFFSETS(
              p.code,
              diag_typescript_namespaces_not_allowed_in_javascript,  //
              namespace_keyword, strlen(u8"declare "), u8"namespace"_sv),
      }));
}

TEST_F(test_parse_typescript_declare_namespace, declare_empty_namespace) {
  test_parser p(u8"declare namespace ns {}"_sv, typescript_options);
  p.parse_and_visit_module();
  EXPECT_THAT(p.visits, ElementsAreArray({
                            "visit_variable_declaration",   // ns
                            "visit_enter_namespace_scope",  // {
                            "visit_exit_namespace_scope",   // }
                            "visit_end_of_module",
                        }));
  EXPECT_THAT(p.variable_declarations,
              ElementsAreArray({namespace_decl(u8"ns"_sv)}));
}

TEST_F(test_parse_typescript_declare_namespace, missing_body) {
  {
    test_parser p(u8"declare namespace ns "_sv, typescript_options,
                  capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",  // ns
                              "visit_end_of_module",         //
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE_OFFSETS(p.code,
                              diag_missing_body_for_typescript_namespace,  //
                              expected_body, strlen(u8"declare namespace ns"),
                              u8""_sv),
        }));
  }

  {
    test_parser p(u8"declare namespace ns\nconsole.log('hello');"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",  // ns
                              "visit_variable_use",          // console
                              "visit_end_of_module",         //
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE_OFFSETS(p.code,
                              diag_missing_body_for_typescript_namespace,  //
                              expected_body, strlen(u8"declare namespace ns"),
                              u8""_sv),
        }));
  }
}

TEST_F(test_parse_typescript_declare_namespace, incomplete_body) {
  {
    test_parser p(u8"declare namespace ns { "_sv, typescript_options,
                  capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_exit_namespace_scope",   // }
                              "visit_end_of_module",          //
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            // TODO(strager): Report a namespace-specific diagnostic.
            DIAG_TYPE_OFFSETS(p.code,
                              diag_unclosed_code_block,  //
                              block_open, strlen(u8"declare namespace ns "),
                              u8"{"_sv),
        }));
  }
}

TEST_F(test_parse_typescript_declare_namespace,
       newline_is_not_allowed_after_namespace_keyword) {
  test_parser p(u8"declare namespace\nns {}"_sv, typescript_options,
                capture_diags);
  p.parse_and_visit_module();
  EXPECT_THAT(p.visits, ElementsAreArray({
                            "visit_variable_declaration",   // ns
                            "visit_enter_namespace_scope",  // {
                            "visit_exit_namespace_scope",   // }
                            "visit_end_of_module",
                        }));
  EXPECT_THAT(
      p.errors,
      ElementsAreArray({
          DIAG_TYPE_OFFSETS(
              p.code,
              diag_newline_not_allowed_after_namespace_keyword,  //
              namespace_keyword, strlen(u8"declare "), u8"namespace"_sv),
      }));
}

TEST_F(test_parse_typescript_declare_namespace,
       declares_are_not_allowed_inside_declare_namespace) {
  {
    test_parser p(u8"declare namespace ns { declare enum E { } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // E
                              "visit_enter_enum_scope",       // {
                              "visit_exit_enum_scope",        // }
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE_2_OFFSETS(
                p.code,
                diag_declare_keyword_is_not_allowed_inside_declare_namespace,  //
                declare_keyword, strlen(u8"declare namespace ns { "),
                u8"declare"_sv,  //
                declare_namespace_declare_keyword, 0, u8"declare"_sv),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare const enum E { } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // E
                              "visit_enter_enum_scope",       // {
                              "visit_exit_enum_scope",        // }
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare const myVariable; }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // myVariable
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare let myVariable; }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // myVariable
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare var myVariable; }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // myVariable
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(
        u8"declare namespace ns { declare class C { myMethod(); } }"_sv,
        typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",    // ns
                              "visit_enter_namespace_scope",   // {
                              "visit_enter_class_scope",       // C
                              "visit_enter_class_scope_body",  // {
                              "visit_property_declaration",    // myMethod
                              "visit_enter_function_scope",    // myMethod
                              "visit_exit_function_scope",     // myMethod
                              "visit_exit_class_scope",        // }
                              "visit_variable_declaration",    // C
                              "visit_exit_namespace_scope",    // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare abstract class C { } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",    // ns
                              "visit_enter_namespace_scope",   // {
                              "visit_enter_class_scope",       // C
                              "visit_enter_class_scope_body",  // {
                              "visit_exit_class_scope",        // }
                              "visit_variable_declaration",    // C
                              "visit_exit_namespace_scope",    // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare interface I { } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // I
                              "visit_enter_interface_scope",  // {
                              "visit_exit_interface_scope",   // }
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare type T = U; }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",    // ns
                              "visit_enter_namespace_scope",   // {
                              "visit_variable_declaration",    // T
                              "visit_enter_type_alias_scope",  //
                              "visit_variable_type_use",       // U
                              "visit_exit_type_alias_scope",   //
                              "visit_exit_namespace_scope",    // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns { declare function f(); }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // f
                              "visit_enter_function_scope",   //
                              "visit_exit_function_scope",    //
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }

  {
    test_parser p(u8"declare namespace ns1 { declare namespace ns2 { } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_statement();
    EXPECT_THAT(p.visits, ElementsAreArray({
                              "visit_variable_declaration",   // ns1
                              "visit_enter_namespace_scope",  // {
                              "visit_variable_declaration",   // ns2
                              "visit_enter_namespace_scope",  // {
                              "visit_exit_namespace_scope",   // }
                              "visit_exit_namespace_scope",   // }
                          }));
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE(
                diag_declare_keyword_is_not_allowed_inside_declare_namespace),
        }));
  }
}

TEST_F(test_parse_typescript_declare_namespace,
       enum_inside_declare_namespace_acts_like_declare_enum) {
  {
    test_parser p(u8"declare namespace ns { enum E { A = f() } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE_FIELD(diag_typescript_enum_value_must_be_constant,
                            declared_enum_kind, enum_kind::declare_enum),
        }))
        << "diag_typescript_enum_value_must_be_constant is not reported for "
           "normal enums but is reported for declare enums";
  }

  {
    test_parser p(u8"declare namespace ns { const enum E { A = f() } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(
        p.errors,
        ElementsAreArray({
            DIAG_TYPE_FIELD(diag_typescript_enum_value_must_be_constant,
                            declared_enum_kind, enum_kind::declare_const_enum),
        }));
  }
}

TEST_F(test_parse_typescript_declare_namespace,
       var_inside_declare_namespace_acts_like_declare_var) {
  {
    // diag_missing_initializer_in_const_declaration is not reported for declare
    // consts.
    test_parser p(u8"declare namespace ns { const myVariable; }"_sv,
                  typescript_options);
    p.parse_and_visit_module();
  }

  {
    test_parser p(u8"declare namespace ns { let myVariable = null; }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.errors,
                ElementsAreArray({
                    DIAG_TYPE(diag_declare_var_cannot_have_initializer),
                }));
  }

  {
    test_parser p(u8"declare namespace ns { var myVariable = null; }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.errors,
                ElementsAreArray({
                    DIAG_TYPE(diag_declare_var_cannot_have_initializer),
                }));
  }
}

TEST_F(test_parse_typescript_declare_namespace,
       function_inside_declare_namespace_acts_like_declare_function) {
  {
    // diag_declare_function_cannot_have_body or diag_missing_function_body is
    // not reported for declare functions.
    test_parser p(u8"declare namespace ns { function f(); }"_sv,
                  typescript_options);
    p.parse_and_visit_module();
  }

  {
    test_parser p(u8"declare namespace ns { async function f(); }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.errors,
                ElementsAreArray({
                    // TODO(strager): Also link to the 'declare' keyword.
                    DIAG_TYPE_OFFSETS(p.code,
                                      diag_declare_function_cannot_be_async,  //
                                      async_keyword,
                                      strlen(u8"declare namespace ns { "),
                                      u8"async"_sv),
                }));
  }
}

TEST_F(test_parse_typescript_declare_namespace,
       class_inside_declare_namespace_acts_like_declare_class) {
  {
    // diag_missing_function_body is not reported in declare classes.
    test_parser p(u8"declare namespace ns { class C { myMethod(); } }"_sv,
                  typescript_options);
    p.parse_and_visit_module();
  }

  {
    // diag_missing_function_body is not reported in declare classes.
    test_parser p(
        u8"declare namespace ns { abstract class C { myMethod(); } }"_sv,
        typescript_options);
    p.parse_and_visit_module();
  }
}

TEST_F(test_parse_typescript_declare_namespace,
       namespace_inside_declare_namespace_acts_like_declare_namespace) {
  {
    test_parser p(
        u8"declare namespace ns1 { namespace ns2 { if (true) {} } }"_sv,
        typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.errors,
                ElementsAreArray({
                    DIAG_TYPE_2_OFFSETS(
                        p.code,
                        diag_declare_namespace_cannot_contain_statement,  //
                        first_statement_token,
                        strlen(u8"declare namespace ns1 { namespace ns2 { "),
                        u8"if"_sv,  //
                        declare_keyword, 0, u8"declare"_sv),
                }));
  }
}

TEST_F(test_parse_typescript_declare_namespace,
       declare_namespace_disallows_most_statements) {
  {
    test_parser p(u8"declare namespace ns { if (true) { } }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.errors,
                ElementsAreArray({
                    DIAG_TYPE_OFFSETS(
                        p.code,
                        diag_declare_namespace_cannot_contain_statement,  //
                        first_statement_token,
                        strlen(u8"declare namespace ns { "), u8"if"_sv),
                }));
  }

  {
    test_parser p(u8"declare namespace ns { console.log('hello'); }"_sv,
                  typescript_options, capture_diags);
    p.parse_and_visit_module();
    EXPECT_THAT(p.errors,
                ElementsAreArray({
                    DIAG_TYPE_OFFSETS(
                        p.code,
                        diag_declare_namespace_cannot_contain_statement,  //
                        first_statement_token,
                        strlen(u8"declare namespace ns { "), u8"console"_sv),
                }));
  }
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
