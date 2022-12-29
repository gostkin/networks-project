#ifndef API_H
#define API_H

#include "network_mode.h"
#include "logger.h"
#include "utils.h"

#include <optional>
#include <string>
#include <vector>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>

#include <Poco/URI.h>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

namespace tg {

namespace conversion_utils {
template <typename T>
inline std::optional<T> GetNullableValue(const Poco::JSON::Object &from, const std::string &field) {
    if (!from.has(field)) {
        return {};
    }
    Poco::Nullable<T> result = from.getNullableValue<T>(field);
    if (result.isNull()) {
        return {};
    }
    return result.value();
}
}  // namespace conversion_utils

struct TelegramCredentials {
    std::string telegram_token;
    std::string telegram_api_url;
};

struct TelegramApiUser {
    int64_t id;
    bool is_bot;
    std::string first_name;
    std::optional<std::string> last_name;
    std::optional<std::string> username;
    std::optional<std::string> language_code;
    std::optional<bool> can_join_groups;
    std::optional<bool> can_read_all_group_messages;
    std::optional<bool> supports_inline_queries;

    TelegramApiUser(const Poco::JSON::Object &from) {
        id = from.getValue<int64_t>("id");
        is_bot = from.getValue<bool>("is_bot");
        first_name = from.getValue<std::string>("first_name");
        last_name = conversion_utils::GetNullableValue<std::string>(from, "last_name");
        username = conversion_utils::GetNullableValue<std::string>(from, "username");
        can_join_groups = conversion_utils::GetNullableValue<bool>(from, "can_join_groups");
        can_read_all_group_messages =
            conversion_utils::GetNullableValue<bool>(from, "can_read_all_group_messages");
        supports_inline_queries =
            conversion_utils::GetNullableValue<bool>(from, "supports_inline_queries");
    }
};

struct TelegramApiMessageEntity {
    std::string type;
    int64_t offset;
    int64_t length;

    TelegramApiMessageEntity(const Poco::JSON::Object &from) {
        type = from.getValue<std::string>("type");
        offset = from.getValue<int64_t>("offset");
        length = from.getValue<int64_t>("length");
    }
};

struct TelegramApiChat {
    int64_t id;
    std::string type;
    std::optional<std::string> username;
    std::optional<std::string> first_name;

    TelegramApiChat(const Poco::JSON::Object &from) {
        type = from.getValue<std::string>("type");
        username = conversion_utils::GetNullableValue<std::string>(from, "username");
        first_name = conversion_utils::GetNullableValue<std::string>(from, "first_name");
        id = from.getValue<int64_t>("id");
    }
};

struct TelegramApiMessage {
    int64_t message_id;
    std::optional<TelegramApiUser> from;
    int64_t date;
    std::vector<TelegramApiMessageEntity> entities;
    std::optional<TelegramApiChat> chat;
    std::optional<std::string> text;

    TelegramApiMessage(const Poco::JSON::Object &from) : entities{} {
        text = conversion_utils::GetNullableValue<std::string>(from, "text");
        message_id = from.getValue<int64_t>("message_id");
        date = from.getValue<int64_t>("date");
        chat = TelegramApiChat(*from.getObject("chat"));

        if (from.has("entities")) {
            Poco::JSON::Array::Ptr entity_obj = from.getArray("entities");
            for (size_t i = 0; i != entity_obj->size(); ++i) {
                entities.emplace_back(*entity_obj->getObject(i));
            }
        }
    }

    std::string GetTextOrEmpty() const {
        if (text.has_value()) {
            return text.value();
        }
        return {};
    }
};

struct TelegramUpdate {
    std::optional<TelegramApiMessage> message;
    int64_t update_id;

    TelegramUpdate(const Poco::JSON::Object &from) {
        message = TelegramApiMessage(*from.getObject("message"));
        update_id = from.getValue<int64_t>("update_id");
    }

    TelegramUpdate(const TelegramUpdate &from) : message{from.message}, update_id{from.update_id} {
    }

    std::string GetMessageTextOrEmpty() const {
        if (message.has_value()) {
            return message->GetTextOrEmpty();
        }
        return {};
    }
};

class TelegramApiError : public std::runtime_error {
public:
    TelegramApiError(int http_code_p, const std::string &details_p)
        : std::runtime_error("telegram api error: code=" + std::to_string(http_code_p) +
                             " details=" + details_p),
          details_(details_p) {
    }

private:
    std::string details_;
};

class TelegramApi {
public:
    TelegramApi(const TelegramCredentials &credentials, NetworkMode mode = NetworkMode::HTTP)
        : credentials_{credentials},
          mode_{mode},
          logger_{logger::LoggerFactory::GetStdoutLogger()} {
    }

    TelegramApiUser GetMe() {
        logger_->LogInfo("GetMe request...");
        auto uri = GetURI("getMe");
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.getPath(),
                                       Poco::Net::HTTPMessage::HTTP_1_1);

