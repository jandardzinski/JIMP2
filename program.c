#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define BUFFER_SIZE 8192
#define URL "http://localhost:1234/v1/chat/completions"  // adres serwera LM Studio

struct MemoryStruct {
    char *memory;
    size_t size;
};			// przechowywanie odpowiedzi HTTP zwroconych przez serwer LM Studio

// Funkcja do zapisu odpowiedzi
static size_t write_callback(void *contents, size_t size, size_t num, void *userp) 
{
    size_t total_size = size * num;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;			//rzutowanie aby moc uzyskac dostep do struktury

    char *ptr = realloc(mem->memory, mem->size + total_size + 1);
    if (ptr == NULL) 
	    return 0;  					// brak pamieci

    mem->memory = ptr;                                                      
    memcpy(&(mem->memory[mem->size]), contents, total_size);		//przypisanie do zmiennej memory pobranej odpowiedzi
    mem->size += total_size;						// uaktualnienie odpowiednio rozmiaru
    mem->memory[mem->size] = '\0';

    return total_size;						//zwraca rozmiar odpowiedzi 
}

void extract_response(char *json) 
{
    char *start = strstr(json, "content");  
    if (start) 
    {
        start = strchr(start, ':');  		// rozpoczecie od dwukropka 
        if (!start) 
	{
            printf("Nie znaleziono odpowiedzi modelu.\n");
            return;
        }
        start++;  						// przesuniecie wskaznika na pierwsza wartosc po ":"
        
       								 //pomijanie spacji oraz pierwszego cudzyslowia
        while (*start == ' ' || *start == '"') 
		start++;

        char *end = strchr(start, '"');  			// znajdowanie pierwszego cudzyslowia ktory zamyka odpowiedz modelu
        if (end) 
	{
            *end = '\0';  					// zamiana na `\0` co konczy danego stringa
        }

        // Zamiana `\n` na prawdziwe nowe linie
        for (char *p = start; *p; p++) 
	{
            if (*p == '\\' && *(p + 1) == 'n') 
	    {
                *p = '\n';
                memmove(p + 1, p + 2, strlen(p + 2) + 1);  // Przesuwamy resztę tekstu
            }
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

        // Usuń znak nowej linii na końcu
        prompt[strcspn(prompt, "\n")] = 0;

        if (strcmp(prompt, "exit") == 0) {
            printf("Zamykanie programu...\n");
            break;
        }

        char post_data[BUFFER_SIZE];

        // Formatowanie JSON
        snprintf(post_data, BUFFER_SIZE - 1,
                 "{"
                 "\"model\": \"llama-3-8b-gpt-4o-ru1.0\","
                 "\"messages\": [{\"role\": \"user\", \"content\": \"%.8000s\"}],"
                 "\"temperature\": 0.3,"
                 "\"top_p\": 1.0,"
                 "\"max_tokens\": 500"
                 "}", prompt);
        post_data[BUFFER_SIZE - 1] = '\0';  // Zakończenie stringa

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
