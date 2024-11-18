#ifndef SEARCH_SERVER_H
#define SEARCH_SERVER_H

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <climits> // Для INT_MAX
#include <memory>
#include <thread>
#include <mutex>
#include <future>

using json = nlohmann::json;

class SearchServer {
private:
    std::mutex mutex;
    using RequestData = std::unordered_map<std::string, std::vector<std::vector<int>>>;
    std::unordered_set<std::string> stopWords = {"the", "is", "at", "which", "on", "in", "and", "a", "to", "ah"};
    void countDocumentMatches(const std::unordered_map<std::string, std::vector<std::vector<int>>>& requestData);
    void calculatePositionDifference(const json& requestsJson, const json& indexJson);

public:
    SearchServer() = default;
    std::vector<std::vector<std::string>> processRequests(std::vector<std::string>& listRequests);
    int findTermID(const json& indexJson, const std::string& token);
    void toLowercase(std::vector<std::string>& tokens);
    void removeStopWords(std::vector<std::string>& tokens);
    std::vector<std::string> tokenize(const std::string& text);
    void processQueries();
    RequestData findDocumentIdsForTokens();
};

#endif // SEARCH_SERVER_H
