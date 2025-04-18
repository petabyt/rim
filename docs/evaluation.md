# Immediate-mode UI API evaluation

Requirements:
- Basic layout constructs
- Not hardcoded on pixels
- ABI

## imgui:
- [hello_imgui](https://github.com/pthom/hello_imgui)
- Almost no layout constructs. Every layout has to be calculated manually
- Not good at scaling
- Hard to use through C (cimgui exists, but is outdated)

## nuklear:
- bad at scaling by design
- Not intended for use outside embedding/games

## microui:
- No tabs, complex layouts, etc
