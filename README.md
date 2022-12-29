# Телеграм бот - проект для курса по сетям

В курсе требовалось сделать проект с сетевым взаимодействием, чат-бот - подходящий проект. Потестировать можно тут: [ссылка](https://t.me/shad_cpp_gostkin_bot)

## Установка и запуск

Для запуска требуются `docker` и `docker-compose` (3.8+). Также необходимо предварительно создать бота через [отца ботов](https://t.me/BotFather).
Далее шаги следующие:
* берем ключ апи телеграма для созданного бота и кладем в файл `config/credentials.yml`
* запускаем бота командой:
```shell
docker compose -f docker-compose.yml up -d --build --force-recreate
```
## Что умеет делать бот

* Запрос `/random`. Бот посылает случайное число ответом на это сообщение.

* Запрос `/weather`. Бот отвечает в чат `Winter Is Coming`.

* Запрос `/styleguide`. Бот отвечает в чат смешной шуткой на тему code review.

* Запрос `/stop`. Процесс бота завершается штатно.

* Запрос `/crash`. Процесс бота завершается аварийно. Например выполняет `abort();`.

Бот хранит оффсет, помнит на чем он остановился, сохраняет в `config/offset_backup.data`
