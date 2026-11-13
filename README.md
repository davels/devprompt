# devprompt

A fast, customizable shell prompt generator.

## Features

- Simple configuration: command-line arguments and environment variables
- Customizable: choose colors and features with precise control over the layout
- No Dependencies: single binary

## Prerequisites

- A [Nerd Font](https://www.nerdfonts.com/) installed and enabled in your terminal

## Install

### Add to your shell

**Bash** `~/.bashrc`:
```bash
# simple prompt
PS1='$(devprompt --shell bash --format "{fg:cyan}%path.path%[ {fg:red}%vcs.status%] {fg:green}> ")'

# more complex with conditional elements
PS1='$(devprompt --format "{fg:green}%sys.user%@%sys.host%[{/} on {fg:magenta}%vcs.branch%[ {fg:red}\[%vcs.status%\]]]{/} at {fg:blue}%path.path%\n{fg:green bold}>{/} ")'
```

The format string can be specified with the `--format` option or with the `DEVPROMPT_FORMAT` environment variable.  If both are specified, the command line argument will take precedence.


## Format Syntax

A format string consists of text, {styles}, [conditional blocks], and %module.variables%.

### Escaping Text

Proceed special charaters with a backslash `\` to output them verbatim.

- `\{` → `{`
- `\}` → `}`
- `\[` → `[`
- `\]` → `]`
- `\%` → `%`
- `\n` → newline
- `\t` → tab
- `\\` → `\` (literal backslash)

### Style

A style consists of one or more of: foregroud color, background color, and options, surrounded by braces `{ }`. Extra whitespace is ignored.
- foreground color: `fg:COLOR`
- background color: `bg:COLOR`
- options: `bold` , `italic` , `underline`

A slash is used to reset all styles to the shell default: `{/}`

COLOR is either:
- a named shell color: black, red, green, yellow, blue, magenta, cyan, white, bright-black, bright-red, bright-green, bright-yellow, bright-blue, bright-magenta, bright-cyan, bright-white
- a hex color: `#999999`

Examples
| `{fg:red}` | red foreground |
| `{fg:blue bg:yellow bold}` | blue foregroud, yellow background, bold text |
| `{/}` | reset colors to shell default |

### Conditional block

Anything surrounded by brackets `[  ]` will only be writtin out if at least one contained variable outputs a value.

### Module Variable

Modules values are accessed through variables. `%MODULE.VARIABLE_NAME%`
