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

## Licença

Este projeto é distribuído sob a licença **BSD 3-Clause**, uma licença
permissiva. Consulte o arquivo [LICENSE](LICENSE) para o texto completo.