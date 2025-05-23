import os
import re

def rename_files_to_lowercase(root_dir):
    # First rename files to lowercase
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            print(filename)
            if filename.lower().endswith(('.cpp', '.h')):
                old_path = os.path.join(dirpath, filename)
                new_filename = filename.lower()
                new_path = os.path.join(dirpath, new_filename)
                if old_path != new_path:
                    print(f'Renaming: {old_path} -> {new_path}')
                    os.rename(old_path, new_path)

def update_includes_to_lowercase(root_dir):
    include_pattern = re.compile(r'(#include\s+")([^"]+)(")')
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.lower().endswith(('.cpp', '.h')):
                filepath = os.path.join(dirpath, filename)
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()

                def replacer(match):
                    inc = match.group(2)
                    return match.group(1) + inc.lower() + match.group(3)

                new_content = include_pattern.sub(replacer, content)

                if new_content != content:
                    print(f'Updating includes in: {filepath}')
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)

if __name__ == "__main__":
    root_directory = 'src/game'  # current directory, change if needed
    rename_files_to_lowercase(root_directory)
    # update_includes_to_lowercase(root_directory)
