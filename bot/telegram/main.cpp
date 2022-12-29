#include <iostream>
#include <optional>

#include "bot_main.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./bot-run <token_file_path> <offset_backup_path>" << std::endl;
        return -1;
    }

    auto token = GetToken(argv[1]);
    if (!token.has_value()) {
        return -1;
    }

    auto config = std::make_shared<BotServerConfig>(
        tg::TelegramCredentials{token.value(),
                                "https://api.telegram.org/"},
        argv[2], NetworkMode::HTTPS);

    BotServer server{config};
    server.Start();
    return 0;
}