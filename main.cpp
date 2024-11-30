#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <queue>
#include <vector>

// Configurações gerais
int buffer_capacity;
int print_time; // Tempo de impressão por página (milissegundos)

// Semáforos e mutex
sem_t semEmpty;
sem_t semFull;
pthread_mutex_t mutexBuffer;

typedef struct {
    char document_name[50];
    int num_pages;
    int priority;
    time_t request_time;
    time_t print_time;
    int process_id;
    int printer_id; // Identificador da impressora
} Document;

// Comparador para a fila de prioridade
struct ComparePriority {
    bool operator()(const Document& a, const Document& b) {
        return a.priority < b.priority; // Maior prioridade no topo
    }
};

// Buffer como fila de prioridade
std::priority_queue<Document, std::vector<Document>, ComparePriority> buffer;

// Contadores para relatório
int documents_printed = 0;
std::vector<int> total_pages_printed;
std::vector<Document> printed_documents;

void* producer(void* args) {
    while (1) {
        // Criar novo documento
        sleep(1);
        Document doc;
        snprintf(doc.document_name, sizeof(doc.document_name), "Documento-%d", rand() % 1000);
        doc.num_pages = rand() % 10 + 1;   // Número de páginas entre 1 e 10
        doc.priority = rand() % 5 + 1;    // Prioridade entre 1 e 5
        doc.request_time = time(NULL);
        doc.process_id = rand() % 5 + 1;  // Processo solicitante (entre 1 e 5)

        sem_wait(&semEmpty);
        pthread_mutex_lock(&mutexBuffer);

        // Adiciona documento ao buffer
        buffer.push(doc);
        printf("Produtor criou: %s, Prioridade: %d, Buffer atual: %lu\n", doc.document_name, doc.priority, buffer.size());

        pthread_mutex_unlock(&mutexBuffer);
        sem_post(&semFull);
    }
}

void* consumer(void* args) {
    int printer_id = *((int*)args);
    while (1) {
        sleep(1);
        sem_wait(&semFull);
        pthread_mutex_lock(&mutexBuffer);

        // Remover documento com maior prioridade
        Document doc_to_print = buffer.top();
        buffer.pop();

        pthread_mutex_unlock(&mutexBuffer);
        sem_post(&semEmpty);

        // Simular tempo de impressão
        usleep(doc_to_print.num_pages * print_time * 1000); // Tempo proporcional ao número de páginas
        doc_to_print.print_time = time(NULL);
        doc_to_print.printer_id = printer_id;

        // Atualizar relatórios
        documents_printed++;
        total_pages_printed[printer_id] += doc_to_print.num_pages;
        printed_documents.push_back(doc_to_print);

        printf("Impressora %d imprimiu: %s, Prioridade: %d, Páginas: %d\n", printer_id, doc_to_print.document_name, doc_to_print.priority, doc_to_print.num_pages);
    }
}

void print_report() {
    printf("\nRelatório Final de Impressão:\n");

    // Total de páginas por impressora
    for (size_t i = 0; i < total_pages_printed.size(); i++) {
        printf("Impressora %zu: %d páginas impressas\n", i, total_pages_printed[i]);
    }

    // Documentos impressos
    printf("\nDocumentos Impressos:\n");
    for (const auto& doc : printed_documents) {
        double print_duration = difftime(doc.print_time, doc.request_time); // Tempo total de impressão
        printf("Documento: %s, Páginas: %d, Prioridade: %d, Processo: %d\n", 
               doc.document_name, doc.num_pages, doc.priority, doc.process_id);
        printf("Horário da Solicitação: %s", ctime(&doc.request_time));
        printf("Horário da Impressão: %s", ctime(&doc.print_time));
        printf("Tempo Total de Impressão: %.2f segundos\n\n", print_duration);
    }
}

int main() {
    srand(time(NULL));

    int num_processes, num_printers;
    printf("Digite o número de processos: ");
    scanf("%d", &num_processes);
    printf("Digite o número de impressoras: ");
    scanf("%d", &num_printers);
    printf("Digite a capacidade máxima do buffer: ");
    scanf("%d", &buffer_capacity);
    printf("Digite o tempo de impressão por página (ms): ");
    scanf("%d", &print_time);

    sem_init(&semEmpty, 0, buffer_capacity);
    sem_init(&semFull, 0, 0);
    pthread_mutex_init(&mutexBuffer, NULL);

    total_pages_printed.resize(num_printers, 0);

    pthread_t producers[num_processes];
    pthread_t consumers[num_printers];
    int printer_ids[num_printers];

    for (int i = 0; i < num_processes; i++) {
        pthread_create(&producers[i], NULL, producer, NULL);
    }
    for (int i = 0; i < num_printers; i++) {
        printer_ids[i] = i;
        pthread_create(&consumers[i], NULL, consumer, &printer_ids[i]);
    }

    sleep(30); // Simulação de 30 segundos
    print_report();

    pthread_mutex_destroy(&mutexBuffer);
    sem_destroy(&semEmpty);
    sem_destroy(&semFull);
    return 0;
}
