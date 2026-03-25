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
    /\\\n/,
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
      $.set_option,
      $.declare_option,
      $.unset_option,
      $.provide_module,
      $.require_module,
      $.source_command,
      $.set_face,
      $.alias_definition,
      $.unalias_definition,
      $.set_register,
      $.echo_command,
      $.fail_command,
      $.nop_command,
      $.remove_hooks,
      $.remove_highlighter,
      $.declare_user_mode,
      $.enter_user_mode,
      $.select_command,
      $.prompt_command,
      $.on_key_command,
      $.add_highlighter,
      $.command,  // generic fallback — MUST be last
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
      repeat(alias($._keyword_switch, $.switch)),
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
      repeat(alias($._define_command_switch, $.switch)),
      field('name', alias($._user_command_name, $.command_name)),
      repeat(alias($._keyword_switch, $.switch)),
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

    set_option: $ => prec(1, seq(
      'set-option',
      repeat(alias($._keyword_switch, $.switch)),
      field('scope', $.scope),
      field('name', $.word),
      repeat1($.argument),
    )),

    declare_option: $ => prec(1, seq(
      'declare-option',
      repeat(alias($._keyword_switch, $.switch)),
      field('type', $.option_type),
      field('name', $.word),
      repeat($.argument),
    )),

    unset_option: $ => prec(1, seq(
      'unset-option',
      field('scope', $.scope),
      field('name', $.word),
    )),

    option_type: $ => choice(
      'int', 'bool', 'str', 'regex', 'coord',
      'int-list', 'str-list', 'range-specs', 'line-specs',
      'completions', 'str-to-str-map',
    ),

    provide_module: $ => prec(1, seq(
      'provide-module',
      repeat(alias($._flag_switch, $.switch)),
      field('name', $.word),
      field('body', $._block),
    )),

    require_module: $ => prec(1, seq(
      'require-module',
      field('name', $.word),
    )),

    source_command: $ => prec(1, seq(
      'source',
      field('filename', $.argument),
      repeat($.argument),
    )),

    set_face: $ => prec(1, seq(
      'set-face',
      field('scope', $.scope),
      field('name', $.word),
      field('spec', $.argument),
    )),

    alias_definition: $ => prec(1, seq(
      'alias',
      field('scope', $.scope),
      field('name', $.word),
      field('command', $.word),
    )),

    unalias_definition: $ => prec(1, seq(
      'unalias',
      field('scope', $.scope),
      field('name', $.word),
      optional(field('expected', $.word)),
    )),

    set_register: $ => prec(1, seq(
      'set-register',
      field('name', $.word),
      repeat1($.argument),
    )),

    echo_command: $ => prec(1, seq(
      'echo',
      repeat(alias($._plain_switch, $.switch)),
      repeat($.argument),
    )),

    fail_command: $ => prec(1, seq(
      'fail',
      repeat($.argument),
    )),

    nop_command: $ => prec(1, seq(
      'nop',
      repeat($.argument),
    )),

    remove_hooks: $ => prec(1, seq(
      'remove-hooks',
      field('scope', $.scope),
      field('group', $.word),
    )),

    remove_highlighter: $ => prec(1, seq(
      'remove-highlighter',
      field('path', $.word),
    )),

    declare_user_mode: $ => prec(1, seq(
      'declare-user-mode',
      field('name', $.word),
    )),

    enter_user_mode: $ => prec(1, seq(
      'enter-user-mode',
      repeat(alias($._flag_switch, $.switch)),
      field('name', $.word),
    )),

    select_command: $ => prec(1, seq(
      'select',
      repeat(alias($._keyword_switch, $.switch)),
      repeat1($.argument),
    )),

    prompt_command: $ => prec(1, seq(
      'prompt',
      repeat(alias($._keyword_switch, $.switch)),
      field('text', $.argument),
      field('body', $._block),
    )),

    on_key_command: $ => prec(1, seq(
      'on-key',
      field('body', $._block),
    )),

    add_highlighter: $ => prec(1, seq(
      'add-highlighter',
      repeat(alias($._keyword_switch, $.switch)),
      field('path', $.word),
      field('type', $.word),
      repeat($.argument),
    )),

    scope: $ => choice('global', 'buffer', 'window', 'current', 'local'),

    mode: $ => choice('insert', 'normal', 'prompt', 'user', 'goto', 'view', 'object', $.word),

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
      $.word,  // bare command name as body (e.g., hook ... .* file-detection)
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

    // Like _keyword_switch but does not greedily consume a bare word as value.
    // Used before word-typed fields (e.g. provide-module name, enter-user-mode name)
    // to avoid the switch eating the field value.
    _flag_switch: $ => prec.right(seq(
      token(seq('-', /[a-zA-Z][a-zA-Z0-9_-]*/)),
      optional(choice(
        $.single_quoted_string,
        $.double_quoted_string,
        $.expansion,
        $.percent_string,
      )),
    )),


    // Like _flag_switch but also accepts non-name word values (digits, dots).
    // Used in define-command before the name to handle -params ..1, -params 1, etc.
    // without consuming the command name (which starts with alpha/underscore).
    _define_command_switch: $ => prec.right(seq(
      token(seq('-', /[a-zA-Z][a-zA-Z0-9_-]*/)),
      optional(choice(
        $.single_quoted_string,
        $.double_quoted_string,
        $.expansion,
        $.percent_string,
        alias($._non_name_word, $.word),
      )),
    )),

    // Matches word-like tokens that cannot be a command name.
    // Starts with non-alpha, non-underscore (e.g., digits, dots: 1, ..1, 0..2)
    _non_name_word: $ => /[^a-zA-Z_\s;#'%"\\][^\s;#'%"\\]*/,

    switch_terminator: $ => '--',

    command_name: $ => $.word,

    // Command name that cannot start with '-', used in define-command
    // to distinguish the name from switches.
    _user_command_name: $ => /[a-zA-Z_][a-zA-Z0-9_-]*/,

    argument: $ => choice(
      $.concatenation,
      $.expansion,
      $.single_quoted_string,
      $.percent_string,
      $.double_quoted_string,
      $.word,
    ),

    // Adjacent tokens without whitespace: "prefix"%val{name}"suffix"
    concatenation: $ => prec(-1, seq(
      $._concat_part,
      repeat1(seq(
        $._concat,
        $._concat_part,
      )),
    )),

    _concat_part: $ => choice(
      $.expansion,
      $.single_quoted_string,
      $.double_quoted_string,
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

    word: $ => token(prec.right(repeat1(choice(
      /[^\s;#'%"\\]+/,
      /\\[^\n]/,  // escaped char (but NOT \n which is line continuation)
    )))),

    comment: $ => seq('#', /.*/),

    _terminator: $ => prec.right(repeat1(choice(';', /\n/))),
  },
});
