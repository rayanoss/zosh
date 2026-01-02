# Zosh - A Simple Unix Shell

A minimal Unix shell implementation written in C. This is my first C project, developed as a learning exercise to understand systems programming, process management, and Unix system calls.

## Features

- Execute external commands
- Built-in commands: `cd`, `exit`
- Input/output redirection (`<`, `>`, `>>`)
- Command pipelines (`|`)
- Background process execution (`&`)
- Signal handling (Ctrl+C)
- Current working directory display in prompt

## Building

```bash
gcc -Wall -Wextra -o shell shell.c
```

## Usage

Run the shell:

```bash
./shell
```

The prompt displays the current working directory:

```
zosh:/home/user>
```

### Examples

Execute a command:
```
zosh:/home/user> ls -la
```

Change directory:
```
zosh:/home/user> cd /tmp
zosh:/tmp>
```

Redirect output:
```
zosh:/tmp> ls > files.txt
zosh:/tmp> echo "text" >> files.txt
```

Redirect input:
```
zosh:/tmp> cat < input.txt
```

Pipeline commands:
```
zosh:/tmp> ls | grep txt | sort
```

Run in background:
```
zosh:/tmp> sleep 10 &
[1] 12345
```

Exit the shell:
```
zosh:/tmp> exit
```

## Architecture

The shell follows a classic read-eval-print loop:

1. Read user input
2. Parse and tokenize the command line
3. Detect special operators (pipes, redirections, background)
4. Execute commands using fork/exec
5. Wait for process completion (if not background)

Key components:

- `zosh_loop()` - Main shell loop
- `zosh_read_line()` - Input reading with dynamic buffer
- `zosh_split_line()` - Tokenization
- `zosh_execute()` - Command dispatcher
- `zosh_launch()` - Simple command execution
- `zosh_execute_pipes()` - Pipeline execution

## Limitations

- No job control (fg, bg, jobs commands)
- No command history
- No command line editing
- Pipes and redirections cannot be combined
- Limited error handling for edge cases

## Learning Outcomes

Through this project, I learned about:

- Process creation and management (fork/exec/wait)
- File descriptors and I/O redirection (dup2)
- Inter-process communication (pipes)
- Signal handling
- Memory management in C (malloc/free)
- String manipulation and parsing
