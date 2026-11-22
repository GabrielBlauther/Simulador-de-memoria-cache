# Simulador de memória cache

**Autores:** Pedro Henrique Grando e Gabriel De Souza Blauther

Simulador em C para a disciplina de Fundamentos de Arquitetura de Computadores. O programa lê um traço de acessos à memória e calcula a taxa de acertos, o tempo médio de acesso e o número de leituras e escritas na memória principal.

## Compilar e executar

No PowerShell, com GCC instalado, execute na raiz do repositório:

```powershell
gcc -std=c99 -Wall -Wextra -O2 simulador_de_cache.c -o simulador_de_cache.exe
.\simulador_de_cache.exe arquivo_de_dados.txt 0 128 64 4 4 LRU 60
```

O exemplo usa uma cache de 8 KiB (64 linhas de 128 bytes), quatro vias, escrita *write-through* e substituição LRU. O tempo de consulta à cache é de 4 ns e o de acesso à memória principal é de 60 ns.

| Argumento | Valor esperado |
| --- | --- |
| Arquivo | Traço com um endereço hexadecimal e uma operação `R` ou `W` por linha |
| Política de escrita | `0` para *write-through* ou `1` para *write-back* |
| Tamanho da linha | Número de bytes, em potência de dois |
| Número de linhas | Potência de dois |
| Associatividade | Número de vias, em potência de dois, até o número de linhas |
| Tempo de acerto | Tempo de consulta à cache, em ns |
| Substituição | `LRU` ou `Aleatoria` |
| Tempo da memória | Tempo de acesso à memória principal, em ns |
| Semente (opcional) | Inteiro para repetir a substituição aleatória no mesmo ambiente |

## Experimentos

O [traço de entrada](arquivo_de_dados.txt) contém 51.200 acessos. Os [resultados em CSV](resultados.csv) e o relatório preliminar em [HTML](relatorio-preliminar.html) e [PDF](relatorio-preliminar.pdf) registram os experimentos com diferentes configurações de cache.
