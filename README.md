# Virtual CAN

Biblioteca C++17 para comunicação com interfaces CAN usando a interface
`ICanBus`. O projeto inclui a implementação SocketCAN para Linux,
`SocketCanBus`, e testes executados contra uma interface CAN virtual (`vcan0`).

## Documentação

### Fundamentos do protocolo

1. [Fundamentos do barramento CAN](<docs/1. can-fundamentos.html>): nós,
   barramento, CAN_H, CAN_L e uma rede com três ECUs.
2. [Identificadores CAN](<docs/2. identificadores-can.html>): IDs padrão de
   11 bits e estendidos de 29 bits, interpretados em hexadecimal.
3. [Frames CAN: Data, RTR e ACK](<docs/3. can-frames-data-rtr-ack.html>):
   campos do frame clássico e análise de exemplos.
   - [Aprofundamento: bit stuffing](<docs/8. bit-stuffing-can.html>): por que
     o CAN insere bits no fluxo e como o receptor os remove.
   - [Aprofundamento: CRC e integridade](<docs/9. crc-integridade-can.html>):
     cálculo e detecção de frames corrompidos no CAN clássico.
   - [Aprofundamento: ACK e detecção de erros](<docs/10. ack-erros-can.html>):
     ACK, erros de bit/stuffing/CRC/forma e confinamento de erros.
   - [Aprofundamento: Error Active, Error Passive e Bus Off](<docs/11. estados-erro-can.html>):
     contadores TEC/REC, transições de estado e recuperação do controlador.
4. [Arbitragem CAN](<docs/4. arbitragem-can.html>): prioridade e lógica
   dominante/recessiva, incluindo a comparação entre `0x100` e `0x200`.
5. [DLC e payload CAN](<docs/5. dlc-payload-can.html>): comprimentos de 0 a
   8 bytes, exemplos decodificados e tabela de mensagens de uma ECU fictícia.
6. [CAN clássico vs CAN FD](docs/can-classic-vs-canfd.html): diferenças de
   payload, taxa de dados e estrutura dos frames.
    - [Aprofundamento: estrutura do frame CAN FD](<docs/12. estrutura-frame-canfd.html>):
      campos FDF, BRS e ESI, fases de bitrate, DLC e CRC.
    - [Aprofundamento: DLC e comprimentos CAN FD](<docs/13. dlc-canfd-mapeamento.html>):
      tabela dos códigos DLC e payloads de até 64 bytes.
    - [Aprofundamento: taxa nominal e taxa de dados](<docs/14. taxa-nominal-vs-dados-canfd.html>):
      diferença entre bitrate nominal, dbitrate e BRS.
    - [Aprofundamento: CRC e compatibilidade CAN FD](<docs/15. crc-tolerancia-compatibilidade-canfd.html>):
      detecção de erros e requisitos de interoperabilidade entre nós.

### Laboratório e desenvolvimento

- [Laboratório CAN virtual com vcan](<docs/6. laboratorio-vcan.html>): uso de
  `candump` e `cansend`, filtros por ID, comparação de payloads e envio periódico
  com registro da captura.
- [Laboratório CAN FD com vcan](<docs/16. laboratorio-canfd-vcan.html>): criar
  `vcanfd0`, enviar frames FD com `cansend` e observar/filtrar com `candump`.
- [ICanBus e SocketCanBus](docs/icanbus-socketcan.html): abstração da
  comunicação CAN e implementação SocketCAN da biblioteca.
- [Protocolo de infotainment do veículo](docs/vehicle-infotainment-protocol.html):
  exemplo aplicado de requisição, resposta e decodificação de telemetria.

## Requisitos

- Linux com suporte ao módulo `vcan`;
- CMake 3.16 ou mais recente;
- compilador C++ com suporte a C++17;
- permissões para criar e ativar uma interface de rede virtual.

## Interface CAN virtual

Crie e ative a interface `vcan0` antes de compilar e executar os testes:

```shell
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up
```

Se a interface já existir, não é necessário executar novamente o comando
`ip link add`. Para removê-la ao terminar:

```shell
sudo ip link delete vcan0
```

## Interface CAN FD virtual

Para testar CAN FD separadamente, crie uma interface virtual dedicada. O MTU
de `72` bytes corresponde ao frame CAN FD completo, incluindo cabeçalho e até
64 bytes de dados:

```shell
sudo modprobe vcan
sudo ip link add dev vcanfd0 type vcan
sudo ip link set dev vcanfd0 mtu 72
sudo ip link set dev vcanfd0 up
```

