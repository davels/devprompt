# devprompt

A fast, lightweight, and customizable shell prompt generator with flexible formatting and Git integration.

## Features

- **Simple configuration:** Configure using command-line options or environment variables.
- **Customizable:** Control colors, formatting, and which prompt elements are displayed.
- **Lightweight:** A single, standalone binary with no runtime dependencies.

## Prerequisites

- A [Nerd Font](https://www.nerdfonts.com/) installed and enabled in your terminal is recommended for displaying icons.

## Shell Integration

Add the following to your `~/.bashrc`:

**Simple prompt**
```bash
PS1='$(devprompt --shell bash --format "{fg:cyan}%path.path%[ {fg:red}%vcs.status%] {fg:green}> ")'
```

**More complex prompt with conditional elements**
```bash
PS1='$(devprompt --shell bash --format "{fg:green}%sys.user%@%sys.host%[{/} on {fg:magenta}%vcs.branch%[ {fg:red}\[%vcs.status%\]]]{/} at {fg:blue}%path.path%\n{fg:green bold}>{/} ")'
```

The format string can be specified using the `--format` command-line option or the `DEVPROMPT_FORMAT` environment variable. If both are specified, the command-line option takes precedence.

## Format Syntax

A format string consists of literal text, styles, conditional blocks, and module variables.

### Escaping Text

Precede special characters with a backslash (`\`) to output them literally.

| Escape | Output |
| :--- | :--- |
| `\{` | `{` |
| `\}` | `}` |
| `\[` | `[` |
| `\]` | `]` |
| `\%` | `%` |
| `\n` | Newline |
| `\t` | Tab |
| `\\` | `\` (literal backslash) |

### Styles

Styles are specified inside braces (`{ }`) and can contain one or more of the following:

- Foreground color: `fg:COLOR`
- Background color: `bg:COLOR`
- Modifiers: `bold`, `italic`, `underline`, `strikethrough`

Extra whitespace inside the braces is ignored.

Use `{/}` to reset all styles to the shell default.

`COLOR` can be either a named color or a hexadecimal color code.

**Named colors:**

`black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, `bright-black`, `bright-red`, `bright-green`, `bright-yellow`, `bright-blue`, `bright-magenta`, `bright-cyan`, `bright-white`

**Hexadecimal colors:**

`#hexcode`

**Examples**

| Style | Effect |
| :--- | :--- |
| `{fg:red}` | red foreground |
| `{fg:#1111FF bg:yellow bold}` | blue foreground, yellow background, bold text |
| `{/}` | Reset styles to the shell default |

### Conditional Blocks

Anything enclosed in square brackets (`[ ]`) is treated as a conditional block.

A conditional block is only included in the output if at least one variable inside the block produces a non-empty value. Otherwise, the entire block is omitted.

### Module Variables

Module variables are used to include dynamic information in the prompt.

Variables follow the format:

`%MODULE.VARIABLE_NAME%`

For example:

- `%sys.user%`
- `%sys.host%`
- `%vcs.branch%`
- `%path.path%`
- `%vcs.status%`
