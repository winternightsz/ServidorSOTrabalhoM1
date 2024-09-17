#ifndef POOL_DE_THREADS_H
#define POOL_DE_THREADS_H

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

using namespace std;

class PoolDeThreads {
public:
    PoolDeThreads(size_t numeroDeThreads); //construtor

    //adiciona uma tarefa na pool de threads
    //function porque esta usando lambda
    void adicionarTarefa(function<void()> tarefa);

private:
    vector<thread> trabalhadores; //vetor de threads
    queue<function<void()>> tarefas; //fila de tarefas

    mutex mutexFila; //mutex pra proteger o acesso a fila
    condition_variable condicao; //variavel de condicao pra controlar a espera

    void funcaoTrabalhador(); //funcao que cada trabalhador executa
};



//pra cada iteracao vai criar uma thread e vai executar funcaoTrabalhador 
PoolDeThreads::PoolDeThreads(size_t numeroDeThreads) {
    for (size_t i = 0; i < numeroDeThreads; ++i) {
        trabalhadores.emplace_back([this] { this->funcaoTrabalhador(); });
    }
}



//unique_lock desbloqueia automaticamente
//vai colcoar uma tarefa na fila de tarefas e notificar uma thread
void PoolDeThreads::adicionarTarefa(function<void()> tarefa) {
    {
        unique_lock<mutex> lock(mutexFila);
        tarefas.push(tarefa);
    }
    condicao.notify_one();
}



//loop infinitop pra poder sempre estar pronto
//vai esperar ate ter uma tarefa na fila e vai remover a primeira pra executar ela
void PoolDeThreads::funcaoTrabalhador() {
    while (true) {
        function<void()> tarefa;
        {
            unique_lock<mutex> lock(mutexFila);
            condicao.wait(lock, [this] { return !tarefas.empty(); }); //espera por uma tarefa na fila
            tarefa = move(tarefas.front());
            tarefas.pop();
        }
        tarefa(); //executa a tarefa
    }
}

#endif // POOL_DE_THREADS_H
