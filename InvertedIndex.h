#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "ConverterJSON.h"

namespace fs = std::filesystem;

class InvertedIndex {
private:

public:
    InvertedIndex()=default;
    // Метод для создания/обновления базы индекса токенов
    void manageIndex(ConverterJSON& converter);
    // Метод для построения индекса
    void createIndex(ConverterJSON& converter);

private:
    //вспомогательные методы построения индекса
    void indexTokens(std::unordered_map<std::string, int>& termIdMap, std::vector<std::string>& tokens, int& nextTermId);
    void createInvertedIndex(std::unordered_map<int, std::vector<std::pair<int, int>>>& invertedIndex,
                             const std::unordered_map<std::string, int>& termIdMap,
                             const std::vector<std::string>& tokens,
                             int documentId);
    void createPositionalIndex(std::unordered_map<int, std::unordered_map<int, std::vector<int>>>& positionalIndex,
                               const std::unordered_map<std::string, int>& termIdMap,
                               const std::vector<std::string>& tokens,
                               int documentId);
};

#endif // INVERTEDINDEX_H
