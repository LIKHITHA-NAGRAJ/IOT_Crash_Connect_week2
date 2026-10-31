import os, re, shutil, sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
src = os.path.join(root, "Firmware")
tpl = os.path.join(root, "Wokwi_Template")
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "Wokwi_Project")

if os.path.isdir(out):
    shutil.rmtree(out)
os.makedirs(out)

files = []
for d, _, names in os.walk(src):
    for n in names:
        if n.endswith((".h", ".cpp")):
            files.append(os.path.join(d, n))


def flat_name(path):
    base = os.path.basename(path)
    return base if base.startswith("cc_") else "cc_" + base


seen = {}
for f in files:
    n = flat_name(f)
    if n in seen:
        sys.exit("name clash: %s and %s" % (seen[n], f))
    seen[n] = f

pattern = re.compile(r'#include "(?:[^"]*/)?([^"/]+\.h)"')
for n, f in sorted(seen.items()):
    text = open(f).read()
    text = pattern.sub(lambda m: '#include "%s"' % (m.group(1) if m.group(1).startswith("cc_") else "cc_" + m.group(1)), text)
    open(os.path.join(out, n), "w").write(text)

for n in os.listdir(tpl):
    shutil.copy(os.path.join(tpl, n), os.path.join(out, n))
print("wrote %d flat source files and %d project files to %s" % (len(seen), len(os.listdir(tpl)), out))
