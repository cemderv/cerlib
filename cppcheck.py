import json
import multiprocessing
import shutil
import subprocess
import sys
import os

build_dir = sys.argv[1]
compile_commands_json = os.path.join(build_dir, "compile_commands.json")

if not os.path.exists(compile_commands_json):
    sys.exit(f'error: compile_commands.json not found in {build_dir}')

print('Performing Cppcheck checks in directory:', build_dir)

cppcheck_cmd = shutil.which('cppcheck')
if not cppcheck_cmd:
    sys.exit('error: unable to find cppcheck')

print('Found cppcheck:', cppcheck_cmd)

with open(compile_commands_json) as f:
    json_doc = json.load(f)


# Remove entries that are irrelevant for checking.
def is_valid_entry(obj):
    key = None

    if 'output' in obj:
        key = obj['output']
    elif 'file' in obj:
        key = obj['file']

    if key is None:
        return True

    return ('_deps' not in key and
            'cerlibTests.dir' not in key and
            'embedded_files' not in key and
            'stb.c' not in key)


json_doc = [obj for obj in json_doc if is_valid_entry(obj)]

output_filename = os.path.join(build_dir, "compile_commands_cppcheck.json")

with open(output_filename, "w") as f:
    f.write(json.dumps(json_doc, indent=2))

num_jobs = multiprocessing.cpu_count()

subprocess.check_call([
    cppcheck_cmd,
    f'--project={output_filename}',
    '--enable=warning',
    '--std=c++20',
    '-j', str(num_jobs)
], shell=False)
