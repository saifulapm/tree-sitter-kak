/**
 * @file Kakscript grammar for tree-sitter
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: 'kakscript',

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

    argument: $ => $.word,

    word: $ => /[^\s;#]+/,

    comment: $ => seq('#', /.*/),

    _terminator: $ => choice(';', /\n/),
  },
});
