
# RoboAspiradorMultiModo

Este projeto implementa um **robô aspirador virtual multi-modo**, desenvolvido para a **Raspberry Pi Pico W**, utilizando o módulo **CYW43439 (Wi-Fi)** e um display **OLED SSD1306**. A aplicação ilustra conceitos de **webserver embarcado**, controle por rede local e manipulação gráfica com base em coordenadas 2D.

## Funcionalidades

- Simulação visual de áreas com poeira (quadrados 4x4).
- Representação do robô aspirador (quadrado 8x8).
- Aspiração de áreas cobertas pelo robô.
- Controle manual via navegador por requisições HTTP.
- Estrutura pronta para modo automático (em desenvolvimento).
- Interface no display OLED via I2C.
- Indicação de modo por LED.

---

## Como funciona

- Ao iniciar, o sistema conecta a uma rede Wi-Fi (modo **STA**).
- Um **servidor HTTP embarcado** é iniciado na Pico W.
- Um HTML é enviado ao navegador do usuário, permitindo controle direcional (`W`, `A`, `S`, `D`).
- Cada clique no botão gera uma requisição do tipo `GET`, tratada pela função `user_request()`.
- O robô se movimenta em resposta, apagando poeira (pixels) sob sua nova posição.
- A posição anterior é apagada corretamente com `ssd1306_remove_robo()` para evitar rastros visuais.

---

## Requisitos

- Raspberry Pi Pico W
- Display OLED SSD1306 (I2C)
- Ambiente de desenvolvimento com **Pico SDK** e **lwIP**
- Conexão Wi-Fi disponível (modo STA)

---

## Estrutura do Projeto

```bash
├── RoboAspiradorMultiModo.c        # Código principal
├── lib/
│   ├── ssd1306.h / .c              # Funções do display
│   └── font.h                      # Fonte 8x8 (opcional)
└── README.md
```

---

## Exemplo de uso

1. Compile e grave o código na Pico W.
2. Conecte-se à mesma rede Wi-Fi configurada no código.
3. Abra o navegador e acesse o IP mostrado no terminal da Pico.
4. Use os botões da interface para mover o robô.
5. Observe a aspiração da poeira em tempo real no display.

---

## Demonstração
https://www.youtube.com/playlist?list=PLaN_cHSVjBi9xvA6UQ6v0qsFj4KHnt05l

---

## Créditos

- Código base inspirado no repositório de [Ricardo Menezes Prates](https://github.com/rmprates84/pico_wifi_cyw43_webserver_04).
- Desenvolvido por [Daniel Porto Braz](https://github.com/DanielPortoBraz) para fins educacionais e de demonstração de sistemas embarcados com Webserver.

---

