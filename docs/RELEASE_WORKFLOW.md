# Процесс релиза МикРоББокс

## Обзор

Релизный workflow использует **атомарный подход** для предотвращения скачивания пользователями незавершенных сборок.

## Проблема (до улучшения)

Ранее workflow работал следующим образом:
1. ✅ Создание git тега
2. ⏳ Сборка firmware (5-10 минут)
3. ⏳ Создание релиза
4. ⏳ Загрузка артефактов

**Риск**: Пользователи могли попытаться скачать релиз по тегу до завершения сборки, получая ошибку или неполный набор файлов.

## Решение (текущая реализация)

Новый workflow использует подход с черновиками (draft releases):

1. ✅ Вычисление версии
2. ✅ Сборка firmware (release и debug)
3. ✅ Подготовка файлов и контрольных сумм
4. ✅ **Создание черновика релиза** с тегом и всеми файлами
5. ✅ **Публикация релиза** (делает его видимым пользователям)

## Преимущества

### Атомарность
- **Тег создается вместе с артефактами**: GitHub создает тег автоматически при создании релиза
- **Все или ничего**: Релиз либо полностью готов, либо не виден пользователям
- **Нет промежуточных состояний**: Пользователи никогда не увидят тег без файлов

### Надежность
- Если сборка падает, релиз не создается
- Если загрузка файлов падает, релиз остается черновиком
- Только после успешной публикации релиз становится доступным

### Соответствие best practices
Реализация основана на рекомендациях:
- GitHub Actions best practices
- Atomic releases pattern
- Draft → Build → Publish workflow

## Как работает workflow

### Шаг 1: Подготовка
```yaml
- Calculate Next Version
- Setup environment
- Generate Epic Release Name
```

### Шаг 2: Сборка
```yaml
- Build Release Firmware
- Build Debug Firmware
- Prepare Release Files (with SHA256 checksums)
```

### Шаг 3: Создание черновика
```yaml
- name: Create Draft GitHub Release
  uses: softprops/action-gh-release@v1
  with:
    draft: true  # Релиз НЕ виден пользователям
    files: |
      microbox-*.bin
      microbox-*.sha256
```

В этот момент:
- ✅ Релиз создан
- ✅ Тег создан
- ✅ Все файлы загружены
- ❌ Релиз **не виден** публике

### Шаг 4: Публикация
```yaml
- name: Publish Release
  run: |
    gh release edit ${{ steps.version.outputs.version }} --draft=false
```

Только после этого шага релиз становится видимым пользователям.

## Что происходит при ошибках

### Ошибка при сборке
- Workflow прерывается
- Черновик не создается
- Тег не создается
- Пользователи ничего не видят

### Ошибка при загрузке файлов
- Workflow прерывается
- Черновик создан, но не опубликован
- Тег создан, но релиз невиден
- Пользователи ничего не видят
- Черновик можно удалить вручную или переиспользовать

### Ошибка при публикации
- Черновик остается в репозитории
- Можно опубликовать вручную через UI
- Или перезапустить workflow

## Запуск релиза

### Через GitHub UI

1. Откройте страницу Actions
2. Выберите "МикРоББокс Release"
3. Нажмите "Run workflow"
4. Выберите тип релиза:
   - `patch` - для багфиксов (0.0.X)
   - `minor` - для новых функций (0.X.0)
   - `major` - для breaking changes (X.0.0)

### Мониторинг процесса

Workflow показывает прогресс:
- ✅ Version calculation
- ⏳ Building firmware... (может занять 5-10 минут)
- ✅ Creating draft release
- ✅ Publishing release

## Проверка релиза

После публикации проверьте:

1. **Релиз виден на странице Releases**
   ```
   https://github.com/GOODWORKRINKZ/microbbox/releases
   ```

2. **Все файлы присутствуют**
   - `microbox-vX.Y.Z-release.bin`
   - `microbox-vX.Y.Z-release.bin.sha256`
   - `microbox-vX.Y.Z-debug.bin`
   - `microbox-vX.Y.Z-debug.bin.sha256`

3. **Тег создан**
   ```bash
   git fetch --tags
   git tag -l
   ```

4. **Контрольные суммы корректны**
   ```bash
   sha256sum -c microbox-vX.Y.Z-release.bin.sha256
   ```

## Откат релиза

Если нужно откатить релиз:

1. Удалить релиз:
   ```bash
   gh release delete vX.Y.Z
   ```

2. Удалить тег локально и удаленно:
   ```bash
   git tag -d vX.Y.Z
   git push --delete origin vX.Y.Z
   ```

## Альтернативные подходы

Во время разработки рассматривались другие подходы:

### Подход с префиксами тегов
```
1. Создать тег pre-v1.0.0
2. Собрать firmware
3. Создать релиз
4. Переименовать тег в v1.0.0
```

**Не выбран**, потому что:
- Сложнее в реализации
- Требует дополнительных git операций
- GitHub не поддерживает переименование тегов напрямую
- Draft releases проще и стандартнее

### Подход с artifacts и отдельным релизом
```
1. Собрать и сохранить artifacts
2. В отдельном job создать релиз с artifacts
```

**Не выбран**, потому что:
- Усложняет workflow
- Требует передачи artifacts между jobs
- Draft releases позволяют сделать все в одном job

## Дополнительные ресурсы

- [GitHub Actions: Releasing](https://docs.github.com/en/actions/how-tos/create-and-publish-actions/release-and-maintain-actions)
- [Best Practices for GitHub Actions Workflows](https://exercism.org/docs/building/github/gha-best-practices)
- [Atomic Releases Pattern](https://pakstech.com/blog/github-actions-release-workflow/)

## Версия документа

- **Создан**: 2025-01-11
- **Последнее обновление**: 2025-01-11
- **Применимо к**: v0.0.16+
