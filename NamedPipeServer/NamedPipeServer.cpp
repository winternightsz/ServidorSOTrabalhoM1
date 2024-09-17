#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "ThreadPool.h"

using namespace std;
using namespace std::chrono;

//define os nomes dos pipes para strings e numeros
#define PIPE_NUMEROS TEXT("\\\\.\\pipe\\PipeNumeros")     
#define PIPE_STRINGS TEXT("\\\\.\\pipe\\PipeStrings") 
#define MAX_NUM_PIPE 2 //ate 2 clientes vao poder estar conectados no pipe simultaneamente porque o maximo sao 2 instancias do pipe

int tempoNumero = 0; //pro rand() nao ficar toda vez igual

void responderSolicitacoesNumeros(HANDLE hPipe) {
    srand(time(0) + tempoNumero);
    char buffer[1024] = { 0 }; //o buffer comeca com zeros
    DWORD bytesLidos;
    bool clienteConectado = true;

    //so sai do loop quando acabar ou se der erro
    while (clienteConectado) {
        memset(buffer, 0, sizeof(buffer)); //vai limpar o buffer toda vez que roda

        auto inicio = high_resolution_clock::now();

        if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesLidos, NULL)) {
            buffer[bytesLidos] = '\0';  
            int numeroAleatorio = rand() % 100;
            string resposta = "" + to_string(numeroAleatorio);
            DWORD bytesEscritos;
            WriteFile(hPipe, resposta.c_str(), resposta.size(), &bytesEscritos, NULL);
        }
        else {
            clienteConectado = false;  //cliente desconectou ou deu erro
        }

        tempoNumero++;

        auto fim = high_resolution_clock::now();
        auto duracao = duration_cast<microseconds>(fim - inicio);

        cout << "Thread " << this_thread::get_id() << " solicitacao NUMEROS. "
            << "Tempo de execucao: " << duracao.count() << "us" << endl;
    }
    
    //fecha o pipe
    CloseHandle(hPipe);
}

void responderSolicitacoesStrings(HANDLE hPipe) {
    srand(time(0));
    char buffer[1024] = { 0 };
    DWORD bytesLidos;
    bool clienteConectado = true;

    while (clienteConectado) {
        memset(buffer, 0, sizeof(buffer));

        auto inicio = high_resolution_clock::now();

        if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesLidos, NULL)) {
            buffer[bytesLidos] = '\0';
            string respostaString = "frase teste ";
            DWORD bytesEscritos;
            WriteFile(hPipe, respostaString.c_str(), respostaString.size(), &bytesEscritos, NULL);
        }
        else {
            clienteConectado = false;
        }

        auto fim = high_resolution_clock::now();
        auto duracao = duration_cast<microseconds>(fim - inicio);

        cout << "Thread " << this_thread::get_id() << " solicitacao STRING. "
            << "Tempo de execucao: " << duracao.count() << "us" << endl;
    }

    CloseHandle(hPipe);
}

int main() {
    //vai criar uma pool de threads com 4 threads
    PoolDeThreads pool(4);

    while (true) {

        //aqui vai criar o pipe pra numeros
        HANDLE hPipeNumeros = CreateNamedPipe(
            PIPE_NUMEROS,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            MAX_NUM_PIPE,  
            1024,
            1024,
            0,
            NULL
        );

        if (hPipeNumeros == INVALID_HANDLE_VALUE) {
            cout << "erro pra criar o pipe de numeros codigo: " << GetLastError() << endl;
            return 1;
        }

        //cria o pipe de strings
        HANDLE hPipeStrings = CreateNamedPipe(
            PIPE_STRINGS,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // quando le do PIPE_WAIT a leitura bloqueia ate que os dados estejam disponiveis no pipe
            MAX_NUM_PIPE,  
            1024,
            1024,
            0,
            NULL
        );

        if (hPipeStrings == INVALID_HANDLE_VALUE) {
            cout << "erro pra criar o pipe de strings codigo: " << GetLastError() << endl;
            return 1;
        }

        cout << "Esperando clientes" << endl;

        //tentar se conectar com o cliente
        //se retornar 0 deu falha na conexao
        if (ConnectNamedPipe(hPipeNumeros, NULL) == 0) {
            if (GetLastError() != ERROR_PIPE_CONNECTED) {
                cout << "erro pra conectar ao cliente de numeros codigo: " << GetLastError() << endl;
                CloseHandle(hPipeNumeros);
                continue;
            }
        }

        
        if (ConnectNamedPipe(hPipeStrings, NULL) == 0) {
            if (GetLastError() != ERROR_PIPE_CONNECTED) {
                cout << "erro pra conectar ao cliente de strings codigo: " << GetLastError() << endl;
                CloseHandle(hPipeStrings);
                continue;
            }
        }

        cout << "Clientes conectados" << endl;

        //adiciona uma nova tarefa no pool de threads
        //o lambda[] faz uma copia da variavel do Handle do hPipeNumeros
        //vai ter as funcoes dentro pra serem executadas quando o Handle do hPipeNumeros for chamado  
        pool.adicionarTarefa([hPipeNumeros] {
            responderSolicitacoesNumeros(hPipeNumeros);
            DisconnectNamedPipe(hPipeNumeros); //disconecta
            CloseHandle(hPipeNumeros); //fecha
            });

        pool.adicionarTarefa([hPipeStrings] {
            responderSolicitacoesStrings(hPipeStrings);
            DisconnectNamedPipe(hPipeStrings);
            CloseHandle(hPipeStrings);
            });

        cout << "Clientes desconectados" << endl;
    }

    return 0;
}
