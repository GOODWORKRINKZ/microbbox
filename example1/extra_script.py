Import("env")

import subprocess

def get_git_version():
    try:
        # Тег и хеш коммита
        tag = subprocess.check_output(['git', 'describe', '--tags', '--abbrev=0']).strip().decode('utf-8')
        commit_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).strip().decode('utf-8')
        git_version = "{}-{}".format(tag, commit_hash)
        return git_version
    except subprocess.CalledProcessError as e:
        print("Error retrieving GIT version:")
        print(e.output)
        return "unknown-version"

# Проверяем BUILD_FLAGS и собираем build_sign
build_flags = env.get('BUILD_FLAGS')
build_sign = ""
build_sign += "1" if any("-D RADIO_1_2G_ENABLED" in flag for flag in build_flags) else "0"
build_sign += "1" if any("-D RADIO_2_4G_ENABLED" in flag for flag in build_flags) else "0"
build_sign += "1" if any("-D RADIO_5_8G_ENABLED" in flag for flag in build_flags) else "0"

# Получаем версию из Git и добавляем build_sign
git_version = get_git_version() + "-{}".format(build_sign)

# Добавляем версии в CPPDEFINES
env.Append(
    CPPDEFINES=[
        ("GIT_VERSION", '"{}"'.format(git_version)),
        ("TARGET", '"FILIN_{}"'.format(build_sign))
    ]
)

print("Building version:", git_version)