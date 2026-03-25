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
    $._expansion_delim_start,
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
      $.try_statement,
      $.hook_definition,
      $.map_definition,
      $.unmap_definition,
      $.define_command,
      $.evaluate_commands,
      $.execute_keys,
      $.command,
    ),

    command: $ => seq(
      field('name', $.command_name),
      repeat(choice(
        $.switch,
        $.argument,
      )),
      optional(seq(
        $.switch_terminator,
        repeat($.argument),
      )),
    ),

    hook_definition: $ => prec(1, seq(
      'hook',
      repeat(alias($._keyword_switch, $.switch)),
      field('scope', $.scope),
      field('name', $.word),
      field('filter', $.argument),
      field('body', $._block),
    )),

    map_definition: $ => prec(1, seq(
      'map',
      repeat(alias($._keyword_switch, $.switch)),
      field('scope', $.scope),
      field('mode', $.mode),
      field('key', $.argument),
      field('keys', $.argument),
    )),

    unmap_definition: $ => prec(1, seq(
      'unmap',
      field('scope', $.scope),
      field('mode', $.mode),
      field('key', $.argument),
      optional(field('expected', $.argument)),
    )),

    define_command: $ => prec(1, seq(
      'define-command',
      repeat(alias($._keyword_switch, $.switch)),
      field('name', $.command_name),
      field('body', $._block),
    )),

    evaluate_commands: $ => prec(1, seq(
      'evaluate-commands',
      repeat(alias($._plain_switch, $.switch)),
      repeat1($.argument),
    )),

    execute_keys: $ => prec(1, seq(
      'execute-keys',
      repeat(alias($._plain_switch, $.switch)),
      repeat1($.argument),
    )),

    scope: $ => choice('global', 'buffer', 'window'),

    mode: $ => choice('insert', 'normal', 'prompt', 'user', 'goto', 'view', 'object'),

    try_statement: $ => prec(1, seq(
      'try',
      field('body', $._block),
      repeat(seq(
        'catch',
        field('handler', $._block),
      )),
    )),

    _block: $ => choice(
      $.percent_string,
      $.single_quoted_string,
      $.double_quoted_string,
      $.expansion,
    ),

    switch: $ => prec.right(seq(
      token(seq('-', /[a-zA-Z][a-zA-Z0-9_-]*/)),
      optional(choice(
        $.single_quoted_string,
        $.double_quoted_string,
        $.expansion,
        $.percent_string,
      )),
    )),

    _plain_switch: $ => token(seq('-', /[a-zA-Z][a-zA-Z0-9_-]*/)),

    _keyword_switch: $ => prec.right(seq(
      token(seq('-', /[a-zA-Z][a-zA-Z0-9_-]*/)),
      optional(choice(
        $.single_quoted_string,
        $.double_quoted_string,
        $.expansion,
        $.percent_string,
        $.word,
      )),
    )),

    switch_terminator: $ => '--',

    command_name: $ => $.word,

    argument: $ => choice(
      $.expansion,
      $.single_quoted_string,
      $.percent_string,
      $.double_quoted_string,
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

    double_quoted_string: $ => seq(
      '"',
      repeat(choice(
        alias($._string_content_double, $.string_content),
        $.expansion,
      )),
      '"',
    ),

    expansion: $ => seq(
      $._expansion_percent,
      field('type', $.expansion_type),
      choice(
        seq(
          $._expansion_delim_start,
          optional(field('content', alias($._bare_string_content, $.string_content))),
          choice($._percent_string_end, $._nonbalanced_string_end),
        ),
      ),
    ),

    expansion_type: $ => token.immediate(choice('val', 'opt', 'sh', 'reg', 'arg', 'file', 'exp')),

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

    word: $ => /[^\s;#'%"]+/,

    comment: $ => seq('#', /.*/),

    _terminator: $ => choice(';', /\n/),
  },
});
