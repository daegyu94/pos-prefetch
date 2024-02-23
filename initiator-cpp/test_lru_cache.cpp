#include <iostream>

#include "lru_cache.h"

using namespace std;

int main() {
    LRUCache lru_cache(3);
    
    // Insert some data into the cache
    lru_cache.Put({1, 100}, {1024, 0});
    lru_cache.Put({2, 200}, {2048, 0});
    lru_cache.Put({3, 300}, {3072, 0});

    // Display the initial state of the cache
    cout << "Initial cache state:" << endl;
    lru_cache.Display();

    // Retrieve some data from the cache
    cout << "Get data with key {2, 200}:" << endl;
    Value_t result = lru_cache.Get({2, 200});
    cout << "Retrieved data: {" << result.file_size << ", " << result.extent_tree << "}" << endl;
    lru_cache.Display();
    
    // Add more data to exceed the cache capacity and observe eviction
    cout << "Add more data to exceed capacity:" << endl;
    lru_cache.Put({4, 400}, {4096, 0});
    lru_cache.Display();

    // Delete data from the cache
    cout << "Delete data with key {3, 300}:" << endl;
    lru_cache.Delete({3, 300});
    lru_cache.Display();

    return 0; 
    return 0;
}
