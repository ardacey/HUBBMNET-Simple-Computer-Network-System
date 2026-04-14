# HUBBMNET

HUBBMNET is a standalone C++ network simulation project that models clients, routing tables, packet layers, message forwarding, and delivery logs.

## Features

- Multi-layer packet handling across application, transport, network, and physical layers
- Client routing with outgoing and incoming queues
- Message splitting, forwarding, reception, and drop handling
- Command-driven simulation with log output for each client
- Ready-to-run sample scenarios under `HUBBMNET/sampleIO`

## Repository Layout

- `HUBBMNET/` contains the source code and bundled sample inputs
- `HUBBMNET/sampleIO/` contains example input sets and expected outputs

## Build

This project does not require external libraries. Build it with any modern C++ compiler that supports C++17.

From the repository root:

```bash
g++ -std=c++17 -O2 -Wall -Wextra HUBBMNET/*.cpp -o hubbmnet
```

On Windows, the same command works with MinGW or a compatible `g++` installation. If you use another compiler, keep the source file list the same and target C++17.

## Run

The program expects six arguments:

1. client data file
2. routing table file
3. command file
4. message chunk limit
5. sender port
6. receiver port

Example:

```bash
./hubbmnet HUBBMNET/sampleIO/01_normal_delivery/clients.dat HUBBMNET/sampleIO/01_normal_delivery/routing.dat HUBBMNET/sampleIO/01_normal_delivery/commands.dat 20 0706 0607
```

## Sample Data

The `HUBBMNET/sampleIO` directory includes multiple scenarios you can use to verify behavior:

- normal message delivery
- corrupted routing data
- multiple messages with routing failures
- custom sample input sets

Each scenario includes input files and, where available, expected output references. The cleaner scenario folders are `HUBBMNET/sampleIO/01_normal_delivery`, `HUBBMNET/sampleIO/02_routing_corruption`, `HUBBMNET/sampleIO/03_multi_message_routing_failures`, and `HUBBMNET/sampleIO/04_custom_sample`.

## Notes

- The simulator writes its results to standard output.
- No external dependencies are required beyond the standard C++ library.
- The sample commands are provided as-is so you can compare runs against the bundled inputs.