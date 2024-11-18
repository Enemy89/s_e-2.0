#ifndef CONVERTERJSON_H
#define CONVERTERJSON_H

#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <algorithm>
#include <memory>

namespace fs = std::filesystem;
using json = nlohmann::json;

class ConverterJSON {
private:
    std::string name;
    std::string version;
    int maxResponses;
    int timeUpdate;
    std::vector<std::string> files;
    nlohmann::json objJson;

public:
    ConverterJSON() = default;
    //значение имени движка из config
    std::string getName();
    //значение времени обновления из config
    int getTimeUpdate();
    //список документов для поиска
    std::vector<std::string> GetTextDocuments();
    //максимальное количество ответов на один запрос
    int GetResponsesLimit() const;
    //список запросов
    std::vector<std::string> GetRequests();
    /*Получаем вектор с данными по релеватности документов каждому запросу*/
    void putAnswers(std::vector<std::vector<std::pair<int, float>>>answers);

    // Вспомогательные методы (проверка и парсинг config)
    bool loadConfig();
    //создание базы термов
    void saveIndex(const std::unordered_map<std::string, int>& termIdMap,
                   const std::unordered_map<int, std::vector<std::pair<int, int>>>& invertedIndex,
                   const std::unordered_map<int, std::unordered_map<int, std::vector<int>>>& positionalIndex);
};

#endif // CONVERTERJSON_H
