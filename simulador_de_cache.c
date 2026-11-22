#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    unsigned int tag;
    int valido;
    int dirty;
    unsigned long long uso;
} LinhaCache;

typedef struct {
    char arquivo[128];
    int escrita;
    unsigned int tam_linha;
    unsigned int linhas;
    unsigned int assoc;
    double hit_time;
    char substituicao[32];
    double tempo_memoria;
} Config;

typedef struct {
    unsigned long long acessos;
    unsigned long long leituras;
    unsigned long long escritas;
    unsigned long long hits_leitura;
    unsigned long long hits_escrita;
    unsigned long long leituras_mp;
    unsigned long long escritas_mp;
    double tempo_total;
} Stats;

int eh_potencia_2(unsigned int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

int eh_lru(const char *s) {
    return strcmp(s, "LRU") == 0;
}

int eh_aleatoria(const char *s) {
    return strcmp(s, "Aleatoria") == 0;
}

void uso(const char *nome) {
    printf("%s arquivo politica_escrita tamanho_linha numero_linhas associatividade hit_time substituicao tempo_memoria [semente]\n", nome);
}

int ler_config(int argc, char **argv, Config *c) {
    if (argc != 9 && argc != 10)
        return 0;

    strncpy(c->arquivo, argv[1], sizeof(c->arquivo) - 1);
    c->arquivo[sizeof(c->arquivo) - 1] = '\0';
    c->escrita = atoi(argv[2]);
    c->tam_linha = (unsigned int) strtoul(argv[3], NULL, 10);
    c->linhas = (unsigned int) strtoul(argv[4], NULL, 10);
    c->assoc = (unsigned int) strtoul(argv[5], NULL, 10);
    c->hit_time = atof(argv[6]);
    strncpy(c->substituicao, argv[7], sizeof(c->substituicao) - 1);
    c->substituicao[sizeof(c->substituicao) - 1] = '\0';
    c->tempo_memoria = atof(argv[8]);

    return 1;
}

int validar_config(Config *c) {
    return (c->escrita == 0 || c->escrita == 1) &&
           eh_potencia_2(c->tam_linha) &&
           eh_potencia_2(c->linhas) &&
           eh_potencia_2(c->assoc) &&
           c->assoc >= 1 &&
           c->assoc <= c->linhas &&
           c->linhas % c->assoc == 0 &&
           (eh_lru(c->substituicao) || eh_aleatoria(c->substituicao));
}

int escolher_vitima(LinhaCache *cache, int inicio, int assoc, int lru) {
    int i, vitima = inicio;

    for (i = 0; i < assoc; i++)
        if (!cache[inicio + i].valido)
            return inicio + i;

    if (!lru)
        return inicio + rand() % assoc;

    for (i = 1; i < assoc; i++)
        if (cache[inicio + i].uso < cache[vitima].uso)
            vitima = inicio + i;

    return vitima;
}

int procurar_hit(LinhaCache *cache, int inicio, int assoc, unsigned int tag) {
    int i;

    for (i = 0; i < assoc; i++)
        if (cache[inicio + i].valido && cache[inicio + i].tag == tag)
            return inicio + i;

    return -1;
}

void finalizar_write_back(LinhaCache *cache, Config *c, Stats *s) {
    unsigned int i;

    if (c->escrita != 1)
        return;

    for (i = 0; i < c->linhas; i++) {
        if (cache[i].valido && cache[i].dirty) {
            s->escritas_mp++;
            s->tempo_total += c->tempo_memoria;
        }
    }
}

void tratar_hit(LinhaCache *cache, int pos, char op, Config *c, Stats *s, unsigned long long *tempo) {
    cache[pos].uso = ++(*tempo);

    if (op == 'R') {
        s->hits_leitura++;
        return;
    }

    s->hits_escrita++;
    if (c->escrita == 0) {
        s->escritas_mp++;
        s->tempo_total += c->tempo_memoria;
    } else {
        cache[pos].dirty = 1;
    }
}

void carregar_bloco(LinhaCache *cache, int inicio, unsigned int tag, char op, Config *c, Stats *s, unsigned long long *tempo) {
    int vitima = escolher_vitima(cache, inicio, (int) c->assoc, eh_lru(c->substituicao));

    if (cache[vitima].valido && cache[vitima].dirty) {
        s->escritas_mp++;
        s->tempo_total += c->tempo_memoria;
    }

    s->leituras_mp++;
    s->tempo_total += c->tempo_memoria;

    cache[vitima].tag = tag;
    cache[vitima].valido = 1;
    cache[vitima].uso = ++(*tempo);
    cache[vitima].dirty = op == 'W' && c->escrita == 1;

    if (op == 'W' && c->escrita == 0) {
        s->escritas_mp++;
        s->tempo_total += c->tempo_memoria;
    }
}

void formatar_decimal(char *destino, size_t tamanho, double valor, int casas) {
    char formato[16];
    int i;

    snprintf(formato, sizeof(formato), "%%.%df", casas);
    snprintf(destino, tamanho, formato, valor);

    for (i = 0; destino[i] != '\0'; i++)
        if (destino[i] == '.')
            destino[i] = ',';
}

int simular(Config *c, Stats *s, unsigned int semente) {
    FILE *entrada = fopen(c->arquivo, "r");
    LinhaCache *cache = calloc(c->linhas, sizeof(LinhaCache));
    unsigned int conjuntos = c->linhas / c->assoc;
    unsigned long long tempo = 0;
    char endereco_txt[64], op;

    if (!entrada || !cache) {
        if (entrada)
            fclose(entrada);
        free(cache);
        return 0;
    }

    memset(s, 0, sizeof(Stats));
    srand(semente);

    while (fscanf(entrada, "%63s %c", endereco_txt, &op) == 2) {
        unsigned int endereco = (unsigned int) strtoul(endereco_txt, NULL, 16);
        unsigned int bloco = endereco / c->tam_linha;
        unsigned int conjunto = bloco % conjuntos;
        unsigned int tag = bloco / conjuntos;
        int inicio = (int) (conjunto * c->assoc);
        int hit;

        if (op != 'R' && op != 'W')
            continue;

        s->acessos++;
        s->tempo_total += c->hit_time;

        if (op == 'R')
            s->leituras++;
        else
            s->escritas++;

        hit = procurar_hit(cache, inicio, (int) c->assoc, tag);

        if (hit >= 0)
            tratar_hit(cache, hit, op, c, s, &tempo);
        else
            carregar_bloco(cache, inicio, tag, op, c, s, &tempo);
    }

    finalizar_write_back(cache, c, s);
    fclose(entrada);
    free(cache);
    return 1;
}

void imprimir_resultado(Config *c, Stats *s) {
    unsigned long long hits = s->hits_leitura + s->hits_escrita;
    double hit_global = s->acessos ? ((double) hits / s->acessos) * 100.0 : 0.0;
    double tempo_medio = s->acessos ? s->tempo_total / s->acessos : 0.0;
    char hit_txt[32], tempo_txt[32];

    formatar_decimal(hit_txt, sizeof(hit_txt), hit_global, 4);
    formatar_decimal(tempo_txt, sizeof(tempo_txt), tempo_medio, 4);

    printf("Arquivo: %s\n", c->arquivo);
    printf("Tempo de acerto na cache: %.4f ns\n", c->hit_time);
    printf("Tempo de leitura e escrita na memoria principal: %.4f ns\n", c->tempo_memoria);
    printf("Acessos: %llu leituras, %llu escritas, %llu no total\n",
           s->leituras, s->escritas, s->acessos);
    printf("Acertos de leitura: %.4f%% (%llu)\n",
           s->leituras ? 100.0 * s->hits_leitura / s->leituras : 0.0, s->hits_leitura);
    printf("Acertos de escrita: %.4f%% (%llu)\n",
           s->escritas ? 100.0 * s->hits_escrita / s->escritas : 0.0, s->hits_escrita);
    printf("Acertos globais: %s%% (%llu)\n", hit_txt, hits);

    printf("\nTabela 1 - Parametros da Simulacao\n\n");
    printf("+----------------------------+----------------+\n");
    printf("| Parametro                  | Valor          |\n");
    printf("+----------------------------+----------------+\n");
    printf("| Tamanho do Bloco           | %-10u bytes |\n", c->tam_linha);
    printf("| Politica de Escrita        | %-14s |\n", c->escrita == 0 ? "write-through" : "write-back");
    printf("| Algoritmo de Substituicao  | %-14s |\n", eh_lru(c->substituicao) ? "LRU" : "Aleatoria");
    printf("| Associatividade            | %-9u blocos |\n", c->assoc);
    printf("+----------------------------+----------------+\n");

    printf("\nTabela 2 - Resultados da Simulacao\n\n");
    printf("+------------------+----------------+--------------------------+----------------+----------------+\n");
    printf("| Numero de Blocos | Taxa de Acerto | Tempo medio de Acesso ns | Leituras na MP | Escritas na MP |\n");
    printf("+------------------+----------------+--------------------------+----------------+----------------+\n");
    printf("| %-16u | %14s%% | %24s | %14llu | %14llu |\n",
           c->linhas, hit_txt, tempo_txt, s->leituras_mp, s->escritas_mp);
    printf("+------------------+----------------+--------------------------+----------------+----------------+\n");
}

int main(int argc, char **argv) {
    Config c;
    Stats s;

    if (!ler_config(argc, argv, &c)) {
        uso(argv[0]);
        return 1;
    }

    if (!validar_config(&c)) {
        printf("Parametros invalidos.\n");
        return 1;
    }

    unsigned int semente = argc == 10 ? (unsigned int) strtoul(argv[9], NULL, 10)
                                       : (unsigned int) time(NULL);
    if (!simular(&c, &s, semente)) {
        printf("Nao foi possivel abrir o arquivo ou alocar memoria.\n");
        return 1;
    }

    if (eh_aleatoria(c.substituicao))
        printf("Semente aleatoria: %u\n", semente);
    imprimir_resultado(&c, &s);
    return 0;
}
