#include <iostream>
#include "Network.h"

using namespace std;

int main(int argc, char *argv[]) {

    Network network;

    vector<Client> clients = network.read_clients(argv[1]);
    network.read_routing_tables(clients, argv[2]);
    vector<string> commands = network.read_commands(argv[3]);

    int message_limit = stoi(argv[4]);
    string sender_port = argv[5];
    string receiver_port = argv[6];

    network.process_commands(clients, commands, message_limit, sender_port, receiver_port);

    return 0;
}


