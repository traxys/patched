# Patched

Patched is a generator of optimal patches for transforming one file into another. The patches are optimal in the sense that they require the less operations.

## Compilation

This uses C++17 so you need an up to date complier.

## Usage

`computePatchOpt file_source file_dest patch_name`

## Performance

This uses a heap of paths + hash map of markings / path saving. It is more performant the less files differ. In case of random files of `n` lines you can expect `n**2` usage of RAM and time at least.
In case of no change it performs in `n` iterations. Those are the worst and best case.
