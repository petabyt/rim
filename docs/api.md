# Padding / Gap

LibUI: Padding is the space between individual controls
CSS: Padding is space around an element, gap is space between controls
Rim: Padding is always inner padding (Like CSS `box-sizing: border-box;`), Gap is space inbetween individual controls

In Rim this is `RIM_PROP_GAP`

# Margin

LibUI: Space around a control (`SetMargined`)
CSS: Space around a control

In Rim, 'space around a control' is `RIM_PROP_INNER_PADDING`.
TODO: Rename to margin? Not a big difference. mainly unique to imgui so not a big deal.

# Expand

Whether to take 100% of space left. Other controls that are expanded share space equally.

In Rim this is known as `RIM_PROP_EXPAND`
