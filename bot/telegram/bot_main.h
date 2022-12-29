#ifndef BOT_MAIN_H
#define BOT_MAIN_H

#include "api.h"
#include "config.h"
#include "logger.h"
#include "message_handlers.h"
#include "utils.h"

#include <fstream>
#include <memory>

class BotServer {
public:
    BotServer(std::shared_ptr<BotServerConfig> config)
        : config_{config},
          api_{std::make_shared<tg::TelegramApi>(config->credentials, config->network_mode)},
          logger_{logger::LoggerFactory::GetStdoutLogger()} {
        message_handler_factory_ = std::make_shared<MessageHandlerFactory>(api_);
        LoadOffset();
    }

    ~BotServer() {
        message_handler_factory_.reset();
        api_.reset();
        config_.reset();
    }

    void Start() {
        logger_->LogInfo("Starting telegram bot server");
        while (!shutdown_) {
            LogAndIgnoreTelegramErrors([&]() {
                auto offset = NextOffset();

                auto updates = api_->GetUpdates(offset);

                for (const auto& update : updates) {
                    if (shutdown_) {
                        break;
                    }

                    logger_->LogInfo("Handling update_id: " + std::to_string(update.update_id) +
                                     ", message: " + update.GetMessageTextOrEmpty());

                    HandleUpdate(update);
                }
            });
        }
    }

private:
    void HandleUpdate(const tg::TelegramUpdate& update) {
        UpdateAndDumpOffset(update);
        if (!update.message.has_value()) {
            return;
        }
        auto message = update.message.value();
        auto handler = message_handler_factory_->GetHandler(message);
        try {
            handler->handle(message);
        } catch (handler_exceptions::CrashRequested) {
            logger_->LogError("Got crash request");
            exit(-1);
        } catch (handler_exceptions::ShutdownRequested) {
            logger_->LogInfo("Got shutdown request");
            shutdown_ = true;
        } catch (std::exception exception) {
            logger_->LogError("Unknown exception: " + std::string(exception.what()));
        } catch (...) {
            logger_->LogError("Unknown exception");
        }
    }

    template <typename Code>
    void LogAndIgnoreTelegramErrors(Code&& lambda) {
        try {
            lambda();
        } catch (tg::TelegramApiError err) {
            logger_->LogError("Telegram api error: " + std::string(err.what()));
        }
    }

    void UpdateAndDumpOffset(const tg::TelegramUpdate& update) {
        auto old_offset = offset_;
        if (offset_.has_value()) {
            offset_ = std::max(offset_.value(), update.update_id);
        } else {
            offset_ = update.update_id;
        }
        logger_->LogInfo("Saving offset from " + GetString(old_offset) + " to " +
                         GetString(offset_));
        DumpOffset();
    }

    std::optional<int64_t> NextOffset() const {
        if (offset_.has_value()) {
            return offset_.value() + 1;
        }
        return offset_;
    }

    void DumpOffset() {
        if (!offset_.has_value()) {
            return;
        }
        std::ofstream out(config_->path_to_backup_file, std::ios::out);
        out << offset_.value() << std::endl;
        out.close();
    }

    void LoadOffset() {
        std::ifstream in(config_->path_to_backup_file, std::ios::in);
        if (!in.is_open()) {
            return;
        }
        int64_t current;
        in >> current;
        offset_ = current;
    }

private:
    std::shared_ptr<BotServerConfig> config_;
    std::shared_ptr<tg::TelegramApi> api_;
    std::shared_ptr<MessageHandlerFactory> message_handler_factory_;
    std::shared_ptr<logger::Logger> logger_;
    std::optional<int64_t> offset_;
    bool shutdown_{false};
};

#endif  // BOT_MAIN_H
