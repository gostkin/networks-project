#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "api.h"
#include "logger.h"
#include "network_mode.h"

struct BotServerConfig {
    tg::TelegramCredentials credentials;
    std::string path_to_backup_file;
    NetworkMode network_mode{NetworkMode::HTTP};
    logger::LogLevel min_level{logger::LogLevel::Info};

    BotServerConfig(const tg::TelegramCredentials& creds, const std::string& backup_file_path,
                    NetworkMode mode, logger::LogLevel level = logger::LogLevel::Info)
        : credentials{creds},
          path_to_backup_file{backup_file_path},
          network_mode{mode},
          min_level{level} {
    }
};

#endif  // CONFIG_H
