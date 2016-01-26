/*
 * cli.c
 *
 *  Created on: Jan 26, 2016
 *      Author: petera
 */

#include "cli.h"
#include "miniutils.h"
#include <stdarg.h>

#ifndef CONFIG_CLI

void cli_init(void) {}
void cli_parse(char *s, u32_t len) {}
s32_t cli_help(u32_t args, ...) {return 0;}

#else

static struct
{
  u32_t arg_sta_ix;
  char line[CLI_MAX_LINE];
  u32_t ix;
  char *args[CLI_MAX_ARGS];
  u32_t arg_ix;
} _cli;

extern void * _variadic_call(void *function_pointer, int nbr_of_args, void *arg_vector);

// get the main menu from some file somewhere in linker space...
CLI_EXTERN_MENU(main)

// Finds a command by name in given command table. Will descend into extra menus
// to search further. Submenus will not be searched.
static const cli_cmd *_cli_cmd_by_name(const cli_cmd *cmd_tbl, const char *name) {
  const cli_cmd *cmd_stack[CLI_MAX_SUBMENU_DEPTH];
  u8_t cmd_stack_level = 0;
  do {
    if (cmd_tbl == NULL ||
        (cmd_tbl->fn == NULL && cmd_tbl->name == NULL)) {
      // end of cmd table
      if (cmd_stack_level > 0) {
        // pop up
        cmd_tbl = cmd_stack[--cmd_stack_level];
        continue;
      } else {
        // last cmd of top stack, bail out
        break;
      }
    }
    if (cmd_tbl->type == CLI_EXTRA) {
      // extra top level menu commands, stack down into extra menu
      ASSERT(cmd_stack_level < CLI_MAX_SUBMENU_DEPTH);
      cmd_stack[cmd_stack_level++] = cmd_tbl + 1;
      cmd_tbl = cmd_tbl->submenu;
      continue;
    } else if (strcmp(cmd_tbl->name, name) == 0) {
      return cmd_tbl;
    }
    cmd_tbl++;
  } while (TRUE);

  return NULL;
}

