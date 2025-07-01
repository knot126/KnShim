#!/usr/bin/env python3
"""
Generate a loader for Lua
"""

import re
import os
import sys

func = re.compile(r"\(?\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*\)?\s*\([^\)]*\);")

def process_file(path, out, funcnames):
	with open(path, "r") as f:
		with open(out, "w") as g:
			for line in f.readlines():
				m = func.search(line)
				
				if "typedef" not in line and "luaL_getn" not in line and "luaL_setn" not in line and m:
					new_line = line.replace(f"({m[1]})", f"(*{m[1]})")
					if line == new_line: new_line = line.replace(m[1], f"(*{m[1]})")
					g.write(new_line)
					
					funcnames.add(m[1])
				elif line == '#include "luaconf.h"\n':
					g.write('#include "luaconf.h"\n#ifdef LUALOADER_SHOULD_DECLARE_FUNCTION_POINTERS\n#undef LUA_API\n#undef LUALIB_API\n#define LUA_API\n#define LUALIB_API\n#endif\n')
				else:
					g.write(line)

def process(path):
	funcnames = set()
	outp = path.replace("/lua-full", "/lua")
	
	process_file(f"{path}/lua.h", f"{outp}/lua.h", funcnames)
	process_file(f"{path}/lauxlib.h", f"{outp}/lauxlib.h", funcnames)
	process_file(f"{path}/lualib.h", f"{outp}/lualib.h", funcnames)
	
	with open(f"{outp}/loader.c", "w") as f:
		f.write(f"""#define LUALOADER_SHOULD_DECLARE_FUNCTION_POINTERS
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

void *KNGetSymbolAddr(const char *name);

void KNLoadLua(void) {{
""")
		
		for name in funcnames:
			f.write(f"\t{name} = KNGetSymbolAddr(\"{name}\");\n")
		
		f.write("}\n")

def main():
	d = sys.argv[1]
	
	process(d)

if __name__ == "__main__":
	main()
