#include <catch.hpp>

#include "../telegram/api.h"
#include "../telegram/fake.h"

tg::TelegramCredentials GetTestCredentials(const std::string &url) {
    return tg::TelegramCredentials{"123", url};
}

TEST_CASE("Single getMe") {
    telegram::FakeServer fake("Single getMe");
    fake.Start();

    auto credentials = GetTestCredentials(fake.GetUrl());
    auto api = tg::TelegramApi(credentials);

    api.GetMe();

    fake.StopAndCheckExpectations();
}

TEST_CASE("getMe error handling") {
    telegram::FakeServer fake("getMe error handling");

    fake.Start();

    auto credentials = GetTestCredentials(fake.GetUrl());
    auto api = tg::TelegramApi(credentials);

    REQUIRE_THROWS_AS(api.GetMe(), tg::TelegramApiError);
    REQUIRE_THROWS_AS(api.GetMe(), tg::TelegramApiError);

    fake.StopAndCheckExpectations();
}

TEST_CASE("Single getUpdates and send messages") {
    telegram::FakeServer fake("Single getUpdates and send messages");
    fake.Start();

    auto credentials = GetTestCredentials(fake.GetUrl());
    auto api = tg::TelegramApi(credentials);

    auto updates = api.GetUpdates();
    REQUIRE(updates.size() == 4);
    REQUIRE(updates.at(0).message->chat->type == "private");
    REQUIRE(updates.at(1).message->chat->type == "private");
    REQUIRE(updates.at(2).message->chat->type == "group");
    REQUIRE(updates.at(3).message->chat->type == "group");
    REQUIRE(updates.at(3).message.value().entities.at(0).type == "bot_command");

    api.SendMessage(updates.at(0).message->chat->id, "Hi!");
    api.SendMessage(updates.at(1).message->chat->id, "Reply", updates.at(1).message->message_id);
    api.SendMessage(updates.at(1).message->chat->id, "Reply", updates.at(1).message->message_id);

    fake.StopAndCheckExpectations();
}

TEST_CASE("Handle getUpdates offset") {
    telegram::FakeServer fake("Handle getUpdates offset");
    fake.Start();

    auto credentials = GetTestCredentials(fake.GetUrl());
    auto api = tg::TelegramApi(credentials);

    auto updates = api.GetUpdates(std::nullopt, 5);
    REQUIRE(updates.size() == 2);
    auto max_update_id = std::max(updates.at(0).update_id, updates.at(1).update_id) + 1;
    updates.clear();

    updates = api.GetUpdates(max_update_id, 5);
    REQUIRE(updates.empty());

    updates = api.GetUpdates(max_update_id, 5);
    REQUIRE(updates.size() == 1);
}
