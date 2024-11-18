#include "InvertedIndex.h"
#include "ConverterJSON.h"
#include "SearchServer.h"
#include <filesystem>
namespace fs = std::filesystem;


void InvertedIndex::manageIndex(ConverterJSON& converter) {
    std::string indexFilePath = "../index.json";
    bool indexExists = fs::exists(indexFilePath);

    // если файл базы существует, то проверяем не пора ли обновить
    if (indexExists) {
        // Получаем текущее время
        auto currentTime = std::chrono::system_clock::now();

        // Получаем время последнего изменения файла
        auto lastWriteTime = fs::last_write_time(indexFilePath);

        // Преобразуем в time_t
        std::time_t current_time_t = std::chrono::system_clock::to_time_t(currentTime);
        std::time_t last_write_time_t = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now() - (fs::file_time_type::clock::now() - lastWriteTime));

        // Вычисляем разницу в секундах
        double elapsedSeconds = std::difftime(current_time_t, last_write_time_t);

        // Сравниваем с интервалом времени (TimeUpdate дб в кофиге в секундах)
        if (elapsedSeconds >= converter.getTimeUpdate()) {  // Используем converter для вызова getTimeUpdate
            createIndex(converter);
        }
    } else {
        // Если индекс не существует, создаем новый индекс
        createIndex(converter);
    }
}

// Метод создания id токенов
void InvertedIndex::indexTokens(std::unordered_map<std::string, int>& termIdMap, std::vector<std::string>& tokens, int& nextTermId) {
    for (const auto& word : tokens) {
        if (termIdMap.find(word) == termIdMap.end()) {
            termIdMap[word] = nextTermId++;
        }
    }
}

// Метод создания инвертированного индекса (частота появления в документах)
void InvertedIndex::createInvertedIndex(std::unordered_map<int, std::vector<std::pair<int, int>>>& invertedIndex,
                                        const std::unordered_map<std::string, int>& termIdMap,
                                        const std::vector<std::string>& tokens,
                                        int documentId) {
    for (const auto& word : tokens) {
        int termId = termIdMap.at(word);

        auto& docList = invertedIndex[termId];
        auto it = std::find_if(docList.begin(), docList.end(), [&](const std::pair<int, int>& p) {
            return p.first == documentId;
        });

        if (it != docList.end()) {
            it->second++;  // Увеличиваем частоту
        } else {
            docList.emplace_back(documentId, 1);  // Новый документ, частота = 1
        }
    }
}

// Метод создания позиционного индекса
void InvertedIndex::createPositionalIndex(std::unordered_map<int, std::unordered_map<int, std::vector<int>>>& positionalIndex,
                                          const std::unordered_map<std::string, int>& termIdMap,
                                          const std::vector<std::string>& tokens,
                                          int documentId) {
    for (int i = 0; i < tokens.size(); ++i) {
        const std::string& word = tokens[i];
        int termId = termIdMap.at(word);

        positionalIndex[termId][documentId].push_back(i);  // Добавляем позицию
    }
}

// Метод для построения индекса основной
void InvertedIndex::createIndex(ConverterJSON& converter) {
    SearchServer searchServer;
    std::unordered_map<std::string, int> termIdMap;
    int nextTermId = 1;  // Следующий ID для нового терма

    // Инвертированный и позиционный индексы
    std::unordered_map<int, std::vector<std::pair<int, int>>> invertedIndex;
    std::unordered_map<int, std::unordered_map<int, std::vector<int>>> positionalIndex;

    // Мапа для сопоставления документов и их ID
    std::unordered_map<std::string, int> documentIdMap;

    // Загружаем document_id из config.json
    std::ifstream configFile("../config.json");
    if (configFile.is_open()) {
        json configJson;
        configFile >> configJson;
        for (const auto& [filePath, docID] : configJson["document_id"].items()) {
            documentIdMap[filePath] = docID.get<int>();
        }
        configFile.close();
    }

    // Мьютексы для защиты общих данных
    std::mutex termIdMapMutex;
    std::mutex invertedIndexMutex;
    std::mutex positionalIndexMutex;

    // Вектор потоков
    std::vector<std::thread> threads;

    // Обработка каждого файла в отдельном потоке
    for (const auto& filePath : converter.GetTextDocuments()) {
        threads.emplace_back([&, filePath]() {
            std::ifstream inputFile(filePath);
            if (!inputFile.is_open()) {
                std::cerr << "Error: Unable to open file " << filePath << std::endl;
                return;
            }

            std::string content((std::istreambuf_iterator<char>(inputFile)),
                                (std::istreambuf_iterator<char>()));
            inputFile.close();

            // Токенизация текста
            std::vector<std::string> tokens = searchServer.tokenize(content);

            // Приведение к нижнему регистру
            searchServer.toLowercase(tokens);

            // Удаление стоп-слов
            searchServer.removeStopWords(tokens);

            // Получение document_id
            int documentId = documentIdMap[filePath];

            // Локальные структуры данных для текущего потока
            std::unordered_map<std::string, int> localTermIdMap;
            std::unordered_map<int, std::vector<std::pair<int, int>>> localInvertedIndex;
            std::unordered_map<int, std::unordered_map<int, std::vector<int>>> localPositionalIndex;

            // Индексация токенов
            int localNextTermId = nextTermId;
            indexTokens(localTermIdMap, tokens, localNextTermId);

            // Создание локального инвертированного индекса
            createInvertedIndex(localInvertedIndex, localTermIdMap, tokens, documentId);

            // Создание локального позиционного индекса
            createPositionalIndex(localPositionalIndex, localTermIdMap, tokens, documentId);

            // Слияние локальных данных с глобальными
            {
                std::lock_guard<std::mutex> lock(termIdMapMutex);
                for (const auto& [term, id] : localTermIdMap) {
                    if (termIdMap.find(term) == termIdMap.end()) {
                        termIdMap[term] = nextTermId++;
                    }
                }
            }

            {
                std::lock_guard<std::mutex> lock(invertedIndexMutex);
                for (const auto& [termId, docList] : localInvertedIndex) {
                    invertedIndex[termId].insert(invertedIndex[termId].end(), docList.begin(), docList.end());
                }
            }

            {
                std::lock_guard<std::mutex> lock(positionalIndexMutex);
                for (const auto& [termId, docPositions] : localPositionalIndex) {
                    for (const auto& [docId, positions] : docPositions) {
                        positionalIndex[termId][docId].insert(
                                positionalIndex[termId][docId].end(),
                                positions.begin(),
                                positions.end()
                        );
                    }
                }
            }
        });
    }

    // Ожидание завершения всех потоков
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Сохранение индексов (основной, инвертированный, позиционный)
    converter.saveIndex(termIdMap, invertedIndex, positionalIndex);
}
