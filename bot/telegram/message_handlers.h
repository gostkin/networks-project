#ifndef MESSAGE_HANDLERS_H
#define MESSAGE_HANDLERS_H

#include "api.h"

#include <cassert>
#include <memory>
#include <random>

namespace handler_exceptions {

class ShutdownRequested : public std::exception {};

class CrashRequested : public std::exception {};

}  // namespace handler_exceptions

class MessageHandler {
public:
    MessageHandler(std::shared_ptr<tg::TelegramApi> api) : api_{api} {
    }
    virtual void handle(const tg::TelegramApiMessage& message) = 0;
    virtual bool matches(const tg::TelegramApiMessage& message) const = 0;
    virtual ~MessageHandler() {
    }

protected:
    std::shared_ptr<tg::TelegramApi> api_;
};

class RandomMessageHandler : public MessageHandler {
public:
    RandomMessageHandler(std::shared_ptr<tg::TelegramApi> api)
        : MessageHandler(api), dist_{0, std::numeric_limits<uint64_t>::max()} {
    }

    void handle(const tg::TelegramApiMessage& message) override final {
        api_->SendMessage(message.chat->id, std::to_string(dist_(mt_)));
    }

    bool matches(const tg::TelegramApiMessage& message) const override final {
        return message.text && message.chat && message.text == "/random";
    }

private:
    std::mt19937 mt_{};
    std::uniform_int_distribution<uint64_t> dist_;
};

class WeatherMessageHandler : public MessageHandler {
public:
    WeatherMessageHandler(std::shared_ptr<tg::TelegramApi> api) : MessageHandler(api) {
    }

    void handle(const tg::TelegramApiMessage& message) override final {
        api_->SendMessage(message.chat->id, "Winter Is Coming");
    }

    bool matches(const tg::TelegramApiMessage& message) const override final {
        return message.text && message.chat && message.text == "/weather";
    }
};

class ReviewJokeMessageHandler : public MessageHandler {
public:
    ReviewJokeMessageHandler(std::shared_ptr<tg::TelegramApi> api) : MessageHandler(api) {
    }

    void handle(const tg::TelegramApiMessage& message) override final {
        api_->SendMessage(message.chat->id, "A funny joke about review");
    }

    bool matches(const tg::TelegramApiMessage& message) const override final {
        return message.text && message.chat && message.text == "/styleguide";
    }
};

class DefaultMessageHandler : public MessageHandler {
public:
    DefaultMessageHandler(std::shared_ptr<tg::TelegramApi> api) : MessageHandler(api) {
    }

    void handle(const tg::TelegramApiMessage& message) override final {
        api_->SendMessage(message.chat->id,
                          "Sorry, your message is not recognized: " +
                              (message.text.has_value() ? message.text.value() : ""));
    }

    bool matches(const tg::TelegramApiMessage&) const override final {
        return true;
    }
};

class ExitOkComandHandler : public MessageHandler {
public:
    ExitOkComandHandler(std::shared_ptr<tg::TelegramApi> api) : MessageHandler(api) {
    }

    void handle(const tg::TelegramApiMessage&) override final {
        throw handler_exceptions::ShutdownRequested();
    }

    bool matches(const tg::TelegramApiMessage& message) const override final {
        return message.text && message.text == "/stop";
    }
};

class ExitCrashComandHandler : public MessageHandler {
public:
    ExitCrashComandHandler(std::shared_ptr<tg::TelegramApi> api) : MessageHandler(api) {
    }

    void handle(const tg::TelegramApiMessage&) override final {
        throw handler_exceptions::CrashRequested();
    }

    bool matches(const tg::TelegramApiMessage& message) const override final {
        return message.text && message.text == "/crash";
    }
};

class MessageHandlerFactory {
public:
    MessageHandlerFactory(std::shared_ptr<tg::TelegramApi> api) {
        handlers_.emplace_back(std::make_shared<RandomMessageHandler>(api));
        handlers_.emplace_back(std::make_shared<WeatherMessageHandler>(api));
        handlers_.emplace_back(std::make_shared<ReviewJokeMessageHandler>(api));
        handlers_.emplace_back(std::make_shared<ExitCrashComandHandler>(api));
        handlers_.emplace_back(std::make_shared<ExitOkComandHandler>(api));

        default_handler_ = std::make_shared<DefaultMessageHandler>(api);
    }

    std::shared_ptr<MessageHandler> GetHandler(const tg::TelegramApiMessage& message) const {
        for (auto& handler : handlers_) {
            if (handler->matches(message)) {
                return handler;
            }
        }
        return default_handler_;
    }

    ~MessageHandlerFactory() {
        handlers_.clear();
        default_handler_.reset();
    }

private:
    std::vector<std::shared_ptr<MessageHandler>> handlers_;
    std::shared_ptr<MessageHandler> default_handler_;
};

#endif  // MESSAGE_HANDLERS_H
