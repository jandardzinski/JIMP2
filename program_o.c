#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define BUFFER_SIZE 8192
#define URL "http://localhost:11434/api/generate"  // Adres lokalnego serwera Ollama

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t num, void *userp) 
{
    size_t total_size = size * num;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + total_size + 1);
    if (ptr == NULL) 
        return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, total_size);
    mem->size += total_size;
    mem->memory[mem->size] = '\0';

    return total_size;
}

void extract_response(char *json) 
{
    char *start = strstr(json, "response");
    if (start) 
    {
        start = strchr(start, ':');
        if (!start) 
        {
            printf("Nie znaleziono odpowiedzi modelu.\n");
            return;
        }
        start++;
        
        while (*start == ' ' || *start == '"') 
            start++;

        char *end = strchr(start, '"');
        if (end) 
        {
            *end = '\0';
        }

        printf("\n Odpowiedz modelu:\n%s\n", start);
    } else {
        printf("Blad: Nie znaleziono odpowiedzi w JSON-ie.\n");
    }
}

int main() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "Błąd inicjalizacji cURL\n");
        return 1;
    }

    while (1) {
        char prompt[BUFFER_SIZE];

        printf("\n Twoje pytanie (wpisz 'exit' aby zakończyć): ");
        fgets(prompt, BUFFER_SIZE, stdin);

        prompt[strcspn(prompt, "\n")] = 0;

        if (strcmp(prompt, "exit") == 0) {
            printf("Zamykanie programu...\n");
            break;
        }

        char post_data[BUFFER_SIZE];

        snprintf(post_data, BUFFER_SIZE - 1,
                 "{"
                 "\"model\": \"mistral\","  // Możesz zmienić model np. na llama3
                 "\"prompt\": \"%.8000s\"," 
                 "\"stream\": false"
                 "}", prompt);
        post_data[BUFFER_SIZE - 1] = '\0';

        chunk.size = 0;
        free(chunk.memory);
        chunk.memory = malloc(1);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, URL);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Błąd cURL: %s\n", curl_easy_strerror(res));
        } else {
            extract_response(chunk.memory);
        }

        curl_slist_free_all(headers);
    }

    curl_easy_cleanup(curl);
    free(chunk.memory);
    curl_global_cleanup();

    return 0;
}
