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

## Format Syntax

A format string consists of text, {styles}, [conditional blocks], and %module.variables%.

### Style


### Conditional block


### Module Variable


