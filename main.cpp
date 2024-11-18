#include <iostream>
#include "SearchServer.h"
#include "InvertedIndex.h"
#include "ConverterJSON.h"


int main() {
    ConverterJSON converterJson;
    InvertedIndex invertedIndex;
    SearchServer searchServer;

    if (!converterJson.loadConfig()) {
        std::cerr << "Failed to load configuration." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "Starting "<< converterJson.getName() << std::endl;
    invertedIndex.manageIndex(converterJson);

    // Получение списка запросов из JSON
    std::vector<std::string> listRequests = converterJson.GetRequests();
    // Обработка запросов и получение результата
    std::vector<std::vector<std::string>> processedRequests = searchServer.processRequests(listRequests);


    return 0;
}
