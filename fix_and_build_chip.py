#!/usr/bin/env python3
"""GN gen + fix build.ninja for chip library"""
import subprocess, sys, os

project_dir = sys.argv[1]
chip_root = sys.argv[2]
build_dir = sys.argv[3]

gn_exe = os.path.join(project_dir, 'gn', 'out', 'gn.exe')
gn_root = os.path.join(chip_root, 'config', 'esp32')

# Step 1: GN gen
print(f"GN gen: {gn_exe} --root={gn_root} gen {build_dir}")
subprocess.check_call([gn_exe, f'--root={gn_root}', 'gen', build_dir])

# Step 2: Fix build.ninja
bn_path = os.path.join(build_dir, 'build.ninja')
with open(bn_path) as f:
    lines = f.readlines()

new_lines = []
skip = 0
for l in lines:
    s = l.strip()
    if s == 'rule gn':
        skip = 3
        continue
    if skip > 0:
        skip -= 1
        continue
    if 'build.ninja.stamp' in s:
        continue
    if 'build build.ninja: gn' == s:
        continue
    if s == 'generator = 1':
        continue
    if s == 'depfile = build.ninja.d':
        continue
    new_lines.append(l)

with open(bn_path, 'w') as f:
    f.writelines(new_lines)

print(f"build.ninja fixed: {len(lines)} -> {len(new_lines)} lines")