Remova a interface quando terminar:

```shell
sudo ip link delete vcanfd0
```

Os testes CAN FD usam a interface virtual dedicada `vcanfd0`. Depois de
criá-la e ativá-la, execute:

```shell
ctest --test-dir build -R canfd_tests --output-on-failure
```

### Interface CAN FD física

Os comandos abaixo são opcionais e se aplicam somente a uma interface física
como `can0`. Eles não são necessários para os testes com `vcanfd0`:

```shell
sudo ip link set can0 type can bitrate 500000 dbitrate 2000000 fd on
sudo ip link set can0 up
```

## Compilação

Configure o projeto na pasta `build` e compile:

```shell
cmake -S . -B build
cmake --build build
```

O primeiro comando lê o `CMakeLists.txt` e gera os arquivos de build. O
segundo compila a biblioteca `canbus`, os executáveis de teste `test_can` e
`test_canfd`, além dos exemplos `vehicle_infotainment` e
`vehicle_infotainment_fd` e `arduino_rtr`.

## Testes

Execute todos os testes com CTest:

```shell
ctest --test-dir build --output-on-failure
```

Os testes verificam:

- envio e recepção de um quadro CAN;
- filtragem por identificador CAN;
- timeout quando nenhum quadro é recebido.
- simulação de comando e resposta entre dois módulos;
- ignorar comandos CAN desconhecidos.

Também é possível executar o binário diretamente:

```shell
./build/test_can
```

## Monitoramento com candump

O utilitário `candump`, do pacote `can-utils`, permite observar os frames
diretamente no terminal enquanto os exemplos ou testes estão em execução.

Instale o pacote, caso ainda não esteja disponível:

```shell
sudo apt install can-utils
```

Em um terminal separado, monitore a interface CAN clássica:

```shell
candump vcan0
```

Para monitorar a interface CAN FD:

```shell
candump vcanfd0
```

É possível filtrar por um identificador específico. Por exemplo, para observar
os frames do exemplo Arduino/RTR no ID `0x12`:

```shell
candump vcan0,0x12:0x7FF
```

O `candump` exibe o ID, o DLC e os bytes do payload. Frames RTR aparecem com a
indicação de frame remoto, permitindo diferenciar a mensagem `EV-ENGNR` do
pedido RTR com DLC `3`.

## Uso da biblioteca

Inclua `SocketCanBus.hpp` e vincule seu programa à biblioteca `canbus`:

```cpp
#include "SocketCanBus.hpp"

#include <chrono>
#include <cstdint>

int main()
{
	SocketCanBus bus("vcan0");

	const std::uint8_t payload[] = {0x11, 0x22, 0x33};
	const auto sendStatus = bus.sendFrame(0x123, payload, 3);

	auto result = bus.receiveFrame(std::chrono::milliseconds(1000));
	if (sendStatus == CanBusStatus::SUCCESS) {
		// result.frame->can_id identifica o quadro recebido.
	}
}
```

`sendFrame` retorna `CanBusStatus::SUCCESS` em caso de sucesso e diferencia
argumentos inválidos de falhas de transmissão. `receiveFrame` retorna o quadro
em `result.frame` e o status em `result.status`, que pode ser `TIMEOUT`,
`WAIT_ERROR` ou `READ_ERROR`. `setFilter` retorna `FILTER_ERROR` quando o filtro
não pode ser aplicado. Os quadros aceitos são CAN clássicos com identificador
padrão e até 8 bytes de dados. Para substituir o transporte, implemente
`ICanBus` e use a implementação necessária no código que depende da interface.

## Simulador de ECU

`EcuSimulator` modela um módulo que recebe um comando CAN específico e envia
uma resposta. Ele depende de `ICanBus`, portanto pode funcionar com
`SocketCanBus` ou com outra implementação compatível.

No exemplo abaixo, o comando esperado tem ID `0x600` e opcode `0x01` no
primeiro byte. Quando esse comando chega, a ECU responde com o ID `0x650` e
dois bytes de dados:

```cpp
#include "EcuSimulator.hpp"
#include "SocketCanBus.hpp"

#include <chrono>

SocketCanBus ecuBus("vcan0");
SocketCanBus clientBus("vcan0");

CanFrame response{};
response.can_id = 0x650;
response.len = 2;
response.data[0] = 0xAA;
response.data[1] = 0x55;

EcuSimulator ecu(ecuBus, 0x600, 0x01, response);

const uint8_t command[] = {0x01, 0x10};
clientBus.sendFrame(0x600, command, 2);

const auto status = ecu.processOnce(std::chrono::seconds(1));
```

