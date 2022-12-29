#include <iostream>
#include <fstream>
#include <optional>

#include "bot_main.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Invalid arguments number" << std::endl;
        return -1;
    }

    auto token = GetToken(argv[1]);
    if (!token.has_value()) {
        return -1;
    }

    auto config = std::make_shared<BotServerConfig>(
        tg::TelegramCredentials{token.value(),
                                "https://api.telegram.org/"},
        "backup_offset.data", NetworkMode::HTTPS);

    BotServer server{config};
    server.Start();
    return 0;
}