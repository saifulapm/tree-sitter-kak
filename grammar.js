/**
 * @file Kakscript grammar for tree-sitter
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: 'kakscript',

  // Order MUST match enum TokenType in scanner.c exactly.
  externals: $ => [
    $._percent_string_start,
    $._percent_string_end,
    $._bare_string_content,
    $._nonbalanced_string_start,
    $._nonbalanced_string_end,
    $._string_content_double,
    $._expansion_percent,
    $._expansion_end,
    $._concat,
  ],

  // IMPORTANT: \n is NOT in extras — it's a terminator.
  // Only horizontal whitespace is auto-skipped.
  extras: $ => [
    /[ \t\r]/,
    $.comment,
  ],

  rules: {
    program: $ => seq(
      repeat($._terminator),
      optional($._statements),
    ),

    _statements: $ => seq(
      repeat(seq($._statement, $._terminator)),
      $._statement,
      optional($._terminator),
    ),

    _statement: $ => choice(
      $.command,
    ),

    command: $ => seq(
      field('name', $.command_name),
      repeat($.argument),
    ),

    command_name: $ => $.word,

    argument: $ => choice(
      $.single_quoted_string,
      $.percent_string,
      $.word,
    ),

    // --- Strings ---

    single_quoted_string: $ => seq(
      "'",
      optional($.string_content),
      "'",
    ),

    string_content: $ => repeat1(choice(
      /[^']+/,
      "''",
    )),

    percent_string: $ => choice(
      // Balanced: %{...} %[...] %<...> %(...)
      seq(
        $._percent_string_start,
        optional(field('content', alias($._bare_string_content, $.string_content))),
        $._percent_string_end,
      ),
      // Non-balanced: %|...| %/.../ etc.
      seq(
        $._nonbalanced_string_start,
        optional(field('content', alias($._bare_string_content, $.string_content))),
        $._nonbalanced_string_end,
      ),
    ),

    word: $ => /[^\s;#'%]+/,

    comment: $ => seq('#', /.*/),

    _terminator: $ => choice(';', /\n/),
  },
});
