Import("env")

import subprocess
import datetime

def get_git_version():
    """Получить версию из Git тегов."""
    try:
        # Тег и хеш коммита
        tag = subprocess.check_output(['git', 'describe', '--tags', '--abbrev=0'], stderr=subprocess.STDOUT).strip().decode('utf-8')
        commit_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], stderr=subprocess.STDOUT).strip().decode('utf-8')
        git_version = "{}-{}".format(tag, commit_hash)
        return git_version
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Git не найден или нет тегов, используется версия по умолчанию")
        return "v1.0.0-dev"

# Получаем версию из Git
git_version = get_git_version()

# Получаем дату сборки
build_date = datetime.datetime.now().strftime("%Y-%m-%d")

# Добавляем версии в CPPDEFINES
env.Append(
    CPPDEFINES=[
        ("GIT_VERSION", '"{}"'.format(git_version)),
        ("PROJECT_NAME", '"МикроББокс"'),
        ("BUILD_DATE", '"{}"'.format(build_date))
    ]
)

print("Building МикроББокс version:", git_version, "на", build_date)