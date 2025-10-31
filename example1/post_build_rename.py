from SCons.Script import AlwaysBuild, Default

Import("env")

import os
import shutil

project_dir = env['PROJECT_DIR']

def find_git_version(cpp_defines):
    for define in cpp_defines:
        if isinstance(define, tuple) and define[0] == "GIT_VERSION":
            return define[1].strip('"')
    return "unknown-version"

def rename_firmware(source, target):
    print(f"Source: {source}")
    print(f"Target: {target}")
    
    target_dir = os.path.dirname(target)
    if not os.path.exists(target_dir):
        print(f"Target directory {target_dir} does not exist. Creating...")
        os.makedirs(target_dir)
    
    if os.path.exists(source):
        print(f"Source file {source} exists, renaming to {target}")
        shutil.copyfile(source, target)
        print(f"File copied successfully to {target}")
    else:
        print(f"Source file {source} does not exist. Cannot rename.")

# Извлекаем версию из CPPDEFINES
git_version = find_git_version(env['CPPDEFINES'])
print(f"GIT_VERSION: {git_version}")

# Настройка действия после сборки
def after_build_action(source, target, env):
    source_path = os.path.abspath(str(target[0]))
    target_path = os.path.abspath(os.path.join(project_dir, "builds", f"FILIN{git_version}.bin"))
    print(f"Post build action: renaming {source_path} to {target_path}")
    rename_firmware(source_path, target_path)

# Используем env.AddPostAction для добавления действия после сборки
env.AddPostAction(
    "$BUILD_DIR/firmware.bin",
    after_build_action
)

# Убедимся, что таргет всегда будет собран
target_firmware = env.Alias('build_target', "$BUILD_DIR/firmware.bin", after_build_action)
AlwaysBuild(target_firmware)
Default(target_firmware)