# Virtual CAN

Biblioteca C++17 para comunicação com interfaces CAN usando a interface
`ICanBus`. O projeto inclui a implementação SocketCAN para Linux,
`SocketCanBus`, e testes executados contra uma interface CAN virtual (`vcan0`).

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

## Compilação

Configure o projeto na pasta `build` e compile:

```shell
cmake -S . -B build
cmake --build build
```

O primeiro comando lê o `CMakeLists.txt` e gera os arquivos de build. O
segundo compila a biblioteca `canbus` e o executável de testes `test_can`.

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

## Licença

Este projeto é distribuído sob a licença **BSD 3-Clause**, uma licença
permissiva. Consulte o arquivo [LICENSE](LICENSE) para o texto completo.