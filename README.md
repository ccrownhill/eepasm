# Extendable EEP1 assembler

An assembler for EEP0 and EEP1 CPU as taught at Imperial College 2022.

This is an alternative version of [Tom Clarke's EEP1 assembler](https://github.com/tomcl/eepAssembler) with one major advantage:

**new custom instructions can be added via a configuration file** `inslist.eepc` **without needing to change the code**.

## Compilation

```
clang++ eepasm.cpp -o eepasm
```

or

```
g++ eepasm.cpp -o eepasm
```

## Usage

First move your instruction config file into `inslist.eepc` which needs to be in
the same directory as the `eepasm` binary.

Then run

```
eepasm infile outfile
```

## Adding custom instructions

Refer to the format of the given `inslist.eepc` to see how instructions are specified and just append them to the file.

## Specifcations of EEP1 assembly and machine code encoding

Main specification:

![img/main_ref.png](img/main_ref.png)

Jump instructions specification:

![img/jmp_ref.png](img/jmp_ref.png)

Memory load/store instruction specification:

![img/mem_ref.png](img/mem_ref.png)