        auto reply = SendRequestAndGetReply(uri, request);

        auto result = reply.extract<Poco::JSON::Object::Ptr>()->getObject("result");

        return TelegramApiUser(*result);
    }

    std::vector<TelegramUpdate> GetUpdates(std::optional<int64_t> offset = {},
                                           std::optional<int64_t> timeout = {}) {
        logger_->LogInfo("Getting updates with offset: " + GetString(offset) + "...");

        auto uri = GetURI("getUpdates");
        if (timeout) {
            uri.addQueryParameter("timeout", std::to_string(timeout.value()));
        }
        if (offset) {
            uri.addQueryParameter("offset", std::to_string(offset.value()));
        }
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.getPathAndQuery(),
                                       Poco::Net::HTTPMessage::HTTP_1_1);

        auto reply = SendRequestAndGetReply(uri, request);

        Poco::JSON::Array::Ptr updates_array =
            reply.extract<Poco::JSON::Object::Ptr>()->getArray("result");
        std::vector<TelegramUpdate> updates;
        for (size_t obj_index = 0; obj_index != updates_array->size(); ++obj_index) {
            updates.emplace_back(*updates_array->getObject(obj_index));
        }

        logger_->LogInfo("Got " + std::to_string(updates.size()) + " updates");

        return updates;
    }

    TelegramApiMessage SendMessage(int64_t chat_id, const std::string &message) {
        Poco::JSON::Object obj;
        logger_->LogInfo("Sending message: " + message + " to: " + std::to_string(chat_id) + "...");
        obj.set("chat_id", chat_id);
        obj.set("text", message);
        std::stringstream body_to_send_stream;
        obj.stringify(body_to_send_stream);
        auto body_to_send = body_to_send_stream.str();

        return SendMessageWithBody(body_to_send);
    }

    TelegramApiMessage SendMessage(int64_t chat_id, const std::string &message,
                                   int64_t reply_to_message_id) {
        logger_->LogInfo("Sending reply message: " + message + " to: " + std::to_string(chat_id) +
                         " on: " + std::to_string(reply_to_message_id) + "...");
        Poco::JSON::Object obj;
        obj.set("chat_id", chat_id);
        obj.set("text", message);
        obj.set("reply_to_message_id", reply_to_message_id);
        std::stringstream body_to_send_stream;
        obj.stringify(body_to_send_stream);
        auto body_to_send = body_to_send_stream.str();

        return SendMessageWithBody(body_to_send);
    }

private:
    TelegramApiMessage SendMessageWithBody(const std::string &body_to_send) {
        auto uri = GetURI("sendMessage");
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, uri.getPath(),
                                       Poco::Net::HTTPMessage::HTTP_1_1);

        request.setContentLength(body_to_send.size());
        request.setContentType("application/json");

        auto session = GetSession(uri);

        session->sendRequest(request) << body_to_send;

        Poco::Net::HTTPResponse response;
        auto &body = session->receiveResponse(response);

        if (response.getStatus() / 100 != 2) {
            throw TelegramApiError(response.getStatus(), "sendMessage error");
        }

        Poco::JSON::Parser parser;
        auto reply = parser.parse(body);

        auto result = reply.extract<Poco::JSON::Object::Ptr>()->getObject("result");
        return TelegramApiMessage(*result);
    }

    Poco::Dynamic::Var SendRequestAndGetReply(const Poco::URI &uri,
                                              Poco::Net::HTTPRequest &request) {
        auto session = GetSession(uri);

        session->sendRequest(request);

        Poco::Net::HTTPResponse response;
        auto &body = session->receiveResponse(response);

        if (response.getStatus() / 100 != 2) {
            throw TelegramApiError(response.getStatus(), "getMe error");
        }

        Poco::JSON::Parser parser;
        auto reply = parser.parse(body);
        return reply;
    }

    Poco::URI GetURI(std::string path) {
        Poco::URI uri(credentials_.telegram_api_url + "bot" + credentials_.telegram_token + "/" +
                      path);
        return uri;
    }

    std::unique_ptr<Poco::Net::HTTPClientSession> GetSession(const Poco::URI &uri) {
        if (mode_ == NetworkMode::HTTP) {
            return std::make_unique<Poco::Net::HTTPClientSession>(uri.getHost(), uri.getPort());
        }
        return std::unique_ptr<Poco::Net::HTTPClientSession>(
            std::make_unique<Poco::Net::HTTPSClientSession>(uri.getHost(), uri.getPort())
                .release());
    }

    std::string OffsetToString(std::optional<int64_t> current) const {
        return current.has_value() ? std::to_string(current.value()) : "none";
    }

private:
    TelegramCredentials credentials_;
    const NetworkMode mode_;
    std::shared_ptr<logger::Logger> logger_;
};

}  // namespace tg

#endif  // API_H
