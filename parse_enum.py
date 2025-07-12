from clang.cindex import Index, CursorKind
import os

def find_enum(cursor, name):
	for c in cursor.get_children():
		if c.kind == CursorKind.ENUM_DECL and c.spelling == name:
			return c
		res = find_enum(c, name)
		if res:
			return res
	return None

def parse_enum(enum_cursor):
	return [(c.spelling, c.enum_value) for c in enum_cursor.get_children()
			if c.kind == CursorKind.ENUM_CONSTANT_DECL]

def name_to_string(name, kind):
	if kind == "widget":
		return name.removeprefix("RIM_").lower()
	elif kind == "prop":
		return name.removeprefix("RIM_PROP_").lower()
	return name

def generate_c_function(func_name, param_name, members, kind):
	lines = []
	lines.append(f'const char *{func_name}(uint32_t {param_name}) {{')
	lines.append(f'\tswitch ({param_name}) {{')
	for name, _ in members:
		val = name_to_string(name, kind)
		lines.append(f'\tcase {name}: return "{val}";')
	lines.append('\tdefault: {')
	lines.append(f'\t\tprintf("{func_name}: unknown %d\\n", {param_name});')
	lines.append('\t\treturn "???";')
	lines.append('\t}')
	lines.append('\t}')
	lines.append('}')
	lines.append('')
	return '\n'.join(lines)

def main():
	index = Index.create()
	tu = index.parse("src/rim_internal.h", args=['-x', 'c', '-std=c99'])

	widget_enum = find_enum(tu.cursor, "RimWidgetType")
	prop_enum = find_enum(tu.cursor, "RimPropType")

	if not widget_enum or not prop_enum:
		print("Missing required enums.")
		return

	widget_members = parse_enum(widget_enum)
	prop_members = parse_enum(prop_enum)

	lines = []
	lines.append('#include <stdio.h>')
	lines.append('#include <stdint.h>')
	lines.append('#include "rim_internal.h"\n')

	lines.append(generate_c_function("rim_eval_widget_type", "type", widget_members, "widget"))
	lines.append(generate_c_function("rim_eval_prop_type", "type", prop_members, "prop"))

	with open("src/enum.c", 'w') as f:
		f.write('\n'.join(lines))

	print("Generated: src/enum.c")

if __name__ == '__main__':
	main()
