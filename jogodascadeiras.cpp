#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <semaphore.h> // para semáforo POSIX (Linux/Unix)
#include <chrono>
#include <random>

using namespace std;

// Utilitário para gerar tempos aleatórios
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> distr(2000, 5000); // 2 a 5 segundos

class JogoDasCadeiras;

// Jogador
class Jogador {
public:
    int id;
    bool eliminado = false;
    JogoDasCadeiras* jogo;
    thread th;

    Jogador(int id, JogoDasCadeiras* jogo) : id(id), jogo(jogo) {}

    void iniciar();
};

// Jogo
class JogoDasCadeiras {
public:
    int num_cadeiras;
    mutex mtx;
    condition_variable music_cv;
    bool musica_tocando = true;
    sem_t semaphore;
    vector<Jogador*> jogadores;
    bool jogo_ativo = true;

    JogoDasCadeiras(int n) : num_cadeiras(n - 1) {
        sem_init(&semaphore, 0, num_cadeiras);
    }

    ~JogoDasCadeiras() {
        sem_destroy(&semaphore);
    }

    void iniciar_jogo();
    void parar_musica();
    void eliminar_jogador();
    void exibir_estado();
};

// Implementação da thread dos jogadores
void Jogador::iniciar() {
    while (!eliminado) {
        unique_lock<mutex> lock(jogo->mtx);
        jogo->music_cv.wait(lock, [this]() { return !jogo->musica_tocando || eliminado; });

        if (eliminado) break; // Se já eliminado, sair

        lock.unlock();

        // Tentar ocupar uma cadeira (semaphore wait)
        if (sem_trywait(&(jogo->semaphore)) == 0) {
            // Conseguiu sentar!
        } else {
            // Não conseguiu sentar
            eliminado = true;
        }
    }
}

// Coordenador: Controla a música
void JogoDasCadeiras::iniciar_jogo() {
    cout << "-----------------------------------------------\n";
    cout << "Bem-vindo ao Jogo das Cadeiras Concorrente!\n";
    cout << "-----------------------------------------------\n\n";

    while (jogadores.size() > 1) {
        cout << "Iniciando rodada com " << jogadores.size() << " jogadores e " << num_cadeiras << " cadeiras.\n";
        musica_tocando = true;

        cout << "A música está tocando... 🎵\n\n";
        this_thread::sleep_for(chrono::milliseconds(distr(gen)));

        parar_musica();

        this_thread::sleep_for(chrono::milliseconds(500)); // Pequeno delay para os jogadores agirem

        eliminar_jogador();
        exibir_estado();

        num_cadeiras--;
        sem_destroy(&semaphore);
        sem_init(&semaphore, 0, num_cadeiras);

        this_thread::sleep_for(chrono::seconds(2));
    }

    cout << "🏆 Vencedor: Jogador P" << jogadores[0]->id << "! Parabéns! 🏆\n";
    cout << "-----------------------------------------------\n";
    cout << "Obrigado por jogar o Jogo das Cadeiras Concorrente!\n";

    jogo_ativo = false;
    music_cv.notify_all();
}

// Para a música
void JogoDasCadeiras::parar_musica() {
    lock_guard<mutex> lock(mtx);
    musica_tocando = false;
    cout << "> A música parou! Os jogadores estão tentando se sentar...\n\n";
    music_cv.notify_all();
}

// Elimina o jogador que não conseguiu sentar
void JogoDasCadeiras::eliminar_jogador() {
    for (auto it = jogadores.begin(); it != jogadores.end(); ++it) {
        if ((*it)->eliminado) {
            cout << "Jogador P" << (*it)->id << " não conseguiu uma cadeira e foi eliminado!\n";
            (*it)->th.join();
            delete (*it);
            jogadores.erase(it);
            break;
        }
    }
}

// Exibe o estado atual das cadeiras
void JogoDasCadeiras::exibir_estado() {
    cout << "-----------------------------------------------\n";
    int cadeira = 1;
    for (auto& jogador : jogadores) {
        if (cadeira <= num_cadeiras) {
            cout << "[Cadeira " << cadeira++ << "]: Ocupada por P" << jogador->id << "\n";
        }
    }
    cout << "-----------------------------------------------\n\n";
}

// MAIN
int main() {
    int n;
    cout << "Digite o número de jogadores: ";
    cin >> n;

    if (n < 2) {
        cout << "Número mínimo de jogadores é 2.\n";
        return 1;
    }

    JogoDasCadeiras jogo(n);

    // Criar jogadores
    for (int i = 1; i <= n; i++) {
        Jogador* p = new Jogador(i, &jogo);
        p->th = thread(&Jogador::iniciar, p);
        jogo.jogadores.push_back(p);
    }

    // Iniciar o jogo
    jogo.iniciar_jogo();

    // Esperar todos os jogadores finalizarem
    for (auto& jogador : jogo.jogadores) {
        if (jogador->th.joinable())
            jogador->th.join();
        delete jogador;
    }

    return 0;
}
