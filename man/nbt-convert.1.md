---
title: "NBT-CONVERT(1)"
date: "nbt-convert-INSERT_VERSION_HERE"
---

# NAME

**nbt-convert** - Convert NBT data

# SYNOPSIS

**nbt-convert** string|binary|edit \<input\> \<output\>...

# DESCRIPTION

Convert Named Binary Tag (NBT) data to stringified NBT (SNBT) data or vice versa

# OPTIONS

type:\
string|binary: Type to convert to. edit: Open the file in your text editor.

input: Input file\
Defaults to stdin. Use - for stdin.

output: Output file\
Defaults to stdin, or to input when editing. Use - for stdin.

# EXAMPLES

- `nbt-convert string servers.dat servers.nbt`
- `nbt-convert edit level.dat`

# BUGS

[Report bugs here](https://github.com/mekb-turtle/nbt-convert/issues)

# AUTHORS

mekb <mekb111@pm.me>
