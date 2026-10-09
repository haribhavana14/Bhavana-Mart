#include "ChatService.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

#ifdef BHAVANAMART_HAS_CURL
#include <curl/curl.h>
#endif

namespace
{
std::string NormalizeQuestion(const std::string& input)
{
    std::string output;
    bool pending_space = false;

    for (unsigned char ch : input)
    {
        if (std::isspace(ch))
        {
            pending_space = !output.empty();
            continue;
        }

        if (pending_space)
            output.push_back(' ');

        pending_space = false;
        output.push_back(static_cast<char>(std::tolower(ch)));
    }

    return output;
}

std::string StaticProductReply(const std::string& question)
{
    if (question.find("search") != std::string::npos ||
        question.find("category") != std::string::npos ||
        question.find("filter") != std::string::npos)
    {
        return "On the Products page, enter a keyword to search, or choose a category to narrow the listings.";
    }

    if (question.find("cart") != std::string::npos ||
        question.find("quantity") != std::string::npos ||
        question.find("remove") != std::string::npos)
    {
        return "Add a listed product to your cart. You can change its quantity or remove it; the cart shows each subtotal and the running total.";
    }

    if (question.find("checkout") != std::string::npos ||
        question.find("payment") != std::string::npos)
    {
        return "Checkout uses a simulated mock payment confirmation. It does not charge real money.";
    }

    if (question.find("order") != std::string::npos ||
        question.find("deliver") != std::string::npos ||
        question.find("shipping") != std::string::npos ||
        question.find("status") != std::string::npos)
    {
        return "Buyers can check Order History. Sellers can view incoming orders and move eligible orders from Confirmed to Shipped to Delivered.";
    }

    if (question.find("review") != std::string::npos ||
        question.find("rating") != std::string::npos ||
        question.find("star") != std::string::npos)
    {
        return "A buyer can submit one 1–5 star rating and review for a product after the relevant order is marked Delivered.";
    }

    if (question.find("seller") != std::string::npos ||
        question.find("listing") != std::string::npos)
    {
        return "Sellers can create, update, and remove their own listings, including name, description, price, stock, category and image URL.";
    }

    if (question.find("price") != std::string::npos ||
        question.find("stock") != std::string::npos ||
        question.find("available") != std::string::npos)
    {
        return "Check the product listing for its displayed price and stock. I cannot verify changing stock or pricing from the chat alone.";
    }

    if (question.find("product") != std::string::npos ||
        question.find("browse") != std::string::npos)
    {
        return "Open Products to browse listings. Use keyword search and category filtering to find items.";
    }

    return "Hi! I'm the BhavanaMart shopping assistant. I can help with product listings, search and categories, cart, mock checkout, orders, sellers, and reviews. What would you like to know?";
}

#ifdef BHAVANAMART_HAS_CURL
std::size_t AppendHttpResponse(char* data, std::size_t size,
                               std::size_t count, void* destination)
{
    const std::size_t bytes = size * count;
    static_cast<std::string*>(destination)->append(data, bytes);
    return bytes;
}

bool CallLanguageModel(const std::string& api_key,
                       const std::string& question,
                       std::string& reply)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    Json::Value payload;
    payload["model"] = "gpt-4o-mini";
    payload["max_tokens"] = 220;
    payload["temperature"] = 0.2;

    Json::Value system_message;
    system_message["role"] = "system";
    system_message["content"] =
        "You are the BhavanaMart shopping assistant. Answer only questions "
        "about product listings and the BhavanaMart shopping workflow: browsing, "
        "keyword/category search, displayed prices and stock, cart, simulated "
        "checkout, orders, seller listings, delivery status, reviews and ratings. "
        "Use concise, friendly answers under 100 words. Never invent live products, "
        "prices, inventory, users or order records. If a question requires a live "
        "database lookup, direct the user to the corresponding page. For unrelated "
        "topics, politely say you only help with BhavanaMart shopping.";

    Json::Value user_message;
    user_message["role"] = "user";
    user_message["content"] = question;
    payload["messages"].append(system_message);
    payload["messages"].append(user_message);

    const std::string request_body = payload.toStyledString();
    std::string response_body;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    const std::string authorization = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, authorization.c_str());

    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     static_cast<long>(request_body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AppendHttpResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 3000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 8000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    const CURLcode result = curl_easy_perform(curl);
    long http_status = 0;
    if (result == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK || http_status < 200 || http_status >= 300)
        return false;

    Json::Value response_json;
    Json::Reader reader;
    if (!reader.parse(response_body, response_json))
        return false;

    const Json::Value& content =
        response_json["choices"][0]["message"]["content"];

    if (!content.isString() || content.asString().empty())
        return false;

    reply = content.asString();
    return true;
}
#endif
} // namespace

ChatService::ChatService()
{
    // Read the secret once at process startup. It never goes to the browser.
    const char* configured_key = std::getenv("OPENAI_API_KEY");
    if (configured_key != nullptr)
        api_key_ = configured_key;

#ifdef BHAVANAMART_HAS_CURL
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        api_key_.clear();
#endif
}

ChatReply ChatService::Ask(const std::string& session_id,
                           const std::string& message)
{
    const std::string question_key = NormalizeQuestion(message);
    if (question_key.empty() || message.size() > 1000)
    {
        return {
            "Please enter a product-related question with up to 1000 characters.",
            false,
            true
        };
    }

    const std::string safe_session =
        session_id.empty() ? "anonymous" : session_id;
    const auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& state = sessions_[safe_session];

        while (!state.message_times.empty() &&
               now - state.message_times.front() >= std::chrono::minutes(1))
        {
            state.message_times.pop_front();
        }

        if (state.message_times.size() >= 10)
        {
            return {
                "You've reached the limit of 10 messages per minute. Please wait a little and try again.",
                true,
                false
            };
        }

        state.message_times.push_back(now);

        const auto cached = state.cache.find(question_key);
        if (cached != state.cache.end())
            return {cached->second, false, api_key_.empty()};
    }

    std::string reply;
    bool degraded = true;

#ifdef BHAVANAMART_HAS_CURL
    if (!api_key_.empty())
        degraded = !CallLanguageModel(api_key_, message, reply);
#endif

    if (degraded || reply.empty())
        reply = StaticProductReply(question_key);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& cache = sessions_[safe_session].cache;

        if (cache.size() >= 100)
            cache.clear();

        cache[question_key] = reply;
    }

    return {reply, false, degraded};
}