`processOnce` lê apenas um frame. Se o timeout ou outro erro ocorrer, ele
propaga o `CanBusStatus` recebido. Se o ID ou opcode não corresponder ao
comando configurado, retorna `COMMAND_NOT_MATCHED` e não envia resposta. Para
um comando reconhecido, retorna o status da transmissão da resposta.

## CAN FD

O projeto também oferece `ICanFdBus` e `SocketCanFdBus`: a primeira define o contrato
do transporte CAN FD e a segunda é sua implementação SocketCAN. O contrato usa
tipos próprios, como `CanFdFrame` e `CanFdBusStatus`, sem depender de `ICanBus`.
`SocketCanFdBus` habilita `CAN_RAW_FD_FRAMES`, aceita payloads de até 64 bytes e
permite enviar flags como `CAN_FD_BITRATE_SWITCH`.

Os testes CAN FD podem ser executados com:

```shell
ctest --test-dir build -R canfd_tests --output-on-failure
```

Em `vcan`, o payload e o tamanho do frame são testados normalmente. Como a
interface virtual não possui bitrate físico, flags relacionadas ao bitrate,
como BRS, podem não ser preservadas no loopback. Em uma interface CAN FD real,
configure também o bitrate de dados com `dbitrate` e `fd on`.

## Exemplo: veículo e infotainment

O executável `vehicle_infotainment` simula dois módulos conectados à mesma
interface CAN virtual: uma ECU do veículo e o infotainment. O infotainment
solicita a telemetria, a ECU responde e o infotainment decodifica os valores
para exibição.

O protocolo do exemplo é:

| Frame | ID | Dados |
| --- | --- | --- |
| Requisição | `0x600` | `0x01` solicita velocidade e temperatura |
| Resposta | `0x650` | bytes 0-1: velocidade; bytes 2-3: temperatura |

Os valores são enviados como inteiros sem sinal em little-endian, multiplicados
por 10. Assim, `873` representa `87.3 km/h` e `223` representa `22.3 C`.

Com a interface `vcan0` ativa, execute:

```shell
./build/vehicle_infotainment
```

O fluxo é equivalente ao de um infotainment real: a tela não conversa
diretamente com o socket; ela recebe dados já interpretados pelo componente de
comunicação. No projeto, o frame trafega por `ICanBus`, a implementação
`SocketCanBus` usa SocketCAN e `EcuSimulator` representa a ECU que responde ao
comando.

### Exemplo veículo e infotainment com CAN FD

O executável `vehicle_infotainment_fd` implementa o mesmo fluxo usando
`ICanFdBus`, `SocketCanFdBus` e `EcuFdSimulator`. Ele usa a interface `vcanfd0`,
IDs estendidos de 29 bits e frames CAN FD.

Com a interface `vcanfd0` ativa, execute:

```shell
./build/vehicle_infotainment_fd
```

O protocolo permanece uma requisição com opcode `0x01` e uma resposta contendo
velocidade e temperatura, mas o transporte usa `CanFdFrame` e suporta payloads
de até 64 bytes.

### Exemplo Arduino com frame RTR

O executável `arduino_rtr` simula um Arduino enviando a mensagem de dados
`EV-ENGNR` e, em seguida, um frame remoto RTR a cada segundo, usando o ID
clássico `0x12`.

O frame de dados contém oito bytes:

| Frame | ID | DLC | Dados |
| --- | --- | --- | --- |
| Dados | `0x12` | `8` | `EV-ENGNR` |
| RTR | `0x12` | `3` | sem payload; DLC 3 solicita três bytes |

O valor `3` do RTR é o DLC, também chamado de tamanho solicitado. Ele não é um
byte de comando dentro do frame, pois frames RTR não carregam payload. O
monitor identifica o bit `CAN_RTR_FLAG` e diferencia os dois tipos de frame.

Este exemplo foi inspirado no vídeo de referência:
[Arduino CAN Bus RTR](https://www.youtube.com/watch?v=my-mBFQCIZ0).

Com a interface `vcan0` ativa, execute:

```shell
./build/arduino_rtr
```

## Licença

Este projeto é distribuído sob a licença **BSD 3-Clause**, uma licença
permissiva. Consulte o arquivo [LICENSE](LICENSE) para o texto completo.