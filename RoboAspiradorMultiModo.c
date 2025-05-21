/**
 * Autor: Daniel Porto Braz
 * Este código faz parte do projeto RoboAspiradorMultiModo, que se refere a um sistema de monitoramento e controle
 * de um robô através de um display e práticas do módulo CYW43439 para que esta seja uma aplicação do conceito de
 * Webserver. O programa funciona em dois modos, sendo o automático em que o robô percorre toda a área do display,
 * e o manual, que o usuário controla o robô por uma página HTML no celular ou computador.
 * 
 * O código está baseado no repositório de Ricardo Menezes Prates: 
 * https://github.com/rmprates84/pico_wifi_cyw43_webserver_04
 */

#include <stdio.h>               // Biblioteca padrão para entrada e saída
#include <string.h>              // Biblioteca manipular strings
#include <stdlib.h>              // funções para realizar várias operações, incluindo alocação de memória dinâmica (malloc)

#include "pico/stdlib.h"         // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "hardware/i2c.h" // Para interface de comunicação I2C
#include "lib/ssd1306.h" // Funções de configuração e manipulação com o display OLED
#include "lib/font.h" // Caracteres 8x8 para exibir no display OLED
#include "hardware/adc.h"        // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "pico/cyw43_arch.h"     // Biblioteca para arquitetura Wi-Fi da Pico com CYW43  

// Não utilizadas
#include "FreeRTOS.h"  // Biblioteca para implementar o FreeRTOS
#include "task.h"      // Biblioteca para implementar tasks do FreeRTOS
#include "queue.h"     // Biblioteca para implementar filas do FreeRTOS

#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

// Credenciais WIFI - Tome cuidado se publicar no github!
#define WIFI_SSID "wifi"
#define WIFI_PASSWORD "senha"

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43
#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_GREEN_PIN 11                // GPIO11 - LED verde

// I2C e Display OLED 1306
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
ssd1306_t ssd; // Inicializa a estrutura do display
static volatile uint8_t x = 8, y = 8, last_x = 0, last_y = 0; // Armazena a posição atual e a anterior do robô

// Armazena o modo atual. True para manual e False para automático
static volatile bool current_mode = false; 
bool direcao = 0; // Controla a direção do robô. 0: Direita; 1: Esquerda
    int passos = 0; // Conta os passos do robô, evitando que ele passe das bordas horizontais

void xSetModeTask(void *vparams){

    while (true){
        
    }
}

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void);

// Rotina do Modo Automático
void auto_mode();

// Move o robô para a frente (para cima no display)
void move_up();

// Move o robô para a esquerda
void move_left();

// Move o robô para a direita
void move_right();

// Move o robô para trás (para baixo no display)
void move_down();

// Inicializar a comunicação i2c 
void initialize_i2c(void);

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Tratamento do request do usuário
void user_request(char **request);

// Função principal
int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
    gpio_led_bitdog();

    // Inicializar a comunicação I2C 
    initialize_i2c();

    // Configuração do Display OLED 1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display


    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    bool init = true; // Variável para ativar a aplicação das funcionalidades iniciais
    
    while (true)
    {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
       cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
       
       if (init){
            ssd1306_fill(&ssd, false);           
            ssd1306_send_data(&ssd);

            // Desenha quadrados 4x4 espalhados pela área do display para representar áreas com poeira
            for (int i = 8; i < 128; i += 16){
                for (int j = 8; j < 64; j += 16){
                    ssd1306_robo(&ssd, 4, i, j); // Reaproveita a função de desenho do robô para desenhar os quadrados de poeira
                    ssd1306_send_data(&ssd);
                    sleep_ms(10);
                }
            }
            ssd1306_robo(&ssd, 8, x, y); // Desenha o robô na posição inicial
            ssd1306_send_data(&ssd);
            sleep_ms(50);
            last_x = x;
            last_y = y;
            init = false;
        }
        
        // Se o robô mudou de posição, a anterior é apagada
        if (last_x != x || last_y != y){
            ssd1306_robo(&ssd, 8, x, y); // Desenha o robô na posição atual
            ssd1306_send_data(&ssd);
            sleep_ms(50);
            ssd1306_remove_robo(&ssd, 8, last_x, last_y); // Apaga o robô desenhado e, consequentemente, a poeira naquela posição
            ssd1306_send_data(&ssd);
            last_x = x;
            last_y = y;
        }

        auto_mode(); // Executa o modo automático, caso este esteja ativado

        sleep_ms(50);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------
// Rotina do Modo Automático
void auto_mode(){
    // Faz o robô percorrer toda a área do display

    if (current_mode){ // Enquanto estiver no modo automático

        if (passos < 14){ // Se o robô não chegou na borda, ele dá mais um passo
            
            if (direcao == 0) // Se a direção for 0, o robô segue para a direita
                move_right();

            else // Se a direção for 0, o robô segue para a esquerda
                move_left();

            passos++;
        }

        else {
            direcao = !direcao; // Alterna a direção após o robô chegar na borda horizontal
            passos = 0; // Reinicia o contador de passos na horizontal
            move_down();
        }

        sleep_ms(800); // Intervalo entre passos
    }
}

// Todas as funções atualizam os eixos do robô garantindo que ele não passe das bordas
// Move o robô para a frente (para cima no display)
void move_up(){
    if (y >= 16) y -= 8;
    sleep_ms(10); // Atraso para evitar múltiplas leituras
}

// Move o robô para a esquerda
void move_left(){
    if (x >= 16) x -= 8;
    sleep_ms(10); // Atraso para evitar múltiplas leituras
}

// Move o robô para a direita
void move_right(){
    if (x <= 112) x += 8;
    sleep_ms(10); // Atraso para evitar múltiplas leituras
}

// Move o robô para trás (para baixo no display)
void move_down(){
    if (y <= 48) y += 8;
    sleep_ms(10); // Atraso para evitar múltiplas leituras
}

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void){
    // Configuração dos LEDs como saída
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, false);
    
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);
}