// return number of ones in number (hamming weight)
static u8_t _cli_count_ones(u32_t x) {
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  return (((x + (x >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 24;
}

// List functions in given command table.
// If recurse is TRUE, the function will descend into submenus
// and list those with extra indentation.
static void _cli_list_funcs(const cli_cmd *cmd_tbl, bool recurse) {
  u32_t visual_sub_level_bit_stack = 0;
  char indent[CLI_HELP_INDENT];
  memset(indent, ' ', CLI_HELP_INDENT);
  const cli_cmd *cmd_stack[CLI_MAX_SUBMENU_DEPTH];
  u8_t cmd_stack_level = 0;
  do {
    if (cmd_tbl == NULL ||
        (cmd_tbl->fn == NULL && cmd_tbl->name == NULL)) {
      // end of cmd table
      if (cmd_stack_level > 0) {
        // pop up
        cmd_tbl = cmd_stack[--cmd_stack_level];
        visual_sub_level_bit_stack >>= 1;
        continue;
      } else {
        // last of top stack, bail out
        break;
      }
    }
    if (cmd_tbl->type == CLI_EXTRA) {
      // extra menu commands in submenu, stack down into extra submenu
      ASSERT(cmd_stack_level < CLI_MAX_SUBMENU_DEPTH);
      cmd_stack[cmd_stack_level++] = cmd_tbl + 1;
      cmd_tbl = cmd_tbl->submenu;
      visual_sub_level_bit_stack = (visual_sub_level_bit_stack << 1) | 0; // no extra indentation
      continue;
    } else {
      // plain command or nonrecursed sub level menu

      u8_t visual_sub_level = _cli_count_ones(visual_sub_level_bit_stack);
      u8_t tab = 0;
      while (tab++ < visual_sub_level) {
        CLI_PRINTF("  ");
      }

      u32_t cmdlen = strlen(cmd_tbl->name);
      cmdlen += visual_sub_level*2;
      s32_t trail = cmdlen >= CLI_HELP_INDENT ? 1 : (CLI_HELP_INDENT-cmdlen);
      indent[trail] = 0;
      CLI_PRINTF("%c" "%s" "%s" "%s\n", cmd_tbl->type == CLI_SUB ? '>' : ' ', cmd_tbl->name, indent, cmd_tbl->help);
      indent[trail] = ' ';

      if (cmd_tbl->type == CLI_SUB && recurse) {
        // found a submenu, recurse down into it
        ASSERT(cmd_stack_level < CLI_MAX_SUBMENU_DEPTH);
        cmd_stack[cmd_stack_level++] = cmd_tbl + 1;
        cmd_tbl = cmd_tbl->submenu;
        visual_sub_level_bit_stack = (visual_sub_level_bit_stack << 1) | 1; // extra indentation
        continue;
      }
    }
    cmd_tbl++;
  } while (TRUE);
}

// Generic cli helper function listing all commands
s32_t cli_help(u32_t args,  ...) {
  char *first_arg = NULL;

  if (args > 0) {
    va_list va;
    va_start(va, args);
    first_arg = va_arg(va, char *);
    va_end(va);
  }

  if (args == 0) {
    _cli_list_funcs(_cli_menu_main, FALSE);
  } else if (first_arg != NULL && strcmp(first_arg, "all") == 0) {
    _cli_list_funcs(_cli_menu_main, TRUE);
  } else {
    va_list va;
    va_start(va, args);
    u32_t i;
    const cli_cmd *cmd_tbl = _cli_menu_main;
    for (i = 0; i < args; i++) {
      char *cmd = va_arg(va, char *);
      cmd_tbl = _cli_cmd_by_name(cmd_tbl, cmd);
      if (cmd_tbl == NULL || cmd_tbl->type == CLI_EXTRA) {
        CLI_PRINTF("ERR: unknown command \"%s\"\n", cmd);
        va_end(va);
        return CLI_OK;
      } else if (cmd_tbl->type == CLI_SUB) {
        if (i < args-1) {
          cmd_tbl = cmd_tbl->submenu;
        }
      }
    }
    va_end(va);

    if (cmd_tbl->type == CLI_FUNC) {
      CLI_PRINTF("%s\n", cmd_tbl->help);
    } else if (cmd_tbl->type == CLI_SUB) {
      _cli_list_funcs(cmd_tbl->submenu, FALSE);
    }
  }

  return CLI_OK;
}

// Scans through arguments until a CLI_FUNC is found
static void _cli_exec(void) {
  u32_t arg_ix = 0;
  const cli_cmd *exe_cmd = NULL;
  const cli_cmd *cur_cmd = _cli_menu_main;
  do {
    cur_cmd = _cli_cmd_by_name(cur_cmd, _cli.args[arg_ix]);
    if (cur_cmd == NULL || cur_cmd->type == CLI_EXTRA) {
      // not found, might be argument or badness
      break;
    } else if (cur_cmd->type == CLI_FUNC) {
      exe_cmd = cur_cmd;
      // found func, break and exec
      break;
    } else if (cur_cmd->type ==CLI_SUB) {
      // submenu
      if (arg_ix < _cli.arg_ix-1) {
        // enter submenu if not last argument
        cur_cmd = cur_cmd->submenu;
        arg_ix++;
      } else {
        exe_cmd = cur_cmd;
        // else stop here and return submenu
        break;
      }
    }
  } while (arg_ix < _cli.arg_ix);

  // check and exec
  if (exe_cmd == NULL || exe_cmd->type == CLI_EXTRA) {
    CLI_PRINTF("ERR: unknown command \"%s\"\n", _cli.args[arg_ix]);
  } else if (exe_cmd->type == CLI_SUB) {
    CLI_PRINTF("ERR: cannot execute menu \"%s\"\n", _cli.args[arg_ix]);
    _cli_list_funcs(exe_cmd->submenu, FALSE);
  } else if (exe_cmd->type == CLI_FUNC) {
    // exec func
    // substitute func string pointer arg with number of
    // remaining args according to cli_func signature
    _cli.args[arg_ix] = (char *)_cli.arg_ix-1-arg_ix;
    s32_t res = (s32_t)_variadic_call(exe_cmd->fn, _cli.arg_ix, &_cli.args[arg_ix]);
    if (res != CLI_OK) {
      CLI_PRINTF("ERR: %i (0x%08x)\n", res, res);
    }
  }
}

// Parses some characters
void cli_parse(char *s, u32_t len) {
  u8_t six = 0;
  while (six < len) {
    char c = s[six++];
    if (strchr(CLI_FORMAT_CHARS_IGNORE, c)) {
      // just ignore these characters
      continue;
    }

    if (_cli.ix >= CLI_MAX_LINE-1) {
      CLI_PRINTF("ERR: line too long\n");
      memset(&_cli, 0, sizeof(_cli));
      continue;
    }

    if (c == CLI_FORMAT_CHAR_DELIM || c == CLI_FORMAT_CHAR_END) {
      if (c == CLI_FORMAT_CHAR_DELIM && _cli.ix == 0) {
        continue;
      }
      // save argument..
      _cli.args[_cli.arg_ix++] = &_cli.line[_cli.arg_sta_ix];
      _cli.line[_cli.ix++] = '\0';
      _cli.arg_sta_ix = _cli.ix;

      if (_cli.arg_ix >= CLI_MAX_ARGS) {
        CLI_PRINTF("ERR: too many arguments\n");
        memset(&_cli, 0, sizeof(_cli));
        continue;
      }

      if (c == CLI_FORMAT_CHAR_DELIM) {
        // ..go on with next arg...
        continue;
      }
      else if (c == CLI_FORMAT_CHAR_END) {
        // .. exec
        _cli_exec();
        memset(&_cli, 0, sizeof(_cli));
        continue;
      }
    }

    _cli.line[_cli.ix++] = c;
  }
}

void cli_init(void) {
  memset(&_cli, 0, sizeof(_cli));
}

#endif // CONFIG_CLI
