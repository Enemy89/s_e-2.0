#include "SearchServer.h"
#include "ConverterJSON.h"

// Метод для токенизации текста
std::vector<std::string> SearchServer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream allText(text);
    std::string word;

    while (allText >> word) {
        // Удаление пунктуации в начале и конце каждого слова
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        tokens.push_back(word);
    }
    return tokens;
}

// Метод для приведения слов к нижнему регистру
void SearchServer::toLowercase(std::vector<std::string>& tokens) {
    for (auto& word : tokens) {
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    }
}

// Метод удаления стоп-слов
void SearchServer::removeStopWords(std::vector<std::string>& tokens) {
    tokens.erase(std::remove_if(tokens.begin(), tokens.end(), [this](const std::string& word) {
        return stopWords.find(word) != stopWords.end();
    }), tokens.end());
}

// предварительная обработка запросов
std::vector<std::vector<std::string>> SearchServer::processRequests(std::vector<std::string>& listRequests) {
    // Ограничение размера вектора до 1000
    if (listRequests.size() > 1000) {
        listRequests.resize(1000);
    }

    // Результирующий список обработанных запросов
    std::vector<std::vector<std::string>> processedRequests;

    // Обработка каждого запроса
    for (auto& request : listRequests) {
        // Токенизация строки
        std::vector<std::string> tokens = tokenize(request);

        // Проверка количества слов до удаления стоп-слов
        if (tokens.size() < 1 || tokens.size() > 10) {
            continue; // Пропустить запись
        }

        // Удаление стоп-слов
        removeStopWords(tokens);

        // Добавление обработанного запроса в итоговый список
        processedRequests.push_back(tokens);
    }
    return processedRequests;
}


// Функция для поиска ID токена в базе индексов
int SearchServer::findTermID(const json& indexJson, const std::string& token) {
    if (indexJson.contains("term_index") && indexJson["term_index"].contains("term_to_id")) {
        const auto& termToID = indexJson["term_index"]["term_to_id"];
        auto it = termToID.find(token);
        if (it != termToID.end()) {
            return it.value(); // Возвращаем ID терма
        }
    }
    return 0; // Возвращаем 0, если терм не найден
}

// Выясняет для каждого токена запроса в каких док айди они содержатся
SearchServer::RequestData SearchServer::findDocumentIdsForTokens() {
    json requestsJson;
    json indexJson;
    RequestData requestDocIds;

    // Загрузка данных из файлов requests.json и index.json
    std::ifstream requestsFile("../requests.json");
    if (requestsFile.is_open()) {
        if (requestsFile.peek() == std::ifstream::traits_type::eof()) {
            std::cerr << "Error: requests.json is empty" << std::endl;
            return {};
        }
        requestsFile >> requestsJson;
        requestsFile.close();
    } else {
        std::cerr << "Error: Unable to open requests.json" << std::endl;
        return {};
    }

    std::ifstream indexFile("../index.json");
    if (indexFile.is_open()) {
        if (indexFile.peek() == std::ifstream::traits_type::eof()) {
            std::cerr << "Error: index.json is empty" << std::endl;
            return {};
        }
        indexFile >> indexJson;
        indexFile.close();
    } else {
        std::cerr << "Error: Unable to open index.json" << std::endl;
        return {};
    }

    // Создание и сортировка вектора для каждого токена
    for (const auto& [requestKey, tokens] : requestsJson.items()) {
        std::vector<std::vector<int>> docIdList;

        for (const auto& token : tokens) {
            int tokenId = token["id"];
            std::vector<int> documentIds;

            if (tokenId == 0) {
                continue;
            }

            // Ищем tokenId в positional_index и собираем id документов
            if (indexJson.contains("positional_index") &&
                indexJson["positional_index"].contains(std::to_string(tokenId))) {

                for (const auto& [docId, positionInfo] : indexJson["positional_index"][std::to_string(tokenId)].items()) {
                    documentIds.push_back(std::stoi(docId));
                }

                // Сортируем documentIds по возрастанию
                std::sort(documentIds.begin(), documentIds.end());
            }

            docIdList.push_back(documentIds);
        }

        requestDocIds[requestKey] = docIdList;
    }

    return requestDocIds;
}

// Считает общие док айди токенов из запроса
void SearchServer::countDocumentMatches(const std::unordered_map<std::string, std::vector<std::vector<int>>>& requestData) {
    for (const auto& [requestKey, docIdLists] : requestData) {
        std::cout << "Processing " << requestKey << ":\n";
        if (docIdLists.size() <= 1) {
            std::cout << "No matches to count." << std::endl;
            continue;
        }

        std::unordered_map<int, int> docCount;

        // Подсчитываем вхождения каждого id документа
        for (const auto& docIds : docIdLists) {
            for (int docId : docIds) {
                docCount[docId]++;
            }
        }

        // Переносим данные из docCount в вектор пар для сортировки
        std::vector<std::pair<int, int>> sortedDocCount(docCount.begin(), docCount.end());

        // Сортируем по убыванию количества совпадений
        std::sort(sortedDocCount.begin(), sortedDocCount.end(),
                  [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                      return a.second > b.second;
                  });

        // Выводим отсортированный результат
        for (const auto& [docId, count] : sortedDocCount) {
            std::cout << "Document ID: " << docId << " - Matches: " << count << "\n";
        }
    }
}

// Подсчет позиций токенов в документах
void SearchServer::calculatePositionDifference(const json& requestsJson, const json& indexJson) {
    for (const auto& [requestKey, tokens] : requestsJson.items()) {
        if (tokens.size() < 2) {
            continue;
        }

        std::cout << "Processing request for: " << requestKey << std::endl;

        for (size_t i = 0; i < tokens.size(); ++i) {
            int tokenId = tokens[i]["id"];
            if (tokenId == 0) continue;

            // Ищем позицию токенов в positional_index
            if (indexJson.contains("positional_index") &&
                indexJson["positional_index"].contains(std::to_string(tokenId))) {

                for (const auto& [docId, positions] : indexJson["positional_index"][std::to_string(tokenId)].items()) {
                    std::cout << "Token " << tokenId << " in doc " << docId << " positions: ";
                    for (const auto& position : positions) {
                        std::cout << position << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
}

// Метод для обработки запросов
void SearchServer::processQueries() {
    // handleUserQueryAndSave();

    auto requestData = findDocumentIdsForTokens();
    countDocumentMatches(requestData);

    json requestsJson;
    json indexJson;

    std::ifstream requestsFile("../requests.json");
    if (requestsFile.is_open()) {
        requestsFile >> requestsJson;
        requestsFile.close();
    }

    std::ifstream indexFile("../index.json");
    if (indexFile.is_open()) {
        indexFile >> indexJson;
        indexFile.close();
    }

    calculatePositionDifference(requestsJson, indexJson);
}