// I2C
void initialize_i2c(){
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário - digite aqui
void user_request(char** request){

    if (strstr(*request, "GET /up") != NULL)
    {
        printf("Robo para a frente.\n");
        current_mode = false;
        gpio_put(LED_GREEN_PIN, 0); // Desliga o led verde do modo automático
        gpio_put(LED_BLUE_PIN, 1); // Liga o led azul do modo manual
        move_up();
    }
    else if (strstr(*request, "GET /left") != NULL)
    {
        printf("Robo para a esquerda.\n");
        current_mode = false;
        gpio_put(LED_GREEN_PIN, 0); // Desliga o led verde do modo automático
        gpio_put(LED_BLUE_PIN, 1); // Liga o led azul do modo manual
        move_left();
    }
    else if (strstr(*request, "GET /right") != NULL)
    {
        printf("Robo para a direita.\n");
        current_mode = false;
        gpio_put(LED_GREEN_PIN, 0); // Desliga o led verde do modo automático
        gpio_put(LED_BLUE_PIN, 1); // Liga o led azul do modo manual
        move_right();
    }
    else if (strstr(*request, "GET /down") != NULL)
    {
        printf("Robo para tras.\n");
        current_mode = false;
        gpio_put(LED_GREEN_PIN, 0); // Desliga o led verde do modo automático
        gpio_put(LED_BLUE_PIN, 1); // Liga o led azul do modo manual
        move_down();
    }
    else if(strstr(*request, "GET /auto") != NULL){
        current_mode = true;
        printf("Modo Automatico ativado.\n");
        gpio_put(LED_BLUE_PIN, 0); // Desliga o led azul do modo manual
        gpio_put(LED_GREEN_PIN, 1); // Liga o led verde do modo automático
        // O modo automático é executado na main
    }
};

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);

    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html), // Formatar uma string e armazená-la em um buffer de caracteres
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "<!DOCTYPE html><html><head><title>RAMM</title><style>"
            "body{background:#c2f016;font-family:sans-serif;text-align:center;margin-top:30px;}"
            "h1{font-size:48px;margin-bottom:20px;}"
            ".r{display:flex;justify-content:center;align-items:center;margin:10px 0;}"
            "button{font-size:28px;font-weight:bold;border-radius:8px;border:none;padding:15px 30px;margin:8px;cursor:pointer;}"
            ".a{background:#28a745;color:#fff;}"
            ".v{background:#fff;color:#00aaff;border:4px solid #00aaff;width:80px;height:80px;margin:8px;}"
            "</style></head><body><h1>RAMM</h1>"

            "<div class=r><form action=\"/auto\"><button class=a>Auto</button></form></div>"

            "<div class=r><div style=\"width:240px;display:flex;justify-content:center;\">"
            "<form action=\"/up\"><button class=v>W</button></form>"
            "</div></div>"

            "<div class=r><div style=\"width:240px;display:flex;justify-content:space-between;\">"
            "<form action=\"/left\"><button class=v>A</button></form>"
            "<form action=\"/down\"><button class=v>S</button></form>"
            "<form action=\"/right\"><button class=v>D</button></form>"
            "</div></div>"

            "</body></html>"

            );

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}
