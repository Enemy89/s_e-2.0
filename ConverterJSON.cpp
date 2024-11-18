#include "ConverterJSON.h"

std::string ConverterJSON::getName() {
    return name;
}

int ConverterJSON::getTimeUpdate() {
    return timeUpdate;
}

std::vector<std::string> ConverterJSON::GetTextDocuments() {
    return files;
}

int ConverterJSON::GetResponsesLimit() const {
    return maxResponses;
}

bool ConverterJSON::loadConfig() {
    std::ifstream configFile("../config.json");

    if (!configFile.is_open()) {
        std::cerr << "Config file is missing." << std::endl;
        return false;
    }

    json configJson;
    try {
        configFile >> configJson;
    } catch (json::parse_error& e) {
        std::cerr << "Config file is empty or contains invalid JSON." << std::endl;
        return false;
    }

    if (configJson.find("config") == configJson.end()) {
        std::cerr << "Config file is empty." << std::endl;
        return false;
    }

    try {
        if (configJson["config"].contains("name")) {
            name = configJson["config"]["name"];
        } else {
            return false;
        }

        if (configJson["config"].contains("version")) {
            version = configJson["config"]["version"];
        } else {
            return false;
        }

        if (configJson["config"].contains("max_responses")) {
            maxResponses = configJson["config"]["max_responses"];
            if (maxResponses <= 0) {
                std::cerr << "Invalid max_responses in config.json. It must be a positive integer." << std::endl;
                return false;
            }
        } else {
            maxResponses = 5;
            configJson["config"]["max_responses"] = maxResponses;
        }

        if (version != VERSION_APP) {
            std::cerr << "Config.json has incorrect file version." << std::endl;
            return false;
        }

        if (configJson["config"].contains("time_update")) {
            timeUpdate = configJson["config"]["time_update"];
            if (timeUpdate <= 0) {
                std::cerr << "Invalid time_update in config.json. It must be a positive integer." << std::endl;
                return false;
            }
        } else {
            return false;
        }

        files.clear();
        std::string resourcesPath = "../resources";

        if (!fs::exists(resourcesPath) || !fs::is_directory(resourcesPath)) {
            std::cerr << "Error: Resources path does not exist or is not a directory." << std::endl;
            return false;
        } else {
            int nextDocID = 1;
            configJson["document_id"] = json::object();

            for (const auto& entry : fs::directory_iterator(resourcesPath)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = fs::relative(entry.path(), fs::current_path()).string();
                    std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
                    files.push_back(relativePath);

                    configJson["document_id"][relativePath] = nextDocID++;
                }
            }

            configJson["files"] = files;
        }

        std::ofstream outputConfigFile("../config.json");
        if (!outputConfigFile.is_open()) {
            std::cerr << "Error: Unable to write to config file." << std::endl;
            return false;
        }

        outputConfigFile << configJson.dump(4);
        outputConfigFile.close();

    } catch (json::type_error& e) {
        std::cerr << "Error reading configuration: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void ConverterJSON::saveIndex(const std::unordered_map<std::string, int>& termIdMap,
                              const std::unordered_map<int, std::vector<std::pair<int, int>>>& invertedIndex,
                              const std::unordered_map<int, std::unordered_map<int, std::vector<int>>>& positionalIndex) {
    json indexJson;

    // Создаем term_index
    json termIndexJson;
    json termToIdJson;
    json idToTermJson;

    for (const auto& [term, id] : termIdMap) {
        termToIdJson[term] = id;
        idToTermJson[std::to_string(id)] = term;
    }

    termIndexJson["term_to_id"] = termToIdJson;
    termIndexJson["id_to_term"] = idToTermJson;
    indexJson["term_index"] = termIndexJson;

    // Создаем inverted_index
    json invertedIndexJson;
    for (const auto& [termId, docList] : invertedIndex) {
        json docListJson;
        for (const auto& [documentId, frequency] : docList) {
            docListJson.push_back({{"document_id", documentId}, {"frequency", frequency}});
        }
        invertedIndexJson[std::to_string(termId)] = docListJson;
    }
    indexJson["inverted_index"] = invertedIndexJson;

    // Создаем positional_index
    json positionalIndexJson;
    for (const auto& [termId, docMap] : positionalIndex) {
        json docMapJson;
        for (const auto& [documentId, positions] : docMap) {
            docMapJson[std::to_string(documentId)] = {{"positions", positions}};
        }
        positionalIndexJson[std::to_string(termId)] = docMapJson;
    }
    indexJson["positional_index"] = positionalIndexJson;

    // Сохраняем индекс в index.json
    std::ofstream indexFile("../index.json");
    if (!indexFile.is_open()) {
        std::cerr << "Error: Unable to write to index file." << std::endl;
        return;
    }
    indexFile << indexJson.dump(4);
    indexFile.close();
}

//Преобразуем список запросов из JSON файла в вектор
std::vector<std::string> ConverterJSON::GetRequests() {
    std::vector<std::string> listRequests;
    objJson.clear();

    std::ifstream requestsFile("../requests.json");
    if (!requestsFile)
        std::cerr << "File requests.json not found."<<std::endl;
    requestsFile >> objJson;
    requestsFile.close();
    for (const auto &i: objJson["requests"])
        listRequests.push_back(i);
    return listRequests;
}

/*
 Преобразуем вектор с результатами поиска в JSON файл
void ConverterJSON::putAnswers(std::vector<std::vector<std::pair<int, float>>> answers)
{
    fileInput.open("../../answers.json", std::ios::out);
    objJson = {{"Answers:",{}}};
    for(int i = 0; i < answers.size(); ++i)
    {
        objJson["Answers:"]["request"+ std::to_string(i)+":"];
        if(answers[i].empty())
        {
            objJson["Answers:"]["request"+ std::to_string(i)+":"] ={{"result:", "false"}};
            continue;
        }
        else
            objJson["Answers:"]["request"+ std::to_string(i)+":"] ={{"result:", "true"}};
        for(int j = 0; j < answers[i].size(); ++j)
        {
            objJson["Answers:"]["request"+ std::to_string(i)+":"]["relevance:"].push_back({{"docid:", answers[i][j].first},{"rank:", ceil(answers[i][j].second*1000)/1000}});
        }
    }
    fileInput << objJson;
    fileInput.close();
}